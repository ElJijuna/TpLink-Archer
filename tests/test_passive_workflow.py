import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location('workflow', Path(__file__).parents[1] / 'tools/passive_workflow.py')
w = importlib.util.module_from_spec(spec)
spec.loader.exec_module(w)


class WorkflowTest(unittest.TestCase):
    def test_modes_and_budget(self):
        self.assertEqual([x[0] for x in w.plan('cold-start')], ['inspect', 'boot', 'radio', 'initial_rfk', 'tx_rfk', 'scan'])
        self.assertEqual(w.plan('recover-loaded')[1][1][0], '--powered-cold-scc-radio')
        self.assertEqual(w.plan('resume-bootstrap')[1][1][0], '--resume-loaded-scc-radio')
        self.assertEqual(w.available_sequences(b'ARCHSEQ\x01' + bytes([1] * 223 + [0] * 33)), 33)
        for bad in [b'', b'badmagic' + bytes(256), b'ARCHSEQ\x01' + bytes([2] * 256)]:
            with self.assertRaises(ValueError):
                w.available_sequences(bad)

    def test_missing_or_uncertain_results(self):
        for rows in [[], [{'result': 'other'}], [{'result': 'context_cleanup_acknowledged', 'network_tx_paused': True}]]:
            with self.assertRaises(ValueError):
                w.phase_result('scan', rows, 'context_cleanup_acknowledged')
        rows = [{'result': 'context_cleanup_acknowledged', 'network_tx_paused': True}, {'operation': 'bulk_async_drained', 'pending': 0}]
        self.assertTrue(w.phase_result('scan', rows, 'context_cleanup_acknowledged')['network_tx_paused'])
        with self.assertRaises(ValueError):
            w.phase_result('scan', rows + rows, 'context_cleanup_acknowledged')

    def test_quality_is_not_promoted(self):
        result = {'result': 'tx_rfk_commands_completed', 'network_tx_paused': True, 'all_calibration_quality_validated': False}
        self.assertFalse(w.phase_result('tx_rfk', [result], result['result'])['all_calibration_quality_validated'])

    def test_failure_stops_before_next_phase(self):
        import contextlib
        import io
        import tempfile
        from unittest.mock import patch
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / 'artifacts').mkdir()
            ledger = root / 'artifacts/37ad-0104-sequences.bin'
            ledger.write_bytes(b'ARCHSEQ\x01' + bytes([1] * 223 + [0] * 33))
            (root / 'artifacts/37ad-0104-context-reservation').write_text('CLEAN\n')
            (root / 'build').mkdir()
            for name, filename in w.TOOLS.items():
                p = root / 'build' / filename
                text = "print('{\"result\":\"firmware_controls\",\"status\":3,\"run_environment\":0}')" if name == 'inspect' else 'raise SystemExit(7)'
                p.write_text('#!/usr/bin/env python3\n' + text + '\n')
                p.chmod(0o700)
            output = root / 'run'
            with patch.multiple(w, ROOT=root, LEDGER=ledger), contextlib.redirect_stdout(io.StringIO()):
                with self.assertRaises(RuntimeError):
                    w.execute('recover-loaded', output)
            report = w.json.loads((output / 'workflow.json').read_text())
            self.assertEqual(report['status'], 'stopped')
            self.assertEqual([p['name'] for p in report['phases']], ['inspect', 'radio'])
            self.assertFalse((output / 'initial_rfk.jsonl').exists())
            # An exclusive workflow lease is released after stopping.
            import fcntl
            with open(str(ledger) + '.workflow', 'r+') as f:
                fcntl.flock(f, fcntl.LOCK_EX | fcntl.LOCK_NB)

    def test_exhaustion_preflight_has_no_child_process(self):
        import tempfile
        from unittest.mock import patch
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            ledger = root / 'ledger'
            ledger.write_bytes(b'ARCHSEQ\x01' + bytes([1] * 235 + [0] * 21))
            output = root / 'run'
            with patch.multiple(w, ROOT=root, LEDGER=ledger), patch.object(w.subprocess, 'run') as launch:
                with self.assertRaises(ValueError):
                    w.execute('cold-start', output)
                launch.assert_not_called()
            report = w.json.loads((output / 'workflow.json').read_text())
            self.assertEqual(report['status'], 'stopped')
            self.assertEqual(report['phases'], [])

    def test_sweep_plan_and_terminal_scope(self):
        self.assertEqual(w.plan('recover-loaded', True)[-1],
                         ('sweep', ['--owned-slot0-channel1-6-11-1', '@capture'],
                          'passive_channel1_6_11_1_complete'))
        row = dict(result='passive_channel1_6_11_1_complete', network_tx_paused=True,
                   rxdck_all_channels_clean=True, full_rf_quality_validated=False)
        self.assertEqual(w.phase_result('sweep', [row], row['result']), row)
        for field, value in [('network_tx_paused', False), ('rxdck_all_channels_clean', False),
                             ('full_rf_quality_validated', True)]:
            with self.assertRaises(ValueError):
                w.phase_result('sweep', [{**row, field:value}], row['result'])

    def test_sweep_budget_stops_before_any_child(self):
        import tempfile
        from unittest.mock import patch
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp)
            ledger=root/'ledger'
            ledger.write_bytes(b'ARCHSEQ\x01'+bytes([1]*229+[0]*27))
            with patch.multiple(w, ROOT=root, LEDGER=ledger), patch.object(w.subprocess, 'run') as launch:
                with self.assertRaises(ValueError):
                    w.execute('cold-start', root/'run', sweep=True)
                launch.assert_not_called()

    def test_cold_radio_bootstrap_uses_verified_loaded_image(self):
        phases=w.plan('cold-radio-bootstrap')
        self.assertEqual(phases[1][0],'radio')
        self.assertEqual(phases[1][1][0],'--cold-scc-radio')
        self.assertNotIn('boot',[p[0] for p in phases])
