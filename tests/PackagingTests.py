import importlib.util
import json
import tempfile
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
def module(name):
    spec=importlib.util.spec_from_file_location(name,ROOT/'tools/release'/f'{name}.py');result=importlib.util.module_from_spec(spec);spec.loader.exec_module(result);return result
stage=module('stage');finalize=module('finalize')
class PackagingTests(unittest.TestCase):
    def test_content_requires_real_contained_files_and_provenance(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);(root/'sound.wav').write_bytes(b'fixture')
            manifest={'entries':[{'file':'sound.wav','license':'CC0','provenance':'test'}]};file=root/'gboy-library.json';file.write_text(json.dumps(manifest))
            self.assertEqual(stage.verify_content(root),1)
            manifest['entries'][0]['file']='../outside.wav';file.write_text(json.dumps(manifest))
            with self.assertRaises(ValueError):stage.verify_content(root)
            manifest['entries'][0]['file']='sound.wav';del manifest['entries'][0]['license'];file.write_text(json.dumps(manifest))
            with self.assertRaises(ValueError):stage.verify_content(root)
    def test_failed_external_validation_prevents_final_package(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);validation=root/'validation';validation.mkdir();(validation/'ctest.xml').write_text('<testsuite tests="10" failures="0" errors="0"/>');(validation/'pluginval.json').write_text('{"passed":false}')
            with self.assertRaises(ValueError):finalize.finish(root,validation,root/'missing.exe',root/'dist','windows')
    def test_checksums_cover_actual_payload_and_gates_remain_honest(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);payload=root/'stage';payload.mkdir();validation=root/'validation';validation.mkdir();(payload/'binary').write_bytes(b'payload');(validation/'ctest.xml').write_text('<testsuite tests="10" failures="0" errors="0"/>');(validation/'pluginval.json').write_text('{"passed":true}');(validation/'installer-smoke.json').write_text('{"passed":true}');installer=root/'installer.exe';installer.write_bytes(b'installer')
            report=finalize.finish(payload,validation,installer,root/'dist','windows')
            self.assertEqual(report['native_tests'],10);self.assertFalse(report['signed']);self.assertFalse(report['public_stable_release']);self.assertTrue((payload/'SHA256SUMS.txt').is_file());self.assertTrue(list((root/'dist').glob('*.zip')))
if __name__=='__main__':unittest.main()
