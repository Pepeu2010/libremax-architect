"""Verify actual conversions with official bundled Blender. No third-party models."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser();parser.add_argument('--blender',required=True);parser.add_argument('--evidence',required=True,type=Path);args=parser.parse_args()
args.evidence.mkdir(parents=True,exist_ok=True)
base=[args.blender,'--background','--factory-startup','--disable-autoexec','--python-exit-code','1','--python']
def run(script,*arguments,success=True):
    result=subprocess.run(base+[str(script),'--',*map(str,arguments)],capture_output=True,timeout=120)
    if (result.returncode==0)!=success:raise RuntimeError(result.stdout.decode(errors='replace')+result.stderr.decode(errors='replace'))
    return result
fixtures=args.evidence/'fixtures';run(root/'scripts/create-import-fixtures.py',fixtures)
report={}
for extension in ('glb','gltf','obj','fbx','stl','ply'):
    target=args.evidence/extension
    result=run(root/'scripts/import-model.py',fixtures/('model.'+extension),target)
    (args.evidence/(extension+'.log')).write_bytes(result.stdout+result.stderr)
    payload=(target/'model.json').read_bytes();model=json.loads(payload)
    assert model['schema']==1 and model['parts'] and len(payload)<4*1024*1024
    assert sum(len(p['triangles']) for p in model['parts'])==12
    textures=[m['baseColorTexture'] for m in model['materials'] if 'baseColorTexture' in m]
    if extension in ('glb','gltf','obj','fbx'):assert textures,extension
    for digest in textures:assert hashlib.sha256((target/(digest+'.png')).read_bytes()).hexdigest()==digest
    report[extension]=dict(bytes=len(payload),triangles=12,colorMaps=len(textures),details=json.loads((target/'result.json').read_bytes()))
bad=fixtures/'unsafe.gltf';bad.write_text(json.dumps(dict(asset=dict(version='2.0'),buffers=[dict(uri='../outside.bin',byteLength=10)])))
run(root/'scripts/import-model.py',bad,args.evidence/'rejected-path',success=False)
bad.write_text('{broken');run(root/'scripts/import-model.py',bad,args.evidence/'rejected-json',success=False)
(args.evidence/'report.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print('MODEL_FORMATS_PASS: GLB, glTF, OBJ, FBX, STL, PLY; mapped colors; bad paths and malformed JSON rejected')
