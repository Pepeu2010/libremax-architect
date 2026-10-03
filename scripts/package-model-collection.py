"""Create an offline .lmaxpack containing immutable meshes, maps and provenance."""
import argparse
import hashlib
import json
from pathlib import Path
import zipfile
root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser();parser.add_argument('output',type=Path);args=parser.parse_args()
catalog=json.loads((root/'starter-models/home-catalog.json').read_bytes())
files={}
for entry in catalog:
    recipe=entry['recipe']
    for filename,hash_key in ((recipe['modelFile'],'meshAsset'),(recipe.get('lodFile'),'editMeshAsset')):
        if not filename:continue
        payload=(root/'starter-models'/filename).read_bytes();digest=hashlib.sha256(payload).hexdigest()
        assert digest==recipe['parameters'][hash_key]
        files[digest+'.json']=payload
        if hash_key=='meshAsset':
            model=json.loads(payload)
            for mat in model.get('materials',[]):
                for channel in ('baseColorTexture','roughnessTexture','normalTexture'):
                    if channel in mat:
                        name=mat[channel]+'.png';data=(root/'starter-models'/name).read_bytes()
                        assert hashlib.sha256(data).hexdigest()==mat[channel];files[name]=data
files['manifest.json']=json.dumps(dict(schema=1,id='contemporary-home',name='Apartamento contemporâneo',version=1),ensure_ascii=False).encode()
files['catalog.json']=json.dumps(catalog,ensure_ascii=False).encode()
args.output.parent.mkdir(parents=True,exist_ok=True)
with zipfile.ZipFile(args.output,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=9) as archive:
    for name,data in sorted(files.items()):archive.writestr(name,data)
print(f'{len(catalog)} models; {args.output.stat().st_size} bytes; SHA256 {hashlib.sha256(args.output.read_bytes()).hexdigest()}')
