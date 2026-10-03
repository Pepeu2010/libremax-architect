"""Prepare CC0 furniture offline; preserve the existing catalogs and IDs.

KayKit source must be the official repository at the pinned commit below.
Poly Haven downloads use its original checksums. Runtime needs no network.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
COMMIT = '96d5930a8dbdb363409bbc2d3341718b00e17c9c'
SOURCE = 'https://github.com/KayKit-Game-Assets/KayKit-Furniture-Bits-1.0'
NAMES = {
    'armchair': 'Poltrona com braços', 'armchair_pillows': 'Poltrona com almofadas',
    'bed_double_A': 'Cama de casal com cabeceira', 'bed_double_B': 'Cama de casal baixa',
    'bed_single_A': 'Cama de solteiro com cabeceira', 'bed_single_B': 'Cama de solteiro baixa',
    'book_set': 'Livros empilhados', 'book_single': 'Livro fechado',
    'cabinet_medium': 'Armário médio', 'cabinet_medium_decorated': 'Armário médio decorado',
    'cabinet_small': 'Armário pequeno', 'cabinet_small_decorated': 'Armário pequeno decorado',
    'cactus_medium_A': 'Cacto alto em vaso', 'cactus_medium_B': 'Cacto ramificado em vaso',
    'cactus_small_A': 'Cacto pequeno redondo', 'cactus_small_B': 'Cacto pequeno ramificado',
    'chair_A': 'Cadeira estofada com braços', 'chair_A_wood': 'Cadeira de madeira com braços',
    'chair_B': 'Cadeira estofada reta', 'chair_B_wood': 'Cadeira de madeira reta',
    'chair_C': 'Cadeira com encosto aberto', 'chair_stool': 'Banqueta com assento estofado',
    'chair_stool_wood': 'Banqueta com assento de madeira', 'couch': 'Sofá de dois lugares',
    'couch_pillows': 'Sofá de dois lugares com almofadas',
    'lamp_standing': 'Luminária de piso com cúpula', 'lamp_table': 'Abajur com cúpula',
    'pictureframe_large_A': 'Quadro grande de paisagem', 'pictureframe_large_B': 'Quadro grande vertical',
    'pictureframe_medium': 'Quadro médio', 'pictureframe_small_A': 'Quadro pequeno horizontal',
    'pictureframe_small_B': 'Quadro pequeno vertical', 'pictureframe_small_C': 'Quadro pequeno quadrado',
    'pictureframe_standing_A': 'Porta-retrato horizontal', 'pictureframe_standing_B': 'Porta-retrato vertical',
    'pillow_A': 'Almofada quadrada', 'pillow_B': 'Almofada alongada',
    'rug_oval_A': 'Tapete oval claro', 'rug_oval_B': 'Tapete oval colorido',
    'rug_rectangle_A': 'Tapete retangular claro', 'rug_rectangle_B': 'Tapete retangular colorido',
    'rug_rectangle_stripes_A': 'Tapete com listras claras', 'rug_rectangle_stripes_B': 'Tapete com listras coloridas',
    'shelf_A_big': 'Prateleira longa aberta', 'shelf_A_small': 'Prateleira curta aberta',
    'shelf_B_large': 'Prateleira longa com nichos', 'shelf_B_large_decorated': 'Prateleira longa com livros e objetos',
    'shelf_B_small': 'Prateleira curta com nichos', 'shelf_B_small_decorated': 'Prateleira curta com livros e objetos',
    'table_low': 'Mesa de centro baixa', 'table_medium_long': 'Mesa de jantar alongada',
    'table_medium': 'Mesa de jantar quadrada', 'table_small': 'Mesa lateral quadrada',
}
DETAILED = [
    ('ceramic_vase_01', 'Vaso de cerâmica com textura natural', 'surface'),
    ('ceramic_vase_04', 'Vaso de cerâmica com alças', 'surface'),
    ('wall_clock', 'Relógio de parede detalhado', 'wall'),
    ('alarm_clock_01', 'Relógio de mesa detalhado', 'surface'),
    ('wooden_bowl_01', 'Tigela de madeira natural', 'surface'),
    ('wooden_bowl_02', 'Tigela de madeira arredondada', 'surface'),
    ('standing_picture_frame_01', 'Porta-retrato de madeira detalhado', 'surface'),
]

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('kaykit', type=Path)
    parser.add_argument('--cache', type=Path, default=ROOT/'build-model-expansion/polyhaven')
    args = parser.parse_args()
    actual = subprocess.check_output(['git', '-C', str(args.kaykit), 'rev-parse', 'HEAD'], text=True).strip()
    if actual != COMMIT:
        raise ValueError('KayKit revision differs from the reviewed CC0 source')
    spec = importlib.util.spec_from_file_location('prepare_modern', ROOT/'scripts/prepare-modern-models.py')
    converter = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(converter)
    target = ROOT/'starter-models'
    folder = args.kaykit/'addons/kaykit_furniture_bits/Assets/gltf'
    assert {p.stem for p in folder.glob('*.gltf')} == set(NAMES)
    catalog, provenance = [], []
    def add(path, identifier, name, category, placement, author, origin, sources, collection):
        payload, dimensions = converter.prepare_model(path, identifier)
        width, depth, height = dimensions
        if collection == 'lightweight-ready':
            stem=identifier.removeprefix('kaykit-')
            if stem.startswith('bed_'): width,depth,height=(1600 if 'double' in stem else 900),2100,850
            elif stem.startswith('armchair'): width,depth,height=850,900,850
            elif stem.startswith('couch'): width,depth,height=2050,900,850
            elif stem.startswith('chair_stool'): width,depth,height=400,400,500
            elif stem.startswith('chair_'): width,depth,height=500,500,850
            elif stem.startswith('cabinet_'): width,depth,height=(1000 if 'medium' in stem else 500),400,650
            elif stem.startswith('shelf_'): width,depth,height=(1200 if any(x in stem for x in ['big','large']) else 600),250,300
            elif stem=='table_low': width,depth,height=1100,650,420
            elif stem=='table_medium_long': width,depth,height=1800,900,750
            elif stem=='table_medium': width,depth,height=1000,1000,750
            elif stem=='table_small': width,depth,height=450,450,500
            elif stem=='lamp_standing': width,depth,height=450,450,1600
            elif stem=='lamp_table': width,depth,height=280,280,500
            else:
                longest=1800 if stem.startswith('rug_') else 450 if stem.startswith('pillow_') else 650 if stem.startswith('pictureframe_large') else 400 if stem.startswith(('pictureframe_medium','cactus_medium')) else 250
                scale=longest/max(width,depth,height)
                width,depth,height=[round(value*scale,1) for value in dimensions]
        # Thin rugs are modelled at the floor plane in the source; use a practical thickness.
        if placement == 'rug': height = max(8, height)
        filename = identifier+'.json'
        digest = hashlib.sha256(payload).hexdigest()
        (target/filename).write_bytes(payload)
        triangles = sum(len(p['triangles']) for p in json.loads(payload)['parts'])
        catalog.append(dict(id=identifier, name=name, category=category, width=width, depth=depth, height=height,
            tags=f'{name} {author} pronto '+('leve estilizado' if collection=='lightweight-ready' else 'realista detalhado'),
            license='CC0-1.0', author=author, origin=origin, date='2026-10-03',
            recipe=dict(type='MeshObject', material='white', modelFile=filename, source=origin, author=author,
                collection=collection, parameters=dict(meshAsset=digest, originalMaterials=True,
                placement=placement, defaultElevation=1200 if placement=='wall' else 0))))
        provenance.append(dict(id=identifier, license='CC0-1.0', author=author, source=origin,
            revision=COMMIT if collection=='lightweight-ready' else None, sources=sources,
            preparedSha256=digest, bytes=len(payload), triangles=triangles,
            dimensionsMillimeters=[width, depth, height], sourceBoundsMillimeters=dimensions,
            dimensionBasis='suggested apartment size' if collection=='lightweight-ready' else 'source bounds'))
        print(identifier, dimensions, triangles, len(payload), flush=True)
    for stem, name in NAMES.items():
        path = folder/(stem+'.gltf')
        definition=json.loads(path.read_text())
        sources=[]
        for local in [path, *[folder/b['uri'] for b in definition['buffers']],
                      *[folder/i['uri'] for i in definition.get('images',[])]]:
            relative=local.resolve().relative_to(args.kaykit.resolve()).as_posix()
            sources.append(dict(url=f'{SOURCE}/blob/{COMMIT}/{relative}',
                sha256=hashlib.sha256(local.read_bytes()).hexdigest(), bytes=local.stat().st_size))
        category='Dormitório' if stem.startswith('bed_') else 'Decoração' if stem.startswith(
            ('book_', 'cactus_', 'lamp_', 'pictureframe_', 'pillow_', 'rug_')) else 'Sala'
        placement='wall' if stem.startswith('shelf_') or (stem.startswith('pictureframe_') and 'standing' not in stem) else 'rug' if stem.startswith('rug_') else 'surface' if stem.startswith(
            ('book_', 'pillow_', 'cactus_small', 'pictureframe_standing', 'lamp_table')) else 'floor'
        add(path, 'kaykit-'+stem, name+' · KayKit', category, placement, 'Kay Lousberg', SOURCE,
            sources, 'lightweight-ready')
    converter.download('https://api.polyhaven.com/assets?type=models', args.cache/'assets.json')
    metadata=json.loads((args.cache/'assets.json').read_text())
    for asset, name, placement in DETAILED:
        path, sources=converter.source_files(asset,args.cache)
        author=', '.join(metadata[asset].get('authors',{}))
        add(path, 'detail-'+asset, name, 'Decoração', placement, author,
            'https://polyhaven.com/a/'+asset,sources,'detail-ready')
    (target/'expanded-catalog.json').write_text(json.dumps(catalog,ensure_ascii=False,indent=2),encoding='utf-8')
    (target/'expanded-provenance.json').write_text(json.dumps(provenance,ensure_ascii=False,indent=2),encoding='utf-8')
    license_text=(args.kaykit/'LICENSE.txt').read_text(encoding='utf-8')
    (target/'KAYKIT_LICENSE.txt').write_text('\n'.join(line.rstrip() for line in license_text.splitlines())+'\n',encoding='utf-8')
    print('Prepared',len(catalog),'models')

if __name__=='__main__': main()
