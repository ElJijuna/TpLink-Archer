"""Fixed Archer research workflow. No retries, mode switching or ledger resets."""
import argparse
import fcntl
import hashlib
import json
import os
from pathlib import Path
import shutil
import stat
import subprocess

ROOT = Path(__file__).resolve().parents[1]
LEDGER = ROOT / 'artifacts/37ad-0104-sequences.bin'
TOOLS = {
    'inspect': 'archer-firmware-state-universal',
    'boot': 'archer-firmware-boot-universal',
    'radio': 'archer-radio-init-universal',
    'initial_rfk': 'archer-rfk-universal',
    'tx_rfk': 'archer-tx-calibrate-universal',
    'scan': 'archer-context-rx-universal',
    'replay': 'archer-passive-scan-decode-universal',
}


def plan(mode):
    if mode not in ('cold-start', 'recover-loaded', 'resume-bootstrap'):
        raise ValueError('Unknown workflow mode')
    phases = [('inspect', [], 'firmware_controls')]
    if mode == 'cold-start':
        phases.append(('boot', ['--boot-usb-no-dcpu'], 'firmware_ready'))
    phases += [
        ('radio', ['--resume-loaded-scc-radio' if mode == 'resume-bootstrap' else '--cold-scc-radio' if mode == 'cold-start' else '--powered-cold-scc-radio', '@capture'], 'scc_radio_nctl_applied'),
        ('initial_rfk', ['--initial-rfk', '@capture'], 'initial_pre_dack_rxdck_clean'),
        ('tx_rfk', ['--channel1-world-calibrate', '@capture'], 'tx_rfk_commands_completed'),
        ('scan', ['--run-owned-source-profile-usb-aggregation-all-management-async', '@capture'], 'context_cleanup_acknowledged'),
    ]
    return phases


def available_sequences(data):
    if len(data) != 264 or data[:8] != b'ARCHSEQ\x01' or any(x not in (0, 1) for x in data[8:]):
        raise ValueError('Invalid sequence ledger; no reset or reuse')
    return data[8:].count(0)


def phase_result(name, records, expected):
    matches = [r for r in records if r.get('result') == expected or r.get('phase') == expected]
    if len(matches) != 1:
        raise ValueError(f'{name}: missing/ambiguous terminal result')
    r = matches[0]
    if name == 'inspect':
        # Control bits only. Identity of resident firmware is still not read back.
        if r.get('run_environment') != 0 or r.get('status') != 3:
            raise ValueError('Unexpected firmware controls before workflow')
    if name in ('tx_rfk', 'scan') and r.get('network_tx_paused') is not True:
        raise ValueError('Network TX pause not confirmed')
    if name == 'scan' and not any(x == {'operation': 'bulk_async_drained', 'pending': 0} for x in records):
        raise ValueError('Async USB shutdown not confirmed')
    return r


def save(path, value):
    temp = path.with_suffix('.tmp')
    with temp.open('w') as f:
        json.dump(value, f, indent=2)
        f.write('\n')
        f.flush()
        os.fsync(f.fileno())
    os.replace(temp, path)
    fd = os.open(path.parent, os.O_RDONLY)
    try:
        os.fsync(fd)
    finally:
        os.close(fd)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def execute(mode, output):
    output = output.resolve()
    output.mkdir()  # Each attempt owns a fresh directory; never overwrite or resume.
    manifest = {'mode': mode, 'status': 'preflight', 'phases': [],
                'quality_validated': False, 'resident_version_readback': False}
    report = output / 'workflow.json'
    lock = os.open(str(LEDGER) + '.workflow', os.O_RDWR | os.O_CREAT | os.O_NOFOLLOW, 0o600)
    try:
        info = os.fstat(lock)
        if not stat.S_ISREG(info.st_mode) or info.st_nlink != 1:
            raise ValueError('Invalid workflow ownership file')
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        remaining = available_sequences(LEDGER.read_bytes())
        manifest['sequences_available_before'] = remaining
        if remaining < 22:
            raise ValueError('Need 22 unused sequences before complete workflow; no automatic epoch reset')
        lease = ROOT / 'artifacts/37ad-0104-context-reservation'
        if not lease.exists() or lease.read_bytes() != b'CLEAN\n':
            raise ValueError('Existing context is not CLEAN; no automatic recovery of uncertain commands')
        bindir = output / 'bin'
        bindir.mkdir()
        binaries = {}
        for name, filename in TOOLS.items():
            source = ROOT / 'build' / filename
            target = bindir / filename
            shutil.copy2(source, target)
            binaries[name] = target
        manifest['binaries'] = {name: {'path': str(p), 'sha256': digest(p)} for name, p in binaries.items()}
        manifest['ledger_sha256_before'] = digest(LEDGER)
        manifest['status'] = 'running'
        save(report, manifest)
        env = dict(os.environ, ARCHER_WORKFLOW_FD=str(lock))
        for name, arguments, expected in plan(mode):
            args = [str(output / name) if a == '@capture' else a for a in arguments]
            entry = {'name': name, 'arguments': args, 'status': 'started'}
            manifest['phases'].append(entry)
            save(report, manifest)  # Records intent before launching any child.
            print(json.dumps({'phase': name, 'status': 'started'}), flush=True)
            stdout, stderr = output / (name + '.jsonl'), output / (name + '.stderr')
            with stdout.open('w') as out, stderr.open('w') as err:
                # No timeout kill/replay: native phases have bounded waits and
                # cleanup. An external interruption leaves durable started state.
                process = subprocess.run([str(binaries[name]), *args], cwd=ROOT, env=env,
                                         pass_fds=(lock,), stdout=out, stderr=err, check=False)
            entry.update(exit_code=process.returncode, stdout_sha256=digest(stdout), stderr_sha256=digest(stderr))
            if process.returncode:
                raise RuntimeError(f'{name} exited {process.returncode}; no next phase or replay')
            records = [json.loads(line) for line in stdout.read_text().splitlines()]
            entry['result'] = phase_result(name, records, expected)
            entry['status'] = 'completed'
            save(report, manifest)
            print(json.dumps({'phase': name, 'status': 'completed'}), flush=True)
        live = json.loads((output / 'scan/networks.json').read_text())
        replay_file = output / 'scan-replay.json'
        with replay_file.open('w') as out:
            subprocess.run([str(binaries['replay']), '--channel1-fcs-appended', str(output / 'scan')],
                           cwd=ROOT, stdout=out, stderr=subprocess.PIPE, check=True)
        if live != json.loads(replay_file.read_text()):
            raise ValueError('Live/offline scan results differ')
        if lease.read_bytes() != b'CLEAN\n':
            raise ValueError('Context ownership not CLEAN after scan')
        manifest.update(status='completed', live_equals_replay=True,
                        scan_summary={k: v for k, v in live.items() if k != 'networks'},
                        network_entries=len(live['networks']),
                        sequences_available_after=available_sequences(LEDGER.read_bytes()),
                        ledger_sha256_after=digest(LEDGER), networks_sha256=digest(output / 'scan/networks.json'))
        save(report, manifest)
        print(json.dumps({'status': 'completed', 'network_entries': len(live['networks'])}), flush=True)
    except BaseException as error:
        manifest.update(status='stopped', error=str(error))
        save(report, manifest)
        raise
    finally:
        os.close(lock)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mode', choices=['cold-start', 'recover-loaded', 'resume-bootstrap'])
    parser.add_argument('new_directory', type=Path)
    args = parser.parse_args()
    execute(args.mode, args.new_directory)
