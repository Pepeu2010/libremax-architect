"""Translate persistent LibreMax lights into native Cycles lights and LED emitters."""
import json
import math
import bpy
from mathutils import Vector


def translate_light(entry, scene):
    p = entry['parameters']
    kind = p.get('kind', 'area')
    native = bpy.data.lights.new(entry['name'], 'AREA' if kind == 'led' else kind.upper())
    native.energy = p.get('power', 500)
    native.color = p.get('color', [1.0, 0.89, 0.73])
    native.use_temperature = p.get('colorMode', 'custom') == 'kelvin'
    if native.use_temperature:
        native.temperature = p.get('temperature', 3000)
        native.color = (1, 1, 1)
    color = tuple(native.temperature_color) if native.use_temperature else tuple(native.color)
    target = Vector([v / 1000 for v in p.get('target', [2000, 1500, 0])])
    position = Vector(entry['position'])
    direction = target - position
    if direction.length_squared < 1e-12:
        direction = Vector((0, 0, -1))
    descriptor = {'id': entry['id'], 'kind': kind, 'power': native.energy,
                  'color': color, 'temperature': native.temperature if native.use_temperature else None,
                  'position': list(position), 'rotationZ': entry.get('rotationZ', 0)}
    if kind == 'led':
        length, width = p.get('size', 1200) / 1000, p.get('sizeY', 12) / 1000
        mesh = bpy.data.meshes.new(entry['id'])
        mesh.from_pydata([(-length/2, -width/2, 0), (length/2, -width/2, 0),
                          (length/2, width/2, 0), (-length/2, width/2, 0)], [], [(0, 1, 2, 3)])
        mesh.update()
        material = bpy.data.materials.new(entry['name'] + ' — emissão')
        material.use_nodes = True
        tree = material.node_tree
        tree.nodes.clear()
        emitter = tree.nodes.new('ShaderNodeEmission')
        emitter.inputs['Color'].default_value = (*color, 1)
        # Radiant flux of a one-sided Lambertian rectangle: P = pi * area * radiance.
        emitter.inputs['Strength'].default_value = p.get('power', 10) / (math.pi * length * width)
        back = tree.nodes.new('ShaderNodeBsdfDiffuse')
        back.inputs['Color'].default_value = (0, 0, 0, 1)
        geometry = tree.nodes.new('ShaderNodeNewGeometry')
        mix = tree.nodes.new('ShaderNodeMixShader')
        output = tree.nodes.new('ShaderNodeOutputMaterial')
        tree.links.new(geometry.outputs['Backfacing'], mix.inputs[0])
        tree.links.new(emitter.outputs[0], mix.inputs[1])
        tree.links.new(back.outputs[0], mix.inputs[2])
        tree.links.new(mix.outputs[0], output.inputs['Surface'])
        mesh.materials.append(material)
        obj = bpy.data.objects.new(entry['name'], mesh)
        obj.rotation_euler = direction.to_track_quat('Z', 'Y').to_euler()
        descriptor.update(type='EMISSIVE_MESH', size=length, sizeY=width,
                          radiance=emitter.inputs['Strength'].default_value)
        bpy.data.lights.remove(native)
    else:
        if kind == 'area':
            native.shape = p.get('shape', 'DISK')
            native.size = p.get('size', 1000) / 1000
            if native.shape == 'RECTANGLE':
                native.size_y = p.get('sizeY', p.get('size', 1000)) / 1000
            descriptor.update(shape=native.shape, size=native.size,
                              sizeY=native.size_y if native.shape == 'RECTANGLE' else native.size)
        elif kind in ('point', 'spot'):
            native.shadow_soft_size = p.get('radius', 0) / 1000
            descriptor['radius'] = native.shadow_soft_size
            if kind == 'spot':
                native.spot_size = math.radians(p.get('angle', 45))
                native.spot_blend = p.get('blend', 0.3)
                descriptor.update(angle=native.spot_size, blend=native.spot_blend)
        elif kind == 'sun':
            native.angle = math.radians(p.get('sunAngle', 0.526))
            descriptor['angle'] = native.angle
        obj = bpy.data.objects.new(entry['name'], native)
        if kind != 'point':
            obj.rotation_euler = direction.to_track_quat('-Z', 'Y').to_euler()
        descriptor['type'] = native.type
    obj.rotation_euler.rotate_axis('Z', math.radians(entry.get('rotationZ', 0)))
    obj.location = position
    scene.collection.objects.link(obj)
    descriptor['direction'] = list((obj.rotation_euler.to_matrix() @ Vector((0, 0, 1 if kind == 'led' else -1))).normalized())
    print('LIBREMAX_LIGHT', json.dumps(descriptor), flush=True)
    return obj
