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


def validate_reload(rows, cold=False):
    required = [('phase', 'epoch_cold_profile_verified' if cold else 'epoch_mac_reset_verified'), ('phase', 'cpu_reset'),
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


def transition(output, discard_dirty_context=False, cold=False, finish_cold=False):
    output = output.resolve()
    if not finish_cold:output.mkdir()
    report = output / 'epoch.json'
    record = {'epoch_id': str(uuid.uuid4()), 'status': 'preflight',
              'mode': 'cold-reload-no-h2c' if cold else 'powered-reload-no-h2c', 'discard_dirty_context': discard_dirty_context, 'resident_version_readback': False}
    earlier_rows=[]
    if finish_cold:
        cold=True
        record=json.loads(report.read_text())
        if record.get('status')!='stopped' or record.get('mode')!='cold-reload-no-h2c':
            raise ValueError('Only a stopped cold boundary may be finished')
        if digest(output/'reload.jsonl')!=record.get('reload_stdout_sha256'):
            raise ValueError('Original cold download log changed')
        earlier_rows=[json.loads(line) for line in (output/'reload.jsonl').read_text().splitlines()]
        for field in ('epoch_cold_profile_verified','cpu_reset','firmware_ready'):
            if sum(row.get('phase')==field for row in earlier_rows)!=1:
                raise ValueError('Original cold download did not reach a unique ready phase')
        writes=[row for row in earlier_rows if row.get('operation')=='firmware_bulk_write']
        if len(writes)!=212 or any(row.get('status')!=0 for row in writes):
            raise ValueError('Original cold download incomplete; no finalization')
        record['initial_reload_evidence']={key:record.get(key) for key in ('reload_exit','reload_stdout_sha256','reload_stderr_sha256','helper_sha256')}
        if 'error' in record:record['previous_failure']=record.pop('error')
        record['finishing_cold_boundary']=True
    locks = []
    try:
        owner = os.open(str(LEDGER) + '.workflow', os.O_RDWR | os.O_CREAT | os.O_NOFOLLOW, 0o600)
        locks.append(owner)
        info = os.fstat(owner)
        if not stat.S_ISREG(info.st_mode) or info.st_nlink != 1:
            raise ValueError('Invalid workflow lock')
        fcntl.flock(owner, fcntl.LOCK_EX | fcntl.LOCK_NB)
        fence = Path(str(LEDGER) + '.epoch-transition')
        if fence.is_symlink() or (fence.exists() and fence.read_bytes() != (b'DIRTY\n' if finish_cold else b'CLEAN\n')):
            raise ValueError('Previous epoch transition incomplete; no automatic retry')
        if finish_cold and not fence.exists():
            raise ValueError('Original cold boundary fence is missing')
        lease = ROOT / 'artifacts/37ad-0104-context-reservation'
        context_fd = os.open(lease, os.O_RDWR | os.O_NOFOLLOW)
        locks.append(context_fd)
        context_stat = os.fstat(context_fd)
        if not stat.S_ISREG(context_stat.st_mode) or context_stat.st_nlink != 1:
            raise ValueError('Invalid context reservation file')
        fcntl.flock(context_fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
        previous_context = os.pread(context_fd, 7, 0)
        if previous_context != b'CLEAN\n' and not (discard_dirty_context and previous_context == b'DIRTY\n'):
            raise ValueError('Context uncertain; explicit discard after verified reload required')
        if finish_cold:
            if (output/'previous-context.bin').read_bytes()!=previous_context:
                raise ValueError('Context changed since failed cold boundary')
        else:
            with (output / 'previous-context.bin').open('xb') as archive_context:
                archive_context.write(previous_context)
                archive_context.flush()
                os.fsync(archive_context.fileno())
        record['previous_context'] = previous_context.decode().strip()
        old_fd = os.open(LEDGER, os.O_RDWR | os.O_NOFOLLOW)
        locks.append(old_fd)
        info = os.fstat(old_fd)
        if not stat.S_ISREG(info.st_mode) or info.st_nlink != 1:
            raise ValueError('Invalid old ledger')
        fcntl.flock(old_fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
        old = os.pread(old_fd, 265, 0)
        record['available_before'] = available_sequences(old)
        archive = output / 'previous-ledger.bin'
        if finish_cold:
            if archive.read_bytes()!=old:raise ValueError('Ledger changed since failed cold boundary')
        else:
            with archive.open('xb') as f:
                f.write(old)
                f.flush()
                os.fsync(f.fileno())
        helper = output / ('archer-epoch-boot-finish' if finish_cold else 'archer-epoch-boot')
        shutil.copy2(ROOT / 'build/archer-epoch-boot-universal', helper)
        record.update(old_ledger_sha256=digest(archive), helper_sha256=digest(helper))
        save(report, record)
        # All updated SequenceLedger users reject this fence, even after a crash.
        # This does not claim compatibility with unmodified historical binaries.
        set_fence(fence, b'DIRTY\n')
        record['status'] = 'reload_started'
        save(report, record)
        env = dict(os.environ, ARCHER_WORKFLOW_FD=str(owner), ARCHER_EPOCH_LEDGER_FD=str(old_fd))
        stdout, stderr = output / ('finish.jsonl' if finish_cold else 'reload.jsonl'), output / ('finish.stderr' if finish_cold else 'reload.stderr')
        with stdout.open('w') as out, stderr.open('w') as err:
            process = subprocess.run([str(helper), '--finish-cold-boundary-no-h2c' if finish_cold else '--reload-cold-no-h2c' if cold else '--reload-powered-no-h2c', str(output / ('finish' if finish_cold else 'reload'))],
                                     cwd=ROOT, env=env, pass_fds=(owner, old_fd), stdout=out, stderr=err, check=False)
        record.update(reload_exit=process.returncode, reload_stdout_sha256=digest(stdout), reload_stderr_sha256=digest(stderr))
        if process.returncode:
            raise RuntimeError('Firmware boundary failed; old ledger retained, transition fenced')
        record['boundary'] = validate_reload(earlier_rows+[json.loads(line) for line in stdout.read_text().splitlines()], cold)
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
        # Only reset the context after the full MAC/DLE/CPU reload boundary.
        current_context_stat = os.lstat(lease)
        if (current_context_stat.st_dev, current_context_stat.st_ino) != (context_stat.st_dev, context_stat.st_ino):
            raise ValueError('Context reservation path changed during reload')
        if os.pread(context_fd, 7, 0) != previous_context:
            raise ValueError('Context reservation changed during reload')
        if os.pwrite(context_fd, b'CLEAN\n', 0) != 6:
            raise OSError('Incomplete context reset write')
        os.fsync(context_fd)
        record['context_after'] = 'CLEAN'
        save(report, record)
        set_fence(fence, b'CLEAN\n')
        record['status'] = 'committed'
        save(report, record)
        print(json.dumps({'status': 'committed', 'available_sequences': record['available_after'],
                          'next_workflow_mode': 'cold-radio-bootstrap' if cold else 'resume-bootstrap'}), flush=True)
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
    parser.add_argument('--discard-dirty-context', action='store_true', help='Archive uncertain context and discard it only after verified MAC/DLE/CPU reload; does not permit another probe')
    parser.add_argument('--cold-after-reconnect', action='store_true', help='Require cold BB/RF baseline after physical reconnect and use cold-radio bootstrap')
    parser.add_argument('--finish-cold-boundary', action='store_true', help='Finish a preserved cold download using control verification/drain only; never reloads firmware')
    args = parser.parse_args()
    transition(args.new_directory, args.discard_dirty_context, args.cold_after_reconnect, args.finish_cold_boundary)
