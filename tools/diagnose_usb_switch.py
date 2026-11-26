"""One verified installer eject with a bounded USB timeline; no firmware or Wi-Fi TX."""
import argparse
import json
from pathlib import Path
import re
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]
HELPER = ROOT / 'build/archer-optical-eject-universal'


def parse_devices(text):
    devices = []
    for block in re.split(r'(?m)^\+-o ', text):
        if '<class IOUSBHostDevice,' not in block.split('\n', 1)[0]:
            continue
        fields = {k: int(v) for k, v in re.findall(
            r'(?m)^  \|   "(idVendor|idProduct|sessionID|locationID)" = (\d+)\s*$', block)}
        identity = (fields.get('idVendor'), fields.get('idProduct'))
        if identity in ((0x0bda, 0x1a2b), (0x37ad, 0x0104)):
            fields['mode'] = 'wifi' if identity[0] == 0x37ad else 'installer'
            devices.append(fields)
    return sorted(devices, key=lambda d: (d['idVendor'], d.get('sessionID', 0)))


def snapshot():
    result = subprocess.run(['/usr/sbin/ioreg', '-r', '-c', 'IOUSBHostDevice', '-l', '-w', '0'],
                            capture_output=True, text=True, timeout=5, check=True)
    return parse_devices(result.stdout)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    parser.add_argument('--current', action='store_true', help='Use current single stable installer; do not ask for unplug/replug')
    parser.add_argument('--bulk-eject', action='store_true', help='Contrast one START STOP UNIT using the existing verified USB installer helper, without ioctl fallback')
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir()  # Never overwrite an earlier observation.
    start = time.monotonic()
    with (output / 'timeline.jsonl').open('x') as log:
        def emit(operation, **fields):
            row = dict(operation=operation, elapsed_seconds=round(time.monotonic()-start, 3), **fields)
            line = json.dumps(row)
            print(line, flush=True)
            log.write(line+'\n'); log.flush()

        previous = None
        def observe():
            nonlocal previous
            current = snapshot()
            if current != previous:
                emit('usb_state', devices=current)
                previous = current
            return current

        def wait_for(predicate, seconds):
            until = time.monotonic()+seconds
            while time.monotonic() < until:
                current = observe()
                if predicate(current):
                    return current
                time.sleep(0.5)
            raise RuntimeError('Timed out waiting for requested physical USB state')

        try:
            subprocess.run(['sudo', '-n', 'true'], check=True, timeout=5)
            if args.current:
                connected = observe()
                if len(connected) != 1 or connected[0]['mode'] != 'installer':
                    raise RuntimeError('Current mode requires exactly one installer')
                for _ in range(6):
                    time.sleep(0.5)
                    if observe() != connected:
                        raise RuntimeError('Current USB session is not stable; no eject attempted')
                emit('current_session_stable')
            else:
                print('Retira el Archer y déjalo fuera hasta el siguiente mensaje.', flush=True)
                wait_for(lambda ds: not ds, 60)
                emit('absence_verified')
                print('Ahora conecta el Archer; no ejecutes otros comandos.', flush=True)
                connected = wait_for(lambda ds: bool(ds), 60)
            if len(connected) != 1:
                raise RuntimeError('Expected exactly one supported Archer device')
            if connected[0]['mode'] == 'wifi':
                emit('already_wifi_no_eject')
            else:
                until = time.monotonic()+15
                while True:
                    current = observe()
                    if current != connected:
                        raise RuntimeError('USB session changed before verified eject; stopped')
                    inspection_helper = ROOT / 'build/archer-mode-switch-universal' if args.bulk_eject else HELPER
                    inspected = subprocess.run([str(inspection_helper), '--inspect'], cwd=ROOT,
                                               capture_output=True, text=True, timeout=5)
                    if inspected.returncode == 0:
                        break
                    if time.monotonic() >= until:
                        raise RuntimeError('Installer CD did not become available; no eject attempted')
                    time.sleep(0.5)
                emit('installer_inspection', stdout=inspected.stdout.strip())
                # Exactly one invocation; existing helper checks identity again.
                command = (['sudo', '-n', str(ROOT / 'build/archer-mode-switch-universal'), '--eject-only-installer']
                           if args.bulk_eject else ['sudo', '-n', str(HELPER), '--eject-installer'])
                emit('eject_method', method='usb_start_stop_unit_once' if args.bulk_eject else 'DKIOCEJECT')
                result = subprocess.run(command, cwd=ROOT,
                                        capture_output=True, text=True, timeout=10)
                emit('eject_result', exit_code=result.returncode,
                     stdout=result.stdout.strip(), stderr=result.stderr.strip())
                if result.returncode and not args.bulk_eject:
                    raise RuntimeError('Eject failed; no retry')
                if result.returncode:
                    emit('command_unconfirmed', exit_code=result.returncode, retry=False)
            until = time.monotonic()+20
            saw_wifi = False
            while time.monotonic() < until:
                saw_wifi |= any(d['mode'] == 'wifi' for d in observe())
                time.sleep(0.25)
            final = observe()
            emit('result', status='observed', saw_wifi=saw_wifi, final_devices=final,
                 automatic_retry=False, firmware_submissions=0, network_submissions=0)
        except (OSError, RuntimeError, subprocess.SubprocessError) as error:
            emit('result', status='stopped', error=str(error), automatic_retry=False)
            return 2
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
