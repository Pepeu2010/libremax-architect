"""Controlled acceptance fixture; production reader must validate the resulting container."""
import copy
import json
import uuid
import zipfile

with zipfile.ZipFile('examples/apartamento-moderno.lmx') as source:
    files = {name: source.read(name) for name in source.namelist()}
scene = json.loads(files['scene.json'])
project = json.loads(files['project.json'])
manifest = json.loads(files['manifest.json'])
with zipfile.ZipFile('build-lighting/native-release/led-render.lmx') as source:
    light = copy.deepcopy(next(e for e in json.loads(source.read('scene.json'))['entities']
                               if e['type'] == 'Light'))
light['uuid'] = str(uuid.uuid4())
light['name'] = 'LED de teto — 3000 K'
light['transform'].update(x=1200, y=2200, z=2600, yaw=90)
light['parameters'].update(target=[1200, 2200, 0], power=20, size=2000, sizeY=15)
scene['entities'].append(light)
for entity in scene['entities']:
    if entity['type'] == 'Light' and entity['parameters']['kind'] == 'area':
        entity['parameters'].update(colorMode='kelvin', temperature=4000)
project['version'] = manifest['version'] = 3
project['name'] = 'Apartamento — Kelvin e LED'
project['renderSettings']['cycles'] = dict(version=1, preset='final', samples=512,
    width=3840, height=2160, maxBounces=12, diffuseBounces=12, glossyBounces=12,
    transmissionBounces=12, transparentBounces=12, denoise=True, device='AUTO',
    clamp=5.0, noiseThreshold=0.008, format='PNG', transparent=False)
for name, data in [('scene.json', scene), ('project.json', project), ('manifest.json', manifest),
                   ('lighting.json', [e for e in scene['entities'] if e['type'] == 'Light'])]:
    files[name] = json.dumps(data, ensure_ascii=False, separators=(',', ':')).encode('utf-8')
with zipfile.ZipFile('build-lighting/apartamento-iluminado.lmx', 'w', zipfile.ZIP_DEFLATED) as output:
    for name, payload in files.items():
        output.writestr(name, payload)
print('4K lighting fixture prepared; actual application validation pending')
