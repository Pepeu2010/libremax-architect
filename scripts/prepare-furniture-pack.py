"""Offline preparation of licensed OBJ furniture; no runtime network dependency.

Usage: python scripts/prepare-furniture-pack.py <extracted Kenney Furniture Kit>
The distributed catalog contains normalized meshes, dimensions and provenance.
"""
import argparse
import hashlib
import json
from pathlib import Path

SELECTION = [
    ('chair', 'Cadeira de madeira', 'Sala', 450, 850, 450, 'floor'),
    ('chairCushion', 'Cadeira estofada', 'Sala', 480, 850, 480, 'floor'),
    ('chairModernCushion', 'Cadeira moderna', 'Sala', 480, 800, 480, 'floor'),
    ('chairRounded', 'Cadeira com encosto curvo', 'Sala', 480, 820, 480, 'floor'),
    ('stoolBar', 'Banqueta alta', 'Sala', 420, 1050, 420, 'floor'),
    ('loungeChair', 'Poltrona confortável', 'Sala', 850, 850, 850, 'floor'),
    ('loungeChairRelax', 'Poltrona de descanso', 'Sala', 850, 900, 950, 'floor'),
    ('loungeDesignChair', 'Poltrona de design', 'Sala', 850, 800, 850, 'floor'),
    ('loungeSofa', 'Sofá compacto pronto', 'Sala', 1700, 850, 850, 'floor'),
    ('loungeSofaLong', 'Sofá amplo pronto', 'Sala', 2300, 850, 900, 'floor'),
    ('loungeSofaCorner', 'Sofá de canto pronto', 'Sala', 2300, 850, 1650, 'floor'),
    ('loungeDesignSofa', 'Sofá moderno pronto', 'Sala', 2100, 800, 900, 'floor'),
    ('loungeSofaOttoman', 'Pufe estofado', 'Sala', 700, 420, 700, 'floor'),
    ('table', 'Mesa de jantar pronta', 'Sala', 1600, 750, 850, 'floor'),
    ('tableRound', 'Mesa redonda', 'Sala', 1100, 750, 1100, 'floor'),
    ('tableGlass', 'Mesa com tampo de vidro', 'Sala', 1600, 750, 850, 'floor'),
    ('tableCoffee', 'Mesa de centro', 'Sala', 1000, 420, 600, 'floor'),
    ('tableCoffeeGlassSquare', 'Mesa de centro com vidro', 'Sala', 700, 420, 700, 'floor'),
    ('cabinetTelevision', 'Rack para televisão', 'Sala', 1600, 450, 450, 'floor'),
    ('bookcaseOpen', 'Estante de livros', 'Sala', 900, 1800, 350, 'floor'),
    ('bedDouble', 'Cama de casal pronta', 'Dormitório', 1600, 950, 2100, 'floor'),
    ('bedSingle', 'Cama de solteiro pronta', 'Dormitório', 950, 950, 2100, 'floor'),
    ('bedBunk', 'Beliche', 'Dormitório', 950, 1800, 2100, 'floor'),
    ('cabinetBedDrawer', 'Mesa de cabeceira pronta', 'Dormitório', 500, 550, 450, 'floor'),
    ('desk', 'Escrivaninha pronta', 'Escritório', 1400, 750, 650, 'floor'),
    ('deskCorner', 'Escrivaninha de canto', 'Escritório', 1500, 750, 1200, 'floor'),
    ('chairDesk', 'Cadeira de escritório', 'Escritório', 650, 1100, 650, 'floor'),
    ('laptop', 'Notebook aberto', 'Escritório', 340, 260, 280, 'surface'),
    ('computerScreen', 'Monitor', 'Escritório', 550, 420, 200, 'surface'),
    ('computerKeyboard', 'Teclado', 'Escritório', 450, 25, 160, 'surface'),
    ('kitchenCoffeeMachine', 'Cafeteira', 'Cozinha', 300, 400, 350, 'surface'),
    ('kitchenBlender', 'Liquidificador', 'Cozinha', 220, 420, 200, 'surface'),
    ('toaster', 'Torradeira', 'Cozinha', 300, 200, 180, 'surface'),
    ('kitchenMicrowave', 'Micro-ondas', 'Eletrodomésticos', 550, 350, 420, 'surface'),
    ('kitchenFridge', 'Geladeira pronta', 'Eletrodomésticos', 700, 1800, 650, 'floor'),
    ('kitchenStove', 'Fogão pronto', 'Eletrodomésticos', 600, 850, 600, 'floor'),
    ('washer', 'Máquina de lavar', 'Eletrodomésticos', 600, 850, 600, 'floor'),
    ('dryer', 'Secadora', 'Eletrodomésticos', 600, 850, 600, 'floor'),
    ('bathtub', 'Banheira', 'Banheiro', 1700, 600, 750, 'floor'),
    ('toilet', 'Vaso sanitário', 'Banheiro', 400, 800, 650, 'floor'),
    ('bathroomSink', 'Pia com bancada', 'Banheiro', 800, 850, 550, 'floor'),
    ('shower', 'Box de banho', 'Banheiro', 900, 2100, 900, 'floor'),
    ('bathroomMirror', 'Espelho de banheiro', 'Banheiro', 700, 800, 50, 'wall'),
    ('pottedPlant', 'Planta decorativa pronta', 'Decoração', 500, 1300, 500, 'floor'),
    ('plantSmall1', 'Planta pequena', 'Decoração', 220, 300, 220, 'surface'),
    ('plantSmall2', 'Suculenta', 'Decoração', 200, 250, 200, 'surface'),
    ('books', 'Conjunto de livros', 'Decoração', 300, 250, 200, 'surface'),
    ('lampRoundFloor', 'Luminária de piso', 'Decoração', 500, 1700, 500, 'floor'),
    ('lampRoundTable', 'Abajur', 'Decoração', 300, 500, 300, 'surface'),
    ('televisionModern', 'Televisão', 'Decoração', 1200, 720, 180, 'surface'),
    ('rugRectangle', 'Tapete de sala pronto', 'Decoração', 2400, 8, 1700, 'rug'),
    ('rugRound', 'Tapete redondo', 'Decoração', 1800, 8, 1800, 'rug'),
]

def convert(path):
    vertices, faces, material = [], {}, 'white'
    for line in path.read_text().splitlines():
        fields = line.split()
        if not fields:
            continue
        if fields[0] == 'v':
            x, y, z = map(float, fields[1:4])
            vertices.append((x, -z, y))
        elif fields[0] == 'usemtl':
            material = fields[1]
        elif fields[0] == 'f':
            indices = [int(i.split('/')[0]) - 1 for i in fields[1:]]
            for i in range(1, len(indices) - 1):
                faces.setdefault(material, []).append([indices[0], indices[i], indices[i + 1]])
    lo = [min(v[i] for v in vertices) for i in range(3)]
    extent = [max(v[i] for v in vertices) - lo[i] for i in range(3)]
    assert min(extent) > 0
    points = [[round((v[i] - lo[i]) / extent[i], 7) for i in range(3)] for v in vertices]
    parts, seen_faces = [], set()
    for material, triangles in faces.items():
        clean = []
        for triangle in triangles:
            key = tuple(sorted(tuple(points[i]) for i in triangle))
            if len(set(key)) < 3 or key in seen_faces:
                continue
            seen_faces.add(key)
            clean.append(triangle)
        triangles = clean
        if not triangles:
            continue
        key = material.lower()
        mapped = ('glass' if 'glass' in key else 'oak' if 'wood' in key else
                  'metal' if 'metal' in key else 'graphite' if 'black' in key or 'dark' in key else
                  'fabric' if 'blue' in key or 'green' in key or 'cloth' in key or 'red' in key or key=='plant' or key=='carpet' else 'white')
        used = sorted({i for triangle in triangles for i in triangle})
        index = {old: new for new, old in enumerate(used)}
        parts.append(dict(material=mapped, vertices=[points[i] for i in used],
                          triangles=[[index[i] for i in triangle] for triangle in triangles]))
    return json.dumps(dict(schema=1, parts=parts), separators=(',', ':')).encode()

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('source', type=Path)
    args = parser.parse_args()
    target = Path(__file__).resolve().parents[1] / 'starter-models'
    target.mkdir(exist_ok=True)
    catalog = []
    for filename, name, category, width, height, depth, placement in SELECTION:
        source = args.source / 'Models' / 'OBJ format' / (filename + '.obj')
        data = convert(source)
        (target / (filename + '.json')).write_bytes(data)
        catalog.append(dict(id='ready-' + filename, name=name, category=category, width=width, height=height, depth=depth,
                            tags=f'{category} {name} pronto Kenney', license='CC0-1.0', author='Kenney',
                            origin='https://kenney.nl/assets/furniture-kit', date='2026-09-30',
                            recipe=dict(type='MeshObject', material='white', modelFile=filename + '.json',
                                        source='https://kenney.nl/assets/furniture-kit',
                                        parameters=dict(meshAsset=hashlib.sha256(data).hexdigest(), originalMaterials=True,
                                                        placement=placement, defaultElevation=1200 if placement=='wall' else 0))))
    (target / 'catalog.json').write_text(json.dumps(catalog, ensure_ascii=False, indent=2), encoding='utf-8')
    license_text = (args.source / 'License.txt').read_text(encoding='utf-8')
    (target / 'LICENSE.txt').write_text(''.join(line.rstrip() + '\n' for line in license_text.splitlines()),
                                      encoding='utf-8')
    print(f'{len(catalog)} real models prepared')

if __name__ == '__main__':
    main()
