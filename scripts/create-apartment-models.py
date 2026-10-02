"""Author LibreMax's original CC0 apartment furniture with Blender (developer tool).

Run: blender --background --factory-startup --python create-apartment-models.py
No external designs, brands or downloaded geometry. Runtime needs no Blender.
Dimensions below are suggested design dimensions in meters, not product claims.
"""
import hashlib
import json
import math
from pathlib import Path

import bpy
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[1]
TARGET = ROOT / 'starter-models'
MATERIALS = {
    'linen': ('Linho areia', [0.66, 0.56, 0.43], .88, 0, 'fabric'),
    'boucle': ('Bouclé creme', [.82, .77, .66], .95, 0, 'fabric'),
    'olive': ('Tecido oliva', [.20, .26, .17], .88, 0, 'fabric'),
    'oak': ('Madeira natural', [.40, .23, .105], .48, 0, 'wood'),
    'walnut': ('Madeira escura', [.16, .074, .036], .43, 0, 'wood'),
    'black': ('Metal grafite', [.035, .04, .047], .31, .75, 'none'),
    'brass': ('Metal champanhe', [.65, .42, .18], .28, .85, 'none'),
    'stone': ('Pedra clara', [.79, .76, .69], .43, 0, 'stone'),
    'mirror': ('Espelho', [.92, .94, .95], .015, 1, 'none'),
    'white': ('Cerâmica clara', [.84, .82, .75], .37, 0, 'none'),
}
CATALOG, PROVENANCE = [], []


def finish(obj, material):
    obj.data.materials.append(bpy.data.materials[material])
    return obj


def box(center, size, material, radius=.025):
    bpy.ops.mesh.primitive_cube_add(size=1, location=center)
    obj = bpy.context.object
    obj.scale = size
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if radius:
        bevel = obj.modifiers.new('Rounded edges', 'BEVEL')
        bevel.width = min(radius, min(size) * .45)
        bevel.segments = 4
        bevel.affect = 'EDGES'
        obj.modifiers.new('Weighted normals', 'WEIGHTED_NORMAL')
    return finish(obj, material)


def cylinder(center, radius, height, material, scale=(1, 1, 1)):
    bpy.ops.mesh.primitive_cylinder_add(vertices=48, radius=radius, depth=height, location=center)
    obj = bpy.context.object
    obj.scale = scale
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    bevel = obj.modifiers.new('Rounded rim', 'BEVEL')
    bevel.width = min(.008, height * .15)
    bevel.segments = 3
    obj.modifiers.new('Weighted normals', 'WEIGHTED_NORMAL')
    for polygon in obj.data.polygons:
        polygon.use_smooth = abs(polygon.normal.z) < .5
    return finish(obj, material)


def rail(start, end, radius, material):
    a, b = Vector(start), Vector(end)
    obj = cylinder((a + b) / 2, radius, (b - a).length, material)
    obj.rotation_euler = (b - a).to_track_quat('Z', 'Y').to_euler()
    return obj


def legs(width, depth, height, material='black', inset=.08):
    for x in (inset, width - inset):
        for y in (inset, depth - inset):
            cylinder((x, y, height / 2), .022, height, material)


def sofa(chaise=False):
    width, depth = (2.60, 1.65) if chaise else (2.05, .93)
    legs(width, .90, .14)
    if chaise:
        cylinder((2.45, 1.51, .07), .022, .14, 'black')
    box((width / 2, .46, .22), (width - .06, .86, .19), 'linen', .07)
    box((width / 2, .105, .60), (width - .06, .21, .58), 'linen', .095)
    for x in (.105, width - .105):
        box((x, .48, .47), (.21, .89, .55), 'linen', .085)
    seat_width = (width - .45) / 3
    for i in range(3):
        x = .225 + seat_width * (i + .5)
        length = 1.36 if chaise and i == 2 else .68
        box((x, .23 + length / 2, .405), (seat_width - .018, length, .20), 'boucle', .085)
        cushion = box((x, .24, .69), (seat_width - .025, .20, .43), 'boucle', .08)
        cushion.rotation_euler.x = math.radians(10)
    cushion = box((.36, .38, .64), (.30, .17, .30), 'olive', .065)
    cushion.rotation_euler.y = math.radians(-15)


def bed():
    legs(1.72, 2.12, .12, 'walnut')
    box((.86, 1.06, .225), (1.72, 2.12, .23), 'linen', .08)
    box((.86, .09, .64), (1.82, .18, 1.28), 'boucle', .08)
    box((.86, 1.11, .46), (1.60, 2.0, .27), 'boucle', .12)
    box((.86, 1.62, .62), (1.61, 1.00, .065), 'olive', .03)
    for x in (.47, 1.25):
        pillow = box((x, .46, .67), (.68, .46, .15), 'linen', .07)
        pillow.rotation_euler.x = math.radians(8)


def headboard():
    box((1.40, .035, .75), (2.80, .07, 1.5), 'walnut', .01)
    for i in range(40):
        box((.035 + i * .07, .085, .75), (.035, .055, 1.5), 'oak', .008)


def nightstand():
    box((.25, .205, .115), (.50, .41, .23), 'walnut', .02)
    box((.25, .42, .115), (.47, .025, .19), 'oak', .01)
    box((.25, .438, .192), (.14, .012, .015), 'brass', .005)


def sideboard(tv=False):
    width, depth, height = (1.80, .38, .52) if tv else (1.40, .42, .80)
    legs(width, depth, .16, 'black')
    box((width / 2, depth / 2, (.16 + height) / 2), (width, depth, height - .16), 'walnut', .025)
    box((width / 2, depth / 2, height), (width + .02, depth + .02, .035), 'stone' if tv else 'oak', .012)
    for i in range(36):
        box(((i + .5) * width / 36, depth + .01, (.18 + height - .04) / 2),
            (.021, .027, height - .22), 'oak', .01)
    for x in (width / 3, 2 * width / 3):
        box((x, depth + .028, (.18 + height - .04) / 2), (.012, .01, height - .23), 'black', .002)


def dining_table():
    cylinder((.525, .525, .035), .29, .07, 'walnut')
    cylinder((.525, .525, .365), .19, .66, 'oak')
    for i in range(28):
        angle = i * math.tau / 28
        cylinder((.525 + .186 * math.cos(angle), .525 + .186 * math.sin(angle), .365), .013, .65, 'oak')
    cylinder((.525, .525, .745), .525, .05, 'stone')


def stool():
    legs(.44, .44, .66, 'black', .055)
    for y in (.055, .385):
        rail((.055, y, .26), (.385, y, .26), .012, 'brass')
    box((.22, .22, .70), (.44, .44, .11), 'boucle', .055)
    box((.22, .035, .89), (.42, .07, .30), 'boucle', .034)


def pouf():
    cylinder((.28, .28, .03), .245, .06, 'walnut')
    cylinder((.28, .28, .235), .28, .39, 'boucle')
    cylinder((.28, .28, .435), .28, .10, 'boucle')


def mirror():
    # An ellipse in the X/Z plane with a thin bronze frame and real mirror surface.
    rim = cylinder((.31, .035, .49), .49, .05, 'brass', (.63265, 1, 1))
    rim.rotation_euler.x = math.pi / 2
    glass = cylinder((.31, .066, .49), .468, .006, 'mirror', (.618, 1, 1))
    glass.rotation_euler.x = math.pi / 2


def floor_lamp():
    cylinder((.20, .20, .025), .20, .05, 'stone')
    points = [( .20 + .84 * (1 - math.cos(t * math.pi / 2)), .20,
                .08 + 1.75 * math.sin(t * math.pi / 2)) for t in [i / 20 for i in range(21)]]
    for a, b in zip(points, points[1:]):
        rail(a, b, .016, 'black')
    # Open shade, rather than a solid cone that blocks its interior.
    x, y, z = points[-1]
    bpy.ops.mesh.primitive_cone_add(vertices=48, radius1=.19, radius2=.075, depth=.19,
                                  end_fill_type='NOTHING', location=(x, y, z - .1))
    shade = finish(bpy.context.object, 'white')
    thickness = shade.modifiers.new('Shade thickness', 'SOLIDIFY')
    thickness.thickness = .005
    for polygon in shade.data.polygons:
        polygon.use_smooth = True


def export_model(asset_id, name, category, placement, elevation, builder):
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    builder()
    graph = bpy.context.evaluated_depsgraph_get()
    groups = {}
    for obj in list(bpy.context.scene.objects):
        if obj.type != 'MESH':
            continue
        evaluated = obj.evaluated_get(graph)
        mesh = evaluated.to_mesh()
        mesh.calc_loop_triangles()
        mat = obj.data.materials[0].name
        group = groups.setdefault(mat, dict(material='lmx-current-' + mat, vertices=[], normals=[], uvs=[], triangles=[]))
        keys = {}
        for triangle in mesh.loop_triangles:
            indices = []
            for loop_index in triangle.loops:
                vertex = mesh.vertices[mesh.loops[loop_index].vertex_index]
                position = obj.matrix_world @ vertex.co
                normal = obj.matrix_world.to_3x3().inverted().transposed() @ mesh.corner_normals[loop_index].vector
                normal.normalize()
                # Native procedural finishes use world coordinates; retain explicit UVs for edits.
                uv = (position.x, position.z)
                key = tuple(round(v, 7) for v in (*position, *normal, *uv))
                if key not in keys:
                    keys[key] = len(group['vertices'])
                    group['vertices'].append(list(position))
                    group['normals'].append(list(normal))
                    group['uvs'].append(list(uv))
                indices.append(keys[key])
            group['triangles'].append(indices)
        evaluated.to_mesh_clear()
    points = [v for part in groups.values() for v in part['vertices']]
    low = [min(v[i] for v in points) for i in range(3)]
    extent = [max(v[i] for v in points) - low[i] for i in range(3)]
    for part in groups.values():
        part['vertices'] = [[round((v[i] - low[i]) / extent[i], 7) for i in range(3)] for v in part['vertices']]
        normals = []
        for normal in part['normals']:
            n = Vector([normal[i] * extent[i] for i in range(3)]).normalized()
            normals.append([round(v, 7) for v in n])
        part['normals'] = normals
        part['uvs'] = [[round(v, 7) for v in uv] for uv in part['uvs']]
    materials = []
    for mat in groups:
        title, color, roughness, metallic, procedural = MATERIALS[mat]
        materials.append(dict(id='lmx-current-' + mat, name=title, baseColor=color,
                              roughness=roughness, metallic=metallic, transmission=0, opacity=1,
                              ior=1.45, procedural=procedural, textureScale=1000))
    payload = json.dumps(dict(schema=1, parts=list(groups.values()), materials=materials), separators=(',', ':')).encode()
    assert len(payload) < 4 * 1024 * 1024, (asset_id, len(payload))
    digest = hashlib.sha256(payload).hexdigest()
    filename = asset_id + '.json'
    (TARGET / filename).write_bytes(payload)
    width, depth, height = [round(value * 1000, 1) for value in extent]
    CATALOG.append(dict(id=asset_id, name=name, category=category, width=width, depth=depth, height=height,
                        tags='apartamento atual moderno contemporâneo compacto original ' + name,
                        license='CC0-1.0', author='LibreMax contributors', origin='LibreMax original design',
                        date='2026-10-01', recipe=dict(type='MeshObject', material='white', modelFile=filename,
                        source='LibreMax original design', author='LibreMax contributors', collection='apartment-modern',
                        parameters=dict(meshAsset=digest, originalMaterials=True, placement=placement,
                                        defaultElevation=elevation))))
    PROVENANCE.append(dict(id=asset_id, license='CC0-1.0', author='LibreMax contributors',
                           generator='scripts/create-apartment-models.py', preparedSha256=digest,
                           dimensionsMillimeters=[width, depth, height], dimensionBasis='suggested',
                           bytes=len(payload), triangles=sum(len(p['triangles']) for p in groups.values())))
    print(f'{asset_id}: {width} x {depth} x {height} mm, {len(payload)} bytes', flush=True)


def main():
    TARGET.mkdir(exist_ok=True)
    for key, (_title, color, roughness, metallic, _procedural) in MATERIALS.items():
        material = bpy.data.materials.new(key)
        material.diffuse_color = (*color, 1)
        material.roughness = roughness
        material.metallic = metallic
    selection = [
        ('current-sofa-compact', 'Sofá compacto de linho e bouclé', 'Sala', 'floor', 0, sofa),
        ('current-sofa-chaise', 'Sofá com chaise e almofadas', 'Sala', 'floor', 0, lambda: sofa(True)),
        ('current-bed-queen', 'Cama queen estofada com roupa de cama', 'Dormitório', 'floor', 0, bed),
        ('current-headboard', 'Cabeceira de madeira ripada', 'Dormitório', 'wall', 0, headboard),
        ('current-nightstand', 'Mesa de cabeceira suspensa', 'Dormitório', 'wall', 450, nightstand),
        ('current-sideboard', 'Aparador compacto com portas ripadas', 'Sala', 'floor', 0, sideboard),
        ('current-tv-console', 'Rack baixo de madeira e pedra', 'Sala', 'floor', 0, lambda: sideboard(True)),
        ('current-dining-table', 'Mesa redonda com base ripada', 'Sala', 'floor', 0, dining_table),
        ('current-bar-stool', 'Banqueta estofada para bancada', 'Cozinha', 'floor', 0, stool),
        ('current-pouf', 'Pufe redondo de bouclé', 'Sala', 'floor', 0, pouf),
        ('current-oval-mirror', 'Espelho oval com moldura champanhe', 'Decoração', 'wall', 950, mirror),
        ('current-floor-lamp', 'Luminária de piso com arco', 'Decoração', 'floor', 0, floor_lamp),
    ]
    for row in selection:
        export_model(*row)
    (TARGET / 'current-catalog.json').write_text(json.dumps(CATALOG, ensure_ascii=False, indent=2), encoding='utf-8')
    (TARGET / 'current-provenance.json').write_text(json.dumps(PROVENANCE, ensure_ascii=False, indent=2), encoding='utf-8')


if __name__ == '__main__':
    main()
