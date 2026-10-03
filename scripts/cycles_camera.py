"""Persistent LibreMax camera -> Blender, shared by rendering and camera acceptance."""
import bpy
from mathutils import Matrix, Vector


def translate_camera(entry, scene):
    parameters = entry['parameters']
    camera = bpy.data.cameras.new(entry['name'])
    camera.lens = parameters.get('lens', 28)
    camera.sensor_width = parameters.get('sensorWidth', 36)
    camera.sensor_fit = 'HORIZONTAL'
    camera.shift_x = parameters.get('shiftX', 0)
    camera.shift_y = parameters.get('shiftY', 0)
    camera.clip_start = parameters.get('clipNear', 100) / 1000
    camera.clip_end = parameters.get('clipFar', 1e6) / 1000
    obj = bpy.data.objects.new(entry['name'], camera)
    scene.collection.objects.link(obj)
    obj.location = entry['position']
    target = Vector([v / 1000 for v in parameters.get('target', [2000, 1500, 1000])])
    direction = target - obj.location
    if direction.length < 1e-8:
        raise ValueError('Camera position and target must differ')
    direction.normalize()
    up = Vector(parameters.get('up', [0, 0, 1]))
    if 'up' not in parameters and abs(direction.z) > .999999:
        up = Vector((0, 1, 0))
    right = direction.cross(up)
    if right.length < 1e-8:
        raise ValueError('Camera up direction is parallel to its target')
    right.normalize()
    up = right.cross(direction).normalized()
    obj.rotation_euler = Matrix((right, up, -direction)).transposed().to_euler()
    scene.camera = obj
    camera.dof.use_dof = True
    camera.dof.aperture_fstop = parameters.get('fstop', 8)
    camera.dof.focus_distance = parameters.get('focusDistance', (target - obj.location).length * 1000) / 1000
    camera.dof.aperture_blades = 7
    print('LIBREMAX_CAMERA', entry['id'], 'lens', camera.lens, 'sensor', camera.sensor_width,
          'shift', camera.shift_x, camera.shift_y, 'clip', camera.clip_start, camera.clip_end, flush=True)
    return camera
