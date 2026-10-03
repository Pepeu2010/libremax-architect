"""Run inside the bundled Blender; compare its actual projection with native evidence."""
import argparse
import json
import os
import sys
import bpy
from mathutils import Vector
from bpy_extras.object_utils import world_to_camera_view
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from cycles_camera import translate_camera

parser = argparse.ArgumentParser()
parser.add_argument('--input', required=True)
parser.add_argument('--output', required=True)
options = parser.parse_args(sys.argv[sys.argv.index('--') + 1:])
with open(options.input, encoding='utf-8') as stream:
    cases = json.load(stream)
report = []
for case in cases:
    scene = bpy.context.scene
    scene.render.resolution_x, scene.render.resolution_y = case['image']
    scene.render.resolution_percentage = 100
    translate_camera(case['camera'], scene)
    bpy.context.view_layer.update()
    error = 0
    for point in case['points']:
        actual = world_to_camera_view(scene, scene.camera, Vector([v / 1000 for v in point['world']]))
        discrepancy = max(abs(actual[i] - point['normalized'][i]) for i in range(3))
        error = max(error, discrepancy)
        if discrepancy > 1e-5:
            raise AssertionError(f"Camera projection mismatch: {case['name']}: {list(actual)} vs {point['normalized']}")
    camera = scene.camera.data
    parameters = case['camera']['parameters']
    assert abs(camera.clip_start - parameters.get('clipNear', 100) / 1000) < 1e-6
    assert abs(camera.clip_end - parameters.get('clipFar', 1e6) / 1000) < 1e-4
    report.append({'name': case['name'], 'points': len(case['points']), 'maxNormalizedError': error})
with open(options.output, 'w', encoding='utf-8') as stream:
    json.dump({'blender': bpy.app.version_string, 'cases': report}, stream, indent=2)
print('CAMERA_BLENDER_PROJECTION_PASS', bpy.app.version_string, len(report), flush=True)
