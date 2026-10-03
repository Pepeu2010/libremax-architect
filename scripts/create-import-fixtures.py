"""Controlled geometry for six actual Blender import paths; not catalog content."""
from pathlib import Path
import sys
import bpy
root=Path(sys.argv[sys.argv.index('--')+1]);root.mkdir(parents=True,exist_ok=True)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
bpy.ops.mesh.primitive_cube_add(size=1)
obj=bpy.context.object;obj.scale=(.5,.7,.9);bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
mat=bpy.data.materials.new('Fixture material');mat.use_nodes=True
shader=mat.node_tree.nodes.get('Principled BSDF');shader.inputs['Roughness'].default_value=.35
image=bpy.data.images.new('Fixture color',width=2,height=2)
image.pixels[:]=[.8,.2,.1,1, .2,.8,.1,1, .2,.1,.8,1, .8,.8,.2,1]
image.filepath_raw=str((root/'color.png').resolve());image.file_format='PNG';image.save()
texture=mat.node_tree.nodes.new('ShaderNodeTexImage');texture.image=image
mat.node_tree.links.new(texture.outputs['Color'],shader.inputs['Base Color']);obj.data.materials.append(mat)
bpy.ops.export_scene.gltf(filepath=str(root/'model.glb'),export_format='GLB')
bpy.ops.export_scene.gltf(filepath=str(root/'model.gltf'),export_format='GLTF_SEPARATE')
bpy.ops.wm.obj_export(filepath=str(root/'model.obj'))
bpy.ops.export_scene.fbx(filepath=str(root/'model.fbx'))
bpy.ops.wm.stl_export(filepath=str(root/'model.stl'))
bpy.ops.wm.ply_export(filepath=str(root/'model.ply'))
