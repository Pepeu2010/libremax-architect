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

def surface_detail(entry, material, shader):
    """Original procedural surfaces; no downloaded images or proprietary assets."""
    tree = material.node_tree
    surface = entry.get('procedural', 'none')
    normal = None
    if surface != 'none' and not entry.get('baseColorTexture'):
        coordinate = tree.nodes.new('ShaderNodeNewGeometry')
        mapping = tree.nodes.new('ShaderNodeMapping')
        mapping.inputs['Scale'].default_value = (4, 4, 60) if surface == 'wood' else (1, 1, 1)
        noise = tree.nodes.new('ShaderNodeTexNoise')
        noise.inputs['Scale'].default_value = {'wood': 3, 'stone': 90, 'fabric': 450, 'paint': 180}.get(surface, 20)
        noise.inputs['Detail'].default_value = 3
        noise.inputs['Roughness'].default_value = 0.65
        tree.links.new(coordinate.outputs['Position'], mapping.inputs['Vector'])
        tree.links.new(mapping.outputs['Vector'], noise.inputs['Vector'])
        if surface in ('wood', 'stone'):
            ramp = tree.nodes.new('ShaderNodeValToRGB')
            low, high = (0.55, 1.12) if surface == 'wood' else (0.87, 1.03)
            for stop, factor in zip(ramp.color_ramp.elements, (low, high)):
                stop.color = (*(min(1.0, c * factor) for c in entry['baseColor']), 1)
            tree.links.new(noise.outputs['Fac'], ramp.inputs['Fac'])
            tree.links.new(ramp.outputs['Color'], shader.inputs['Base Color'])
        bump = tree.nodes.new('ShaderNodeBump')
        bump.inputs['Strength'].default_value = 0.12 if surface == 'paint' else 0.22
        bump.inputs['Distance'].default_value = 0.0003
        tree.links.new(noise.outputs['Fac'], bump.inputs['Height'])
        normal = bump.outputs['Normal']
        if surface == 'fabric': shader.inputs['Sheen Weight'].default_value = 0.25
    if entry.get('transmission', 0) == 0:
        bevel = tree.nodes.new('ShaderNodeBevel')
        bevel.inputs['Radius'].default_value = 0.0007
        bevel.samples = 4
        if normal: tree.links.new(normal, bevel.inputs['Normal'])
        normal = bevel.outputs['Normal']
    if normal: tree.links.new(normal, shader.inputs['Normal'])

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
    settings = package.get('renderSettings', {})
    scene.cycles.use_denoising = settings.get('denoise', True)
    scene.cycles.max_bounces = 10
    scene.cycles.transmission_bounces = 8
    scene.cycles.use_adaptive_sampling = True
    scene.cycles.adaptive_threshold = 0.03
    scene.view_settings.view_transform = 'AgX'
    scene.view_settings.look = 'AgX - Medium High Contrast'
    scene.view_settings.exposure = settings.get('exposure', 0.0)
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
        surface_detail(entry, material, shader)
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
    selected_id = settings.get('camera', '')
    camera_entry = next((camera for camera in package['cameras'] if camera['id'] == selected_id), None) if selected_id else (package['cameras'][0] if package['cameras'] else None)
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
    scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value = settings.get('environmentStrength', 0.2)
    print('LIBREMAX_SETTINGS', camera_entry['id'], 'exposure', scene.view_settings.exposure,
          'environment', settings.get('environmentStrength', 0.2), 'denoise', scene.cycles.use_denoising,
          'look', scene.view_settings.look, flush=True)
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
