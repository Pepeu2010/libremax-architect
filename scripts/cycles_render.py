"""LibreMax -> Cycles. Invoked by QProcess; no shell or user-generated Python."""
import argparse
import json
import os
import sys
import base64
import hashlib
import math
import bpy
import bmesh
from mathutils import Vector
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from cycles_lights import translate_light

def arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument('--scene', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--preview')
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
    if normal: tree.links.new(normal, shader.inputs['Normal'])

def surface_maps(entry, material, shader, texture_paths):
    tree = material.node_tree
    uv = tree.nodes.new('ShaderNodeTexCoord')
    for channel, socket in [('baseColorTexture', 'Base Color'), ('roughnessTexture', 'Roughness'), ('normalTexture', None)]:
        if not entry.get(channel):
            continue
        texture = tree.nodes.new('ShaderNodeTexImage')
        texture.image = bpy.data.images.load(texture_paths[entry[channel]], check_existing=True)
        texture.image.colorspace_settings.name = 'sRGB' if channel == 'baseColorTexture' else 'Non-Color'
        texture.interpolation = 'Linear'
        tree.links.new(uv.outputs['UV'], texture.inputs['Vector'])
        if socket:
            tree.links.new(texture.outputs['Color'], shader.inputs[socket])
            if channel == 'baseColorTexture' and 'alphaCutoff' in entry:
                cutoff = tree.nodes.new('ShaderNodeMath')
                cutoff.operation = 'GREATER_THAN'
                cutoff.inputs[1].default_value = entry['alphaCutoff']
                tree.links.new(texture.outputs['Alpha'], cutoff.inputs[0])
                tree.links.new(cutoff.outputs[0], shader.inputs['Alpha'])
        else:
            normal = tree.nodes.new('ShaderNodeNormalMap')
            normal.inputs['Strength'].default_value = entry.get('normalStrength', 1)
            normal.uv_map = 'LMXSurfaceUV'
            tree.links.new(texture.outputs['Color'], normal.inputs['Color'])
            bevel = next((node for node in tree.nodes if node.type == 'BEVEL'), None)
            tree.links.new(normal.outputs['Normal'], bevel.inputs['Normal'] if bevel else shader.inputs['Normal'])


def photographic_mesh(part, entry, material):
    mesh = bpy.data.meshes.new(part['owner'])
    mesh.from_pydata(part['vertices'], [], part['triangles'])
    mesh.update()
    authored_uv = entry.get('modelUV', False) and 'uvs' in part
    if not authored_uv:
        # OCC solids have separate vertices. Imported models retain authored seams.
        bm = bmesh.new()
        bm.from_mesh(mesh)
        bmesh.ops.remove_doubles(bm, verts=list(bm.verts), dist=1e-7)
        bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
        bmesh.ops.dissolve_limit(bm, angle_limit=0.001, verts=list(bm.verts), edges=list(bm.edges))
        bm.to_mesh(mesh)
        bm.free()
        mesh.update()
    uv = mesh.uv_layers.new(name='LMXSurfaceUV')
    scale = 1000.0 / entry.get('textureScale', 1000.0)
    rotation = math.radians(entry.get('textureRotation', 0))
    offset = [v / 1000 for v in entry.get('textureOffset', [0, 0, 0])]
    for face in mesh.polygons:
        axis = max(range(3), key=lambda i: abs(face.normal[i]))
        axes = (1, 2) if axis == 0 else (0, 2) if axis == 1 else (0, 1)
        for loop in face.loop_indices:
            if authored_uv:
                uv.data[loop].uv = part['uvs'][mesh.loops[loop].vertex_index]
                continue
            p = mesh.vertices[mesh.loops[loop].vertex_index].co
            u, v = ((p[i] + offset[i]) * scale for i in axes)
            uv.data[loop].uv = (u * math.cos(rotation) - v * math.sin(rotation), u * math.sin(rotation) + v * math.cos(rotation))
        face.use_smooth = True
    mesh.set_sharp_from_angle(angle=math.radians(35))
    if authored_uv and 'normals' in part:
        mesh.normals_split_custom_set_from_vertices(part['normals'])
    obj = bpy.data.objects.new(part['owner'], mesh)
    bpy.context.scene.collection.objects.link(obj)
    obj.data.materials.append(material)
    # Room boundaries meet other solids; beveling them opens tiny daylight gaps.
    if not authored_uv and part.get('kind') not in ('Wall', 'HalfWall', 'Floor', 'Ceiling'):
        bevel = obj.modifiers.new('Physical edge highlights', 'BEVEL')
        bevel.width = 0.0004 if entry.get('transmission', 0) else 0.0012
        bevel.segments = 3
        bevel.limit_method = 'ANGLE'
        bevel.angle_limit = math.radians(35)
        bevel.harden_normals = True
    if not authored_uv:
        normal = obj.modifiers.new('Planar face normals', 'WEIGHTED_NORMAL')
        normal.keep_sharp = True
    # Thin architectural panes pass daylight shadow rays; camera/glossy rays keep glass.
    # This avoids noisy refractive caustics, while preserving geometry and reflections.
    if part.get('kind') == 'Window' and entry.get('transmission', 0) > 0:
        obj.visible_shadow = False
    return obj


def main():
    args = arguments()
    with open(args.scene, encoding='utf-8') as stream:
        package = json.load(stream)
    if package['schema'] != 1:
        raise ValueError('Unsupported scene schema')
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    scene = bpy.context.scene
    if bpy.app.version < (5, 2, 0):
        raise RuntimeError('LibreMax requires Blender 5.2 LTS or newer')
    scene.render.engine = 'CYCLES'
    scene.cycles.samples = args.samples
    settings = package.get('renderSettings', {})
    scene.cycles.use_denoising = settings.get('denoise', True)
    cycles = settings.get('cycles', {})
    scene.cycles.max_bounces = cycles.get('maxBounces', 16)
    scene.cycles.diffuse_bounces = cycles.get('diffuseBounces', 8)
    scene.cycles.glossy_bounces = cycles.get('glossyBounces', 8)
    scene.cycles.transmission_bounces = cycles.get('transmissionBounces', 12)
    scene.cycles.transparent_max_bounces = cycles.get('transparentBounces', 12)
    scene.cycles.denoising_prefilter = 'ACCURATE'
    scene.cycles.denoising_input_passes = 'RGB_ALBEDO_NORMAL'
    scene.cycles.sample_clamp_indirect = cycles.get('clamp', 5)
    scene.cycles.use_adaptive_sampling = True
    scene.cycles.adaptive_threshold = cycles.get('noiseThreshold', 0.008)
    scene.render.film_transparent = cycles.get('transparent', False)
    scene.view_settings.view_transform = 'AgX'
    scene.view_settings.look = 'AgX - Medium High Contrast'
    scene.view_settings.exposure = settings.get('exposure', 0.0)
    scene.cycles.device = 'CPU'
    selected_backend = 'CPU'
    selected_devices = []
    if args.device == 'AUTO':
        prefs = bpy.context.preferences.addons['cycles'].preferences
        for backend in ('OPTIX', 'CUDA', 'HIP', 'ONEAPI'):
            try:
                prefs.compute_device_type = backend
                prefs.refresh_devices()
                devices = [device for device in prefs.devices if device.type == backend]
                if devices:
                    for device in prefs.devices:
                        device.use = device.type == backend
                    scene.cycles.device = 'GPU'
                    selected_backend = backend
                    selected_devices = [device.name for device in devices]
                    print('LIBREMAX_DEVICE', backend, [device.name for device in devices], flush=True)
                    break
            except (TypeError, RuntimeError):
                continue
    print('LIBREMAX_RENDER_DEVICE', scene.cycles.device, flush=True)
    materials = {}
    texture_paths = {}
    environment = settings.get('hdri', {})
    environment_hash = environment.get('asset') if settings.get('environmentMode') == 'hdri' else None
    texture_hashes = {entry[channel] for entry in package['materials']
                      for channel in ('baseColorTexture', 'roughnessTexture', 'normalTexture') if channel in entry}
    for digest, encoded in package.get('assets', {}).items():
        data = base64.b64decode(encoded, validate=True)
        if hashlib.sha256(data).hexdigest() != digest or len(digest) != 64 or any(c not in '0123456789abcdef' for c in digest):
            raise ValueError('Invalid asset hash')
        if digest not in texture_hashes and digest != environment_hash:
            continue
        extension = '.' + environment['format'] if digest == environment_hash else '.png'
        texture_path = os.path.join(os.path.dirname(args.scene), digest + extension)
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
        surface_maps(entry, material, shader, texture_paths)
        materials[entry['id']] = material
    material_entries = {entry['id']: entry for entry in package['materials']}
    for part in package['meshes']:
        photographic_mesh(part, material_entries[part['material']], materials[part['material']])
    for entry in package['lights']:
        translate_light(entry, scene)
    selected_id = settings.get('camera', '')
    camera_entry = next((camera for camera in package['cameras'] if camera['id'] == selected_id), None) if selected_id else (package['cameras'][0] if package['cameras'] else None)
    if camera_entry is None:
        raise ValueError('Create a persistent camera before rendering')
    camera = bpy.data.cameras.new(camera_entry['name'])
    camera.lens = camera_entry['parameters'].get('lens', 28)
    camera.sensor_width = 36
    camera.sensor_fit = 'HORIZONTAL'
    obj = bpy.data.objects.new(camera_entry['name'], camera)
    scene.collection.objects.link(obj)
    obj.location = camera_entry['position']
    target = Vector([v / 1000 for v in camera_entry['parameters'].get('target', [2000, 1500, 1000])])
    obj.rotation_euler = (target - obj.location).to_track_quat('-Z', 'Y').to_euler()
    scene.camera = obj
    camera.dof.use_dof = True
    camera.dof.aperture_fstop = camera_entry['parameters'].get('fstop', 8)
    camera.dof.focus_distance = camera_entry['parameters'].get('focusDistance', (target - obj.location).length * 1000) / 1000
    camera.dof.aperture_blades = 7
    scene.world.use_nodes = True
    scene.world.node_tree.nodes['Background'].inputs['Color'].default_value = (*settings.get('backgroundColor', [0.7, 0.8, 1.0]), 1.0)
    scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value = settings.get('environmentStrength', 0.2)
    if settings.get('environmentMode', 'studio') == 'sky':
        sky = scene.world.node_tree.nodes.new('ShaderNodeTexSky')
        sky.sky_type = 'MULTIPLE_SCATTERING'
        sky.sun_elevation = math.radians(settings.get('sunElevation', 35))
        sky.sun_rotation = math.radians(settings.get('sunRotation', 30))
        sky.sun_size = math.radians(0.526)
        sky.sun_intensity = 1
        scene.world.node_tree.links.new(sky.outputs['Color'], scene.world.node_tree.nodes['Background'].inputs['Color'])
    if settings.get('environmentMode') == 'hdri':
        if environment_hash not in texture_paths:
            raise ValueError('Embedded HDRI missing')
        world = scene.world.node_tree
        texture = world.nodes.new('ShaderNodeTexEnvironment')
        texture.image = bpy.data.images.load(texture_paths[environment_hash], check_existing=True)
        texture.image.colorspace_settings.name = 'Linear Rec.709'
        texture.projection = 'EQUIRECTANGULAR'
        coordinate = world.nodes.new('ShaderNodeTexCoord')
        mapping = world.nodes.new('ShaderNodeMapping')
        mapping.inputs['Rotation'].default_value[2] = math.radians(environment.get('rotation', 0))
        world.links.new(coordinate.outputs['Generated'], mapping.inputs['Vector'])
        world.links.new(mapping.outputs['Vector'], texture.inputs['Vector'])
        world.links.new(texture.outputs['Color'], world.nodes['Background'].inputs['Color'])
        if not environment.get('visible', True):
            camera_background = world.nodes.new('ShaderNodeBackground')
            camera_background.inputs['Color'].default_value = (*settings.get('backgroundColor', [0.7, 0.8, 1.0]), 1.0)
            camera_background.inputs['Strength'].default_value = 1
            rays = world.nodes.new('ShaderNodeLightPath')
            mix = world.nodes.new('ShaderNodeMixShader')
            world.links.new(rays.outputs['Is Camera Ray'], mix.inputs[0])
            world.links.new(world.nodes['Background'].outputs['Background'], mix.inputs[1])
            world.links.new(camera_background.outputs['Background'], mix.inputs[2])
            world.links.new(mix.outputs[0], world.nodes['World Output'].inputs['Surface'])
        print('LIBREMAX_HDRI', environment_hash, list(texture.image.size), environment.get('rotation', 0), environment.get('visible', True), flush=True)
    print('LIBREMAX_PHOTOGRAPHIC', 'PBR maps', len(texture_paths), 'environment', settings.get('environmentMode', 'studio'), 'UV physical scale', 'beveled geometry', 'fstop', camera.dof.aperture_fstop, flush=True)
    print('LIBREMAX_SETTINGS', camera_entry['id'], 'exposure', scene.view_settings.exposure,
          'environment', settings.get('environmentStrength', 0.2), 'denoise', scene.cycles.use_denoising,
          'look', scene.view_settings.look, flush=True)
    scene.render.resolution_x = args.width
    scene.render.resolution_y = args.height
    scene.render.resolution_percentage = 100
    is_exr = args.output.lower().endswith('.exr')
    scene.render.image_settings.file_format = 'OPEN_EXR' if is_exr else 'JPEG' if args.output.lower().endswith(('.jpg', '.jpeg')) else 'PNG'
    scene.render.image_settings.color_mode = 'RGBA' if is_exr or scene.render.film_transparent else 'RGB'
    scene.render.image_settings.color_depth = '32' if is_exr else '8'
    if is_exr:
        scene.render.image_settings.exr_codec = 'ZIP'
    scene.render.filepath = os.path.abspath(args.output)
    scene.render.use_file_extension = False
    bpy.ops.wm.save_as_mainfile(filepath=os.path.join(os.path.dirname(args.scene), 'scene.blend'))
    # These are engine statistics, never a percentage inferred from elapsed time.
    def engine_stats(stats):
        print('LIBREMAX_STATS', stats, flush=True)
    bpy.app.handlers.render_stats.append(engine_stats)
    print('LIBREMAX_ENGINE', json.dumps({'blender': bpy.app.version_string,
        'device': scene.cycles.device, 'backend': selected_backend, 'devices': selected_devices,
        'samples': scene.cycles.samples,
        'maxBounces': scene.cycles.max_bounces}), flush=True)
    print('LIBREMAX_STAGE Rendering', flush=True)
    try:
        bpy.ops.render.render(write_still=True)
    except RuntimeError as error:
        if scene.cycles.device != 'GPU':
            raise
        print('LIBREMAX_GPU_FALLBACK', str(error), flush=True)
        scene.cycles.device = 'CPU'
        print('LIBREMAX_ENGINE', json.dumps({'blender': bpy.app.version_string,
            'device': 'CPU', 'backend': 'CPU', 'devices': [], 'samples': scene.cycles.samples,
            'maxBounces': scene.cycles.max_bounces, 'fallback': True}), flush=True)
        bpy.ops.render.render(write_still=True)
    if is_exr:
        if not args.preview:
            raise ValueError('EXR requires an integrated preview path')
        scene.render.image_settings.file_format = 'PNG'
        scene.render.image_settings.color_mode = 'RGBA' if scene.render.film_transparent else 'RGB'
        scene.render.image_settings.color_depth = '8'
        bpy.data.images['Render Result'].save_render(os.path.abspath(args.preview), scene=scene)
    print('LIBREMAX_COMPLETED', scene.render.filepath, flush=True)

if __name__ == '__main__':
    main()
