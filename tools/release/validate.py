#!/usr/bin/env python3
"""Pinned external validation; record commands, tool hash, logs and real exit status."""
import argparse, hashlib, json, os, stat, subprocess, sys, urllib.request, zipfile
from pathlib import Path

def run(command, log, timeout=900):
    log.parent.mkdir(parents=True,exist_ok=True)
    with log.open('w',encoding='utf-8') as output:
        try: result=subprocess.run(command,stdout=output,stderr=subprocess.STDOUT,timeout=timeout);code=result.returncode
        except subprocess.TimeoutExpired: code=124
    return {'command':list(map(str,command)), 'exit_code':code,'passed':code==0,'log':log.name}

def main():
    p=argparse.ArgumentParser();p.add_argument('--platform',choices=['windows','macos'],required=True);p.add_argument('--plugin',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--au',action='store_true');args=p.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    if args.au:
        result=run(['/usr/bin/auval','-v','aumu','Gb01','Gboy'],args.output/'auval.log')
        (args.output/'auval.json').write_text(json.dumps(result,indent=2));return 0 if result['passed'] else 1
    tool=args.output/'pluginval-tool';tool.mkdir(exist_ok=True)
    asset='pluginval_Windows.zip' if args.platform=='windows' else 'pluginval_macOS.zip'
    url='https://github.com/Tracktion/pluginval/releases/download/v1.0.4/'+asset
    with urllib.request.urlopen(url,timeout=90) as response: data=response.read()
    archive=tool/asset;archive.write_bytes(data)
    with zipfile.ZipFile(archive) as package:
        for item in package.infolist():
            if not (tool/item.filename).resolve().is_relative_to(tool.resolve()): raise ValueError('Unsafe validator archive')
        package.extractall(tool)
    binary=next(tool.rglob('pluginval.exe')) if args.platform=='windows' else next(tool.rglob('pluginval.app'))/'Contents/MacOS/pluginval'
    if args.platform=='macos': binary.chmod(binary.stat().st_mode|stat.S_IXUSR|stat.S_IXGRP|stat.S_IXOTH)
    command=[str(binary.resolve()),'--validate-in-process','--strictness-level','5','--timeout-ms','120000','--output-dir',str(args.output.resolve()),str(args.plugin.resolve())]
    result=run(command,args.output/'pluginval-console.log');result.update(version='1.0.4',strictness=5,download=url,archive_sha256=hashlib.sha256(data).hexdigest())
    (args.output/'pluginval.json').write_text(json.dumps(result,indent=2));print(json.dumps(result));return 0 if result['passed'] else 1
if __name__=='__main__':sys.exit(main())
