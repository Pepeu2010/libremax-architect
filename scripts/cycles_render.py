"""LibreMax -> Cycles. Invoked by QProcess; no shell or user-generated Python."""
import argparse
import json
import os
import sys
import base64
import hashlib
import bpy
from mathutils import Vector

def arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument('--scene', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--width', type=int, default=1920)
    parser.add_argument('--height', type=int, default=1080)
    parser.add_argument('--samples', type=int, default=128)
    parser.add_argument('--device', choices=('AUTO', 'CPU'), default='AUTO')
    return parser.parse_args(sys.argv[sys.argv.index('--') + 1:])

def main():
    args = arguments()
    with open(args.scene, encoding='utf-8') as stream:
        package = json.load(stream)
    if package['schema'] != 1:
        raise ValueError('Unsupported scene schema')
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    scene.cycles.samples = args.samples
    scene.cycles.use_denoising = True
    scene.cycles.device = 'CPU'
    if args.device == 'AUTO':
        prefs = bpy.context.preferences.addons['cycles'].preferences
        for backend in ('OPTIX', 'CUDA', 'HIP', 'ONEAPI'):
            try:
                prefs.compute_device_type = backend
                prefs.refresh_devices()
                devices = [device for device in prefs.devices if device.type != 'CPU']
                if devices:
                    for device in prefs.devices:
                        device.use = device.type != 'CPU'
                    scene.cycles.device = 'GPU'
                    print('LIBREMAX_DEVICE', backend, [device.name for device in devices], flush=True)
                    break
            except (TypeError, RuntimeError):
                continue
    print('LIBREMAX_RENDER_DEVICE', scene.cycles.device, flush=True)
    materials = {}
    texture_paths = {}
    for digest, encoded in package.get('assets', {}).items():
        data = base64.b64decode(encoded, validate=True)
        if hashlib.sha256(data).hexdigest() != digest or len(digest) != 64 or any(c not in '0123456789abcdef' for c in digest):
            raise ValueError('Invalid asset hash')
        texture_path = os.path.join(os.path.dirname(args.scene), digest + '.png')
        with open(texture_path, 'wb') as stream:
            stream.write(data)
        texture_paths[digest] = texture_path
    for entry in package['materials']:
        material = bpy.data.materials.new(entry['name'])
        material.use_nodes = True
        shader = material.node_tree.nodes.get('Principled BSDF')
        shader.inputs['Base Color'].default_value = (*entry['baseColor'], entry.get('opacity', 1.0))
        shader.inputs['Roughness'].default_value = entry.get('roughness', 0.5)
        shader.inputs['Metallic'].default_value = entry.get('metallic', 0.0)
        shader.inputs['Transmission Weight'].default_value = entry.get('transmission', 0.0)
        shader.inputs['IOR'].default_value = entry.get('ior', 1.45)
        if entry.get('baseColorTexture'):
            texture = material.node_tree.nodes.new('ShaderNodeTexImage')
            texture.image = bpy.data.images.load(texture_paths[entry['baseColorTexture']], check_existing=True)
            texture.projection = 'BOX'
            texture.projection_blend = 0.15
            coordinate = material.node_tree.nodes.new('ShaderNodeNewGeometry')
            mapping = material.node_tree.nodes.new('ShaderNodeMapping')
            scale = 1000.0 / entry.get('textureScale', 1000.0)
            mapping.inputs['Scale'].default_value = (scale, scale, scale)
            import math
            mapping.inputs['Rotation'].default_value.z = math.radians(entry.get('textureRotation', 0))
            mapping.inputs['Location'].default_value = [v / 1000 for v in entry.get('textureOffset', [0, 0, 0])]
            material.node_tree.links.new(coordinate.outputs['Position'], mapping.inputs['Vector'])
            material.node_tree.links.new(mapping.outputs['Vector'], texture.inputs['Vector'])
            material.node_tree.links.new(texture.outputs['Color'], shader.inputs['Base Color'])
        materials[entry['id']] = material
    for index, part in enumerate(package['meshes']):
        mesh = bpy.data.meshes.new(f'mesh-{index}')
        mesh.from_pydata(part['vertices'], [], part['triangles'])
        mesh.update()
        obj = bpy.data.objects.new(part['owner'], mesh)
        scene.collection.objects.link(obj)
        obj.data.materials.append(materials[part['material']])
    for entry in package['lights']:
        p = entry['parameters']
        kind = p.get('kind', 'area').upper()
        light = bpy.data.lights.new(entry['name'], kind)
        light.energy = p.get('power', 500)
        light.color = p.get('color', [1.0, 0.89, 0.73])
        if kind == 'AREA':
            light.shape = 'DISK'
            light.size = p.get('size', 1000) / 1000
        elif kind == 'SPOT':
            import math
            light.spot_size = math.radians(p.get('angle', 45))
            light.spot_blend = p.get('blend', 0.3)
        obj = bpy.data.objects.new(entry['name'], light)
        scene.collection.objects.link(obj)
        obj.location = entry['position']
        target = Vector([v / 1000 for v in p.get('target', [2000, 1500, 0])])
        obj.rotation_euler = (target - obj.location).to_track_quat('-Z', 'Y').to_euler()
    camera_entry = package['cameras'][0] if package['cameras'] else None
    if camera_entry is None:
        raise ValueError('Create a persistent camera before rendering')
    camera = bpy.data.cameras.new(camera_entry['name'])
    camera.lens = camera_entry['parameters'].get('lens', 28)
    obj = bpy.data.objects.new(camera_entry['name'], camera)
    scene.collection.objects.link(obj)
    obj.location = camera_entry['position']
    target = Vector([v / 1000 for v in camera_entry['parameters'].get('target', [2000, 1500, 1000])])
    obj.rotation_euler = (target - obj.location).to_track_quat('-Z', 'Y').to_euler()
    scene.camera = obj
    scene.world.use_nodes = True
    scene.world.node_tree.nodes['Background'].inputs['Color'].default_value = (0.7, 0.8, 1.0, 1.0)
    scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value = 0.25
    scene.render.resolution_x = args.width
    scene.render.resolution_y = args.height
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = 'JPEG' if args.output.lower().endswith(('.jpg', '.jpeg')) else 'PNG'
    scene.render.filepath = os.path.abspath(args.output)
    scene.render.use_file_extension = False
    bpy.ops.wm.save_as_mainfile(filepath=os.path.join(os.path.dirname(args.scene), 'scene.blend'))
    bpy.ops.render.render(write_still=True)
    print('LIBREMAX_COMPLETED', scene.render.filepath, flush=True)

if __name__ == '__main__':
    main()
