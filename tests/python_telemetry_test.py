import importlib.util
import sys
import tempfile
import unittest
import uuid
from pathlib import Path

SCRIPT = Path(__file__).parents[1] / "scripts/rpi-dashboard-telemetry.py"
sys.dont_write_bytecode = True
SPEC = importlib.util.spec_from_file_location("telemetry_sender", SCRIPT)
sender = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(sender)


class TelemetrySenderTest(unittest.TestCase):
    def test_cbor_is_deterministic_and_uses_canonical_map_order(self):
        self.assertEqual(sender.cbor({"bbb": 2, "a": 1}), bytes.fromhex("a26161016362626202"))
        self.assertEqual(sender.cbor({"a": 1, "bbb": 2}), sender.cbor({"bbb": 2, "a": 1}))

    def test_device_id_is_persisted(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "identity"
            first = sender.device_id(path)
            self.assertEqual(first, sender.device_id(path))
            self.assertEqual(path.stat().st_mode & 0o777, 0o600)

    def test_registration_decoder_accepts_matching_response(self):
        device, instance = uuid.uuid4(), uuid.uuid4()
        response = sender.envelope("registration_result", device, instance)
        response["accepted"] = True
        sender.decode_registration(sender.cbor(response), device, instance)

    def test_interval_validation(self):
        with self.assertRaises(SystemExit):
            sender.main(["--dashboard-host", "localhost", "--interval", "0", "--once"])


if __name__ == "__main__":
    unittest.main()
