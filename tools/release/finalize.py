#!/usr/bin/env python3
import argparse, hashlib, json, shutil, xml.etree.ElementTree as ET
from pathlib import Path

def finish(stage, validation, installer, output, platform):
    tests=ET.parse(validation/'ctest.xml').getroot()
    suites=list(tests.iter('testsuite')) if tests.tag!='testsuite' else [tests]
    if not suites or any(int(s.get('failures','0'))+int(s.get('errors','0')) for s in suites): raise ValueError('Native tests did not pass')
    pluginval=json.loads((validation/'pluginval.json').read_text())
    if not pluginval['passed']: raise ValueError('pluginval did not pass')
    smoke=json.loads((validation/'installer-smoke.json').read_text())
    if not smoke['passed']: raise ValueError('Installer smoke failed')
    if platform=='macos' and not json.loads((validation/'auval.json').read_text())['passed']: raise ValueError('AU validation failed')
    destination=stage/'Validation';destination.mkdir(exist_ok=True)
    for path in validation.iterdir():
        if path.is_file():shutil.copy2(path,destination/path.name)
    shutil.copy2(installer,stage/installer.name)
    manifest={'candidate':'0.6.0-rc.1','native_tests':sum(int(s.get('tests','0')) for s in suites),'pluginval_strictness':5,'auval':platform=='macos','installer_smoke':True,'signed':False,'manual_daw_gate':'open','public_stable_release':False}
    (stage/'RELEASE-REPORT.json').write_text(json.dumps(manifest,indent=2)+'\n')
    lines=[]
    for file in sorted(stage.rglob('*')):
        if file.is_file():
            if not file.resolve().is_relative_to(stage.resolve()):raise ValueError('External package symlink')
            lines.append(hashlib.sha256(file.read_bytes()).hexdigest()+'  '+file.relative_to(stage).as_posix())
    (stage/'SHA256SUMS.txt').write_text('\n'.join(lines)+'\n')
    output.mkdir(parents=True,exist_ok=True)
    shutil.make_archive(str(output/('G-Boy-0.6.0-rc.1-'+platform)),'zip',stage)
    shutil.copy2(installer,output/installer.name)
    shutil.copy2(stage/'SHA256SUMS.txt',output/'SHA256SUMS.txt')
    for file in sorted(output.iterdir()):
        if file.is_file() and file.name!='DOWNLOAD-SHA256SUMS.txt':
            with (output/'DOWNLOAD-SHA256SUMS.txt').open('a') as out:out.write(hashlib.sha256(file.read_bytes()).hexdigest()+'  '+file.name+'\n')
    return manifest
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--stage',type=Path,required=True);p.add_argument('--validation',type=Path,required=True);p.add_argument('--installer',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--platform',required=True);a=p.parse_args();print(json.dumps(finish(a.stage,a.validation,a.installer,a.output,a.platform)))
