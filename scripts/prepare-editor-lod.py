"""Create distinct editing meshes; full meshes remain unchanged for Cycles."""
import hashlib
import json
from pathlib import Path
import bpy
from mathutils import Vector

root=Path(__file__).resolve().parents[1]/'starter-models'
proof=[]
for catalog_name in ('modern-catalog.json','current-catalog.json','expanded-catalog.json','home-catalog.json'):
    catalog=json.loads((root/catalog_name).read_bytes())
    for asset in catalog:
        recipe=asset['recipe']
        if recipe.get('collection') not in ('apartment-modern','detail-ready'):continue
        full=json.loads((root/recipe['modelFile']).read_bytes())
        triangles=sum(len(p['triangles']) for p in full['parts'])
        if triangles<2500:continue
        extent=[asset['width']/1000,asset['depth']/1000,asset['height']/1000]
        bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
        parts=[]
        for part in full['parts']:
            mesh=bpy.data.meshes.new('Source mesh')
            mesh.from_pydata([[v[i]*extent[i] for i in range(3)] for v in part['vertices']],[],part['triangles'])
            mesh.update()
            if 'uvs' in part:
                uv=mesh.uv_layers.new(name='UVMap')
                for loop in mesh.loops:uv.data[loop.index].uv=part['uvs'][loop.vertex_index]
            for face in mesh.polygons:face.use_smooth=True
            obj=bpy.data.objects.new('Editing mesh',mesh);bpy.context.collection.objects.link(obj)
            modifier=obj.modifiers.new('Editing detail','DECIMATE');modifier.ratio=min(1,1500/triangles);modifier.use_collapse_triangulate=True
            evaluated=obj.evaluated_get(bpy.context.evaluated_depsgraph_get());reduced=evaluated.to_mesh();reduced.calc_loop_triangles()
            out=dict(material=part['material'],vertices=[],triangles=[],normals=[],uvs=[])
            for triangle in reduced.loop_triangles:
                indices=[]
                for index in triangle.loops:
                    loop=reduced.loops[index];v=reduced.vertices[loop.vertex_index].co
                    normal=Vector([reduced.corner_normals[index].vector[i]*extent[i] for i in range(3)]).normalized()
                    if normal.length<.1:normal=Vector((0,0,1))
                    uv=reduced.uv_layers.active.data[index].uv if reduced.uv_layers.active else (v.x,v.y)
                    indices.append(len(out['vertices']));out['vertices'].append([round(max(0,min(1,v[i]/extent[i])),6) for i in range(3)])
                    out['normals'].append([round(n,6) for n in normal]);out['uvs'].append([round(n,6) for n in uv])
                out['triangles'].append(indices)
            if out['triangles']:parts.append(out)
            evaluated.to_mesh_clear()
        low=dict(schema=1,parts=parts,materials=full.get('materials',[]))
        payload=json.dumps(low,separators=(',',':')).encode()
        name='lod-'+recipe['modelFile'];(root/name).write_bytes(payload)
        recipe['lodFile']=name;recipe['parameters']['editMeshAsset']=hashlib.sha256(payload).hexdigest()
        low_count=sum(len(p['triangles']) for p in parts)
        proof.append(dict(id=asset['id'],fullTriangles=triangles,editorTriangles=low_count,fullSha256=recipe['parameters']['meshAsset'],editorSha256=recipe['parameters']['editMeshAsset']))
        print(f"{asset['id']}: {triangles} -> {low_count}",flush=True)
    (root/catalog_name).write_text(json.dumps(catalog,ensure_ascii=False,indent=2),encoding='utf-8')
(root/'editor-lod-provenance.json').write_text(json.dumps(proof,indent=2),encoding='utf-8')
