#!/usr/bin/env python3
"""Stage explicit release payloads and corresponding source; no build libraries/PDBs."""
import argparse, hashlib, json, os, re, shutil, tarfile
from pathlib import Path

def copy(source, destination):
    if source.is_dir(): shutil.copytree(source, destination, dirs_exist_ok=True, symlinks=True)
    else:
        destination.parent.mkdir(parents=True, exist_ok=True); shutil.copy2(source, destination)

def verify_content(root):
    manifest = json.loads((root / 'gboy-library.json').read_text(encoding='utf-8'))
    for entry in manifest['entries']:
        for key in ('file', 'preview'):
            if key not in entry: continue
            path = root / entry[key]
            if not path.resolve().is_relative_to(root.resolve()) or not path.is_file():
                raise ValueError('Missing or unsafe content: ' + entry[key])
        if not entry.get('license') or not entry.get('provenance'): raise ValueError('Missing provenance')
    return len(manifest['entries'])

def stage(repo, build, output, platform, commit):
    version = re.search(r'project\(GBoy VERSION ([0-9.]+)', (repo/'CMakeLists.txt').read_text()).group(1)
    if output.exists(): shutil.rmtree(output)
    output.mkdir(parents=True)
    artifacts = build/'GBoy_artefacts/Release'
    copy(artifacts/'VST3/G Boy.vst3', output/'VST3/G Boy.vst3')
    if platform == 'windows': copy(artifacts/'Standalone/G Boy.exe', output/'Standalone/G Boy.exe')
    else:
        copy(artifacts/'Standalone/G Boy.app', output/'Standalone/G Boy.app')
        copy(artifacts/'AU/G Boy.component', output/'AU/G Boy.component')
    copy(repo/'factory-content', output/'Factory Content')
    count = verify_content(output/'Factory Content')
    copy(repo/'docs', output/'Documentation')
    for name in ('README.md','LICENSE','LICENSE-BINARY.txt','AGPL-3.0.txt','GPL-3.0.txt','THIRD-PARTY.md'):
        copy(repo/name, output/'Documentation'/name)
    juce = build/'_deps/juce-src'
    if not (juce/'LICENSE.md').is_file(): raise ValueError('JUCE corresponding source missing')
    with tarfile.open(output/'Source.tar.gz','w:gz') as archive:
        allowed = ['CMakeLists.txt','README.md','LICENSE','LICENSE-BINARY.txt','AGPL-3.0.txt','GPL-3.0.txt','THIRD-PARTY.md','src','tests','tools','docs','installers','assets','.github','factory-content']
        def source_filter(info):
            parts = Path(info.name).parts
            if any(x in ('.git','__pycache__','.DS_Store') for x in parts) or info.name.endswith(('.wav','.gboy','.pyc')): return None
            info.uid = info.gid = 0; info.uname = info.gname = ''; return info
        for name in allowed:
            if (repo/name).exists(): archive.add(repo/name, arcname='G-Boy/'+name, filter=source_filter)
        archive.add(juce, arcname='JUCE', filter=source_filter)
    copy(juce/'LICENSE.md',output/'Documentation/JUCE-LICENSE.md')
    notices = [
        'modules/juce_audio_plugin_client/AU/AudioUnitSDK/LICENSE.txt',
        'modules/juce_audio_formats/codecs/flac/Flac Licence.txt',
        'modules/juce_audio_formats/codecs/oggvorbis/Ogg Vorbis Licence.txt',
        'modules/juce_audio_processors/format_types/VST3_SDK/LICENSE.txt',
        'modules/juce_audio_processors/format_types/LV2_SDK/lv2/COPYING',
        'modules/juce_graphics/image_formats/jpglib/README',
        'modules/juce_graphics/image_formats/pnglib/LICENSE',
        'modules/juce_core/zip/zlib/README',
        'modules/juce_graphics/fonts/harfbuzz/COPYING',
        'modules/juce_graphics/unicode/sheenbidi/LICENSE']
    for notice in notices:
        if (juce/notice).is_file(): copy(juce/notice,output/'Documentation/ThirdParty'/notice)
    info = dict(product='G Boy', version=version+'-rc.1', plugin_version=version, source_commit=commit,
                platform=platform, factory_assets=count, signing='unsigned release candidate',
                native_daw_testing='not yet performed', source_archive='Source.tar.gz', juce='8.0.6')
    (output/'BUILD-INFO.json').write_text(json.dumps(info,indent=2)+'\n')
    return info

if __name__ == '__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--repo',type=Path,default=Path.cwd());parser.add_argument('--build',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--platform',choices=['windows','macos'],required=True);parser.add_argument('--commit',default=os.environ.get('GITHUB_SHA','local'))
    args=parser.parse_args();print(json.dumps(stage(args.repo.resolve(),args.build.resolve(),args.output.resolve(),args.platform,args.commit)))
