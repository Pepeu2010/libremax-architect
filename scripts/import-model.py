"""Trusted Blender bridge. Import geometry without running scripts from a model.

blender --background --factory-startup --disable-autoexec --python-exit-code 1
        --python import-model.py -- INPUT OUTPUT_DIRECTORY
"""
import hashlib
import json
import math
from pathlib import Path
import sys
import bpy
from mathutils import Vector

source, target = map(Path, sys.argv[sys.argv.index('--') + 1:])
target.mkdir(parents=True, exist_ok=True)
if source.stat().st_size > 128 * 1024 * 1024:
    raise ValueError('O arquivo precisa ter menos de 128 MB.')
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
suffix = source.suffix.lower()
if suffix == '.gltf':
    data = json.loads(source.read_text(encoding='utf-8'))
    for item in data.get('buffers', []) + data.get('images', []):
        uri = item.get('uri', '')
        if uri and not uri.startswith('data:'):
            path = Path(uri)
            if path.is_absolute() or '..' in path.parts or ':' in uri or uri.startswith('/'):
                raise ValueError('Use arquivos e texturas da mesma pasta do modelo.')
importers = {
    '.gltf': bpy.ops.import_scene.gltf, '.glb': bpy.ops.import_scene.gltf,
    '.obj': bpy.ops.wm.obj_import, '.fbx': bpy.ops.import_scene.fbx,
    '.stl': bpy.ops.wm.stl_import, '.ply': bpy.ops.wm.ply_import,
}
if suffix not in importers:
    raise ValueError('Formato de modelo não suportado.')
importers[suffix](filepath=str(source.resolve()))
objects = [o for o in bpy.context.scene.objects if o.type == 'MESH']
if not objects or len(objects) > 512:
    raise ValueError('O arquivo não contém um modelo utilizável ou tem peças demais.')
original_triangles = sum(sum(max(0, len(p.vertices)-2) for p in o.data.polygons) for o in objects)
if original_triangles > 1000000:
    raise ValueError('O modelo tem mais de um milhão de faces. Exporte uma versão mais leve.')
print('LMX_IMPORT: preparing', flush=True)
# Bound the portable payload, while retaining smooth normals and material separation.
if original_triangles > 16000:
    for obj in objects:
        modifier = obj.modifiers.new('Portable model', 'DECIMATE')
        modifier.ratio = 16000 / original_triangles
        modifier.use_collapse_triangulate = True
groups, materials, textures = {}, {}, {}
prefix = 'import-' + hashlib.sha256(source.read_bytes()).hexdigest()[:16]

def image_map(image, channel=None):
    if image is None:
        return None
    key = image.name + str(channel)
    if key in textures:
        return textures[key]
    # glTF images may be packed but lazily decoded until pixels are requested.
    width,height=image.size
    if width<=0 or height<=0 or width*height>64000000:
        return None
    if len(image.pixels)==0:
        return None
    copy = image.copy()
    try:
        w, h = copy.size
        if w <= 0 or h <= 0 or w * h > 64000000:
            return None
        scale = min(1, 1024 / max(w, h))
        copy.scale(max(1, int(w*scale)), max(1, int(h*scale)))
        if channel is not None:
            pixels=list(copy.pixels[:])
            for index in range(0,len(pixels),4):
                pixels[index:index+3]=[pixels[index+channel]]*3
            copy.pixels[:]=pixels
        path = target / 'temporary.png'
        copy.file_format = 'PNG'
        copy.filepath_raw = str(path.resolve())
        copy.save()
        data = path.read_bytes()
        digest = hashlib.sha256(data).hexdigest()
        path.replace(target / (digest + '.png'))
        textures[key] = digest
        return digest
    finally:
        bpy.data.images.remove(copy)

def material_entry(mat):
    key = mat.name if mat else 'Sem acabamento'
    if key in materials:
        return materials[key]['id']
    mid = prefix + '-' + str(len(materials))
    color = list(mat.diffuse_color[:3]) if mat else [.65, .65, .65]
    roughness, metallic = (mat.roughness, mat.metallic) if mat else (.6, 0)
    entry = dict(id=mid, name=key[:512], baseColor=color, roughness=roughness,
                 metallic=metallic, transmission=0, opacity=1, ior=1.45)
    if mat and mat.use_nodes:
        nodes = [n for n in mat.node_tree.nodes if n.type == 'BSDF_PRINCIPLED']
        if nodes:
            shader = nodes[0]
            entry.update(baseColor=list(shader.inputs['Base Color'].default_value[:3]),
                         roughness=shader.inputs['Roughness'].default_value,
                         metallic=shader.inputs['Metallic'].default_value,
                         transmission=shader.inputs['Transmission Weight'].default_value,
                         ior=max(1, min(3, shader.inputs['IOR'].default_value)))
            # Preserve directly connected color maps; complex node networks need baking first.
            links = shader.inputs['Base Color'].links
            if links and links[0].from_node.type == 'TEX_IMAGE':
                digest = image_map(links[0].from_node.image)
                if digest:
                    entry.update(baseColorTexture=digest, baseColor=[1,1,1], modelUV=True)
            rough = shader.inputs['Roughness'].links
            if rough:
                node=rough[0].from_node
                channel=None
                if node.type in ('SEPRGB','SEPARATE_COLOR'):
                    channel={'Red':0,'Green':1,'Blue':2,'R':0,'G':1,'B':2}.get(rough[0].from_socket.name)
                    connected=node.inputs[0].links
                    node=connected[0].from_node if connected else None
                if node and node.type=='TEX_IMAGE':
                    digest=image_map(node.image,channel)
                    if digest:entry.update(roughnessTexture=digest,modelUV=True)
            normal=shader.inputs['Normal'].links
            if normal and normal[0].from_node.type=='NORMAL_MAP':
                node=normal[0].from_node
                connected=node.inputs['Color'].links
                if connected and connected[0].from_node.type=='TEX_IMAGE':
                    digest=image_map(connected[0].from_node.image)
                    if digest:entry.update(normalTexture=digest,normalStrength=min(2,max(0,node.inputs['Strength'].default_value)),modelUV=True)
    materials[key] = entry
    return mid

depsgraph = bpy.context.evaluated_depsgraph_get()
for obj in objects:
    evaluated = obj.evaluated_get(depsgraph)
    mesh = evaluated.to_mesh()
    mesh.calc_loop_triangles()
    transform = obj.matrix_world
    normal_transform = transform.to_3x3().inverted_safe().transposed()
    for triangle in mesh.loop_triangles:
        mat = obj.data.materials[triangle.material_index] if triangle.material_index < len(obj.data.materials) else None
        mid = material_entry(mat)
        group = groups.setdefault(mid, dict(material=mid, vertices=[], triangles=[], normals=[], uvs=[]))
        indices = []
        for loop in triangle.loops:
            vertex = transform @ mesh.vertices[mesh.loops[loop].vertex_index].co
            normal = (normal_transform @ mesh.corner_normals[loop].vector).normalized()
            if normal.length < .1:
                normal = Vector((0,0,1))
            uv = mesh.uv_layers.active.data[loop].uv[:] if mesh.uv_layers.active else (vertex.x,vertex.y)
            indices.append(len(group['vertices']))
            group['vertices'].append(list(vertex)); group['normals'].append(list(normal)); group['uvs'].append(list(uv))
        if transform.determinant() < 0:
            indices.reverse()
        group['triangles'].append(indices)
    evaluated.to_mesh_clear()
if len(groups) > 64:
    raise ValueError('O modelo usa mais de 64 acabamentos. Junte os acabamentos semelhantes.')
points = [p for group in groups.values() for p in group['vertices']]
if not points or any(not math.isfinite(v) for p in points for v in p):
    raise ValueError('A geometria do modelo é inválida.')
low = [min(p[i] for p in points) for i in range(3)]
extent = [max(p[i] for p in points)-low[i] for i in range(3)]
if min(extent) < .00001:
    raise ValueError('O modelo precisa ter largura, comprimento e altura.')
for group in groups.values():
    group['vertices'] = [[round((p[i]-low[i])/extent[i],6) for i in range(3)] for p in group['vertices']]
    group['normals'] = [[round(v,6) for v in Vector([n[i]*extent[i] for i in range(3)]).normalized()] for n in group['normals']]
    group['uvs'] = [[round(v,6) for v in uv] for uv in group['uvs']]
payload = json.dumps(dict(schema=1,parts=list(groups.values()),materials=list(materials.values())),separators=(',',':'),ensure_ascii=False).encode()
if len(payload) > 4*1024*1024:
    raise ValueError('O modelo convertido ficou grande demais. Exporte uma versão mais leve.')
(target/'model.json').write_bytes(payload)
(target/'result.json').write_text(json.dumps(dict(dimensions=[v*1000 for v in extent],
    originalTriangles=original_triangles,triangles=sum(len(g['triangles']) for g in groups.values()),
    simplified=original_triangles>16000)),encoding='utf-8')
print('LMX_IMPORT: complete',flush=True)
