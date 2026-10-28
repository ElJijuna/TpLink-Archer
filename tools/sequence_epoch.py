"""Explicit research epoch transition after verified firmware reload; never wraps."""
import argparse
import fcntl
import json
import os
from pathlib import Path
import shutil
import stat
import subprocess
import uuid
from passive_workflow import ROOT, LEDGER, available_sequences, save, digest


def validate_reload(rows):
    required = [('phase', 'epoch_mac_reset_verified'), ('phase', 'cpu_reset'),
                ('phase', 'firmware_ready'), ('operation', 'bulk_async_drained'),
                ('result', 'epoch_firmware_reloaded_no_h2c')]
    indices = []
    for key, value in required:
        found = [i for i, r in enumerate(rows) if r.get(key) == value]
        if len(found) != 1:
            raise ValueError('Missing/duplicate epoch boundary evidence: ' + value)
        indices.append(found[0])
    if indices != sorted(indices):
        raise ValueError('Out-of-order epoch boundary evidence')
    result = rows[indices[-1]]
    if result.get('h2c_submissions') != 0 or result.get('quiet_ms_min', 0) < 250:
        raise ValueError('No clean no-H2C boundary')
    if rows[indices[-2]].get('pending') != 0:
        raise ValueError('Outstanding USB callbacks at epoch boundary')
    writes = [r for r in rows if r.get('operation') == 'firmware_bulk_write']
    if len(writes) != 212 or any(r.get('status') != 0 for r in writes):
        raise ValueError('Incomplete bound firmware download')
    return result


def new_epoch_bytes():
    # Keep historical protocol reservations 0/1 conservative in every epoch.
    return b'ARCHSEQ\x01' + bytes([1, 1]) + bytes(254)


def transition(output):
    output = output.resolve()
    output.mkdir()
    report = output / 'epoch.json'
    record = {'epoch_id': str(uuid.uuid4()), 'status': 'preflight',
              'mode': 'powered-reload-no-h2c', 'resident_version_readback': False}
    locks = []
    try:
        owner = os.open(str(LEDGER) + '.workflow', os.O_RDWR | os.O_CREAT | os.O_NOFOLLOW, 0o600)
        locks.append(owner)
        info = os.fstat(owner)
        if not stat.S_ISREG(info.st_mode) or info.st_nlink != 1:
            raise ValueError('Invalid workflow lock')
        fcntl.flock(owner, fcntl.LOCK_EX | fcntl.LOCK_NB)
        fence = Path(str(LEDGER) + '.epoch-transition')
        if fence.is_symlink() or (fence.exists() and fence.read_bytes() != b'CLEAN\n'):
            raise ValueError('Previous epoch transition incomplete; no automatic retry')
        lease = ROOT / 'artifacts/37ad-0104-context-reservation'
        if lease.read_bytes() != b'CLEAN\n':
            raise ValueError('Context uncertain; epoch transition refused')
        old_fd = os.open(LEDGER, os.O_RDWR | os.O_NOFOLLOW)
        locks.append(old_fd)
        info = os.fstat(old_fd)
        if not stat.S_ISREG(info.st_mode) or info.st_nlink != 1:
            raise ValueError('Invalid old ledger')
        fcntl.flock(old_fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
        old = os.pread(old_fd, 265, 0)
        record['available_before'] = available_sequences(old)
        archive = output / 'previous-ledger.bin'
        with archive.open('xb') as f:
            f.write(old)
            f.flush()
            os.fsync(f.fileno())
        helper = output / 'archer-epoch-boot'
        shutil.copy2(ROOT / 'build/archer-epoch-boot-universal', helper)
        record.update(old_ledger_sha256=digest(archive), helper_sha256=digest(helper))
        save(report, record)
        # All updated SequenceLedger users reject this fence, even after a crash.
        # This does not claim compatibility with unmodified historical binaries.
        set_fence(fence, b'DIRTY\n')
        record['status'] = 'reload_started'
        save(report, record)
        env = dict(os.environ, ARCHER_WORKFLOW_FD=str(owner), ARCHER_EPOCH_LEDGER_FD=str(old_fd))
        stdout, stderr = output / 'reload.jsonl', output / 'reload.stderr'
        with stdout.open('w') as out, stderr.open('w') as err:
            process = subprocess.run([str(helper), '--reload-powered-no-h2c', str(output / 'reload')],
                                     cwd=ROOT, env=env, pass_fds=(owner, old_fd), stdout=out, stderr=err, check=False)
        record.update(reload_exit=process.returncode, reload_stdout_sha256=digest(stdout), reload_stderr_sha256=digest(stderr))
        if process.returncode:
            raise RuntimeError('Firmware boundary failed; old ledger retained, transition fenced')
        record['boundary'] = validate_reload([json.loads(line) for line in stdout.read_text().splitlines()])
        # The helper submits no H2C after reload. If that changes, new-epoch
        # sequence reservations must be carried forward before allowing commit.
        if os.pread(old_fd, 265, 0) != old:
            raise ValueError('Old ledger unexpectedly modified during no-H2C reload')
        actual = os.lstat(LEDGER)
        if (actual.st_dev, actual.st_ino) != (info.st_dev, info.st_ino):
            raise ValueError('Ledger path changed during transition')
        record['status'] = 'boundary_verified'
        save(report, record)
        next_path = Path(str(LEDGER) + '.next-' + record['epoch_id'])
        new_fd = os.open(next_path, os.O_RDWR | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW, 0o600)
        locks.append(new_fd)
        fcntl.flock(new_fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
        data = new_epoch_bytes()
        if os.write(new_fd, data) != len(data):
            raise OSError('Incomplete new epoch write')
        os.fsync(new_fd)
        os.replace(next_path, LEDGER)
        sync_directory(LEDGER.parent)
        record.update(status='ledger_replaced', new_ledger_sha256=digest(LEDGER), available_after=available_sequences(data))
        save(report, record)
        set_fence(fence, b'CLEAN\n')
        record['status'] = 'committed'
        save(report, record)
        print(json.dumps({'status': 'committed', 'available_sequences': record['available_after'],
                          'next_workflow_mode': 'resume-bootstrap'}), flush=True)
    except BaseException as error:
        record.update(status='stopped', error=str(error))
        save(report, record)
        raise
    finally:
        for fd in reversed(locks):
            os.close(fd)


def sync_directory(path):
    fd = os.open(path, os.O_RDONLY)
    try:
        os.fsync(fd)
    finally:
        os.close(fd)


def set_fence(path, data):
    temp = path.with_name(path.name + '.tmp-' + str(uuid.uuid4()))
    fd = os.open(temp, os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW, 0o600)
    try:
        if os.write(fd, data) != len(data):
            raise OSError('Incomplete epoch fence write')
        os.fsync(fd)
    finally:
        os.close(fd)
    os.replace(temp, path)
    sync_directory(path.parent)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('new_directory', type=Path)
    transition(parser.parse_args().new_directory)
