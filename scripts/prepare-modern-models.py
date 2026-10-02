"""Prepare a curated CC0 Poly Haven collection. Requires numpy, Pillow and curl.

Developer tool only. Runtime uses bundled meshes/maps and never downloads files.
Sources, original MD5/SHA256 and prepared SHA256 are recorded in provenance.json.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import io
import json
from pathlib import Path
import subprocess

import numpy as np
from PIL import Image, ImageStat

SELECTION = [
    ('modern_arm_chair_01', 'Poltrona de madeira e couro', 'Sala', 'floor'),
    ('mid_century_lounge_chair', 'Poltrona de descanso contemporânea', 'Sala', 'floor'),
    ('steel_frame_shelves_01', 'Estante de madeira e metal', 'Sala', 'floor'),
    ('steel_frame_shelves_03', 'Estante contemporânea com gavetas', 'Sala', 'floor'),
    ('modern_coffee_table_01', 'Mesa de centro em madeira e pedra', 'Sala', 'floor'),
    ('modern_coffee_table_02', 'Mesa de centro com tampo claro', 'Sala', 'floor'),
    ('modern_wooden_cabinet', 'Aparador de madeira contemporâneo', 'Sala', 'floor'),
    ('outdoor_table_chair_set_01', 'Mesa e cadeiras para varanda', 'Decoração', 'floor'),
    ('dining_chair_02', 'Cadeira de jantar detalhada', 'Sala', 'floor'),
    ('coffee_table_round_01', 'Mesa redonda de mármore e metal', 'Sala', 'floor'),
    ('side_table_01', 'Mesa lateral compacta', 'Sala', 'floor'),
    ('drawer_cabinet', 'Gaveteiro de madeira e metal', 'Escritório', 'floor'),
    ('metal_office_desk', 'Escrivaninha de estilo industrial', 'Escritório', 'floor'),
    ('modern_ceiling_lamp_01', 'Luminária de teto contemporânea', 'Decoração', 'ceiling'),
    ('desk_lamp_arm_01', 'Luminária articulada de mesa', 'Escritório', 'surface'),
    ('ceramic_vase_02', 'Vaso de cerâmica natural', 'Decoração', 'surface'),
    ('ceramic_vase_03', 'Vaso de cerâmica contemporâneo', 'Decoração', 'surface'),
    ('throw_pillows_01', 'Almofadas decorativas detalhadas', 'Decoração', 'surface'),
    ('potted_plant_04', 'Suculenta em vaso de cerâmica', 'Decoração', 'surface'),
    ('round_wooden_table_02', 'Mesa de jantar redonda de madeira', 'Sala', 'floor'),
    ('metal_stool_02', 'Banqueta de metal com assento redondo', 'Cozinha', 'floor'),
    ('metal_stool_03', 'Banqueta industrial compacta', 'Cozinha', 'floor'),
    ('wicker_basket_02', 'Cesto de fibras naturais', 'Decoração', 'floor'),
    ('side_table_tall_01', 'Mesa de apoio alta de madeira', 'Sala', 'floor'),
]
# Some source exports use scene units larger than meters. These are explicit
# suggested apartment dimensions, not certified manufacturer specifications.
DIMENSION_OVERRIDES = {'steel_frame_shelves_01': [900, 411.8, 1755.4]}
ROOT = Path(__file__).resolve().parents[1]
TARGET = ROOT / 'starter-models'

def download(url, path, md5=None):
    assert url.startswith(('https://api.polyhaven.com/', 'https://dl.polyhaven.org/'))
    path.parent.mkdir(parents=True, exist_ok=True)
    if not path.exists():
        subprocess.run(['curl', '--silent', '--show-error', '--location', '--fail', '--retry', '2',
                        '--max-time', '90', url, '--output', str(path)], check=True)
    data = path.read_bytes()
    if md5 and hashlib.md5(data).hexdigest() != md5:
        raise ValueError(f'Source integrity failed: {path}')
    return dict(url=url, bytes=len(data), md5=hashlib.md5(data).hexdigest(),
                sha256=hashlib.sha256(data).hexdigest())

def source_files(asset_id, cache):
    folder = cache / asset_id
    download(f'https://api.polyhaven.com/files/{asset_id}', folder/'files.json')
    files = json.loads((folder/'files.json').read_text(encoding='utf-8'))
    entry = files['gltf']['1k']['gltf']
    filename = asset_id + '.gltf'
    provenance = [download(entry['url'], folder/filename, entry['md5'])]
    for relative, item in entry.get('include', {}).items():
        path = (folder/relative).resolve()
        assert path.is_relative_to(folder.resolve())
        provenance.append(download(item['url'], path, item['md5']))
    return folder/filename, provenance

def read_accessor(gltf, buffers, index):
    access = gltf['accessors'][index]
    assert 'sparse' not in access
    view = gltf['bufferViews'][access['bufferView']]
    dtype = {5120:'i1',5121:'u1',5122:'<i2',5123:'<u2',5125:'<u4',5126:'<f4'}[access['componentType']]
    count = {'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4}[access['type']]
    width = np.dtype(dtype).itemsize
    offset = view.get('byteOffset',0) + access.get('byteOffset',0)
    result = np.ndarray((access['count'],count), dtype=dtype, buffer=buffers[view['buffer']],
                        offset=offset, strides=(view.get('byteStride',count*width),width)).copy()
    if access.get('normalized'):
        result = result.astype(float)/np.iinfo(dtype).max
    return result

def node_matrix(node):
    if 'matrix' in node:
        return np.array(node['matrix']).reshape((4,4),order='F')
    x,y,z,w = node.get('rotation',[0,0,0,1])
    rotation=np.array([[1-2*(y*y+z*z),2*(x*y-z*w),2*(x*z+y*w)],
                       [2*(x*y+z*w),1-2*(x*x+z*z),2*(y*z-x*w)],
                       [2*(x*z-y*w),2*(y*z+x*w),1-2*(x*x+y*y)]])
    result=np.eye(4)
    result[:3,:3]=rotation@np.diag(node.get('scale',[1,1,1]))
    result[:3,3]=node.get('translation',[0,0,0])
    return result

def save_map(image):
    image.thumbnail((512,512),Image.Resampling.LANCZOS)
    data=io.BytesIO()
    image.save(data,format='PNG',optimize=True)
    payload=data.getvalue()
    digest=hashlib.sha256(payload).hexdigest()
    (TARGET/(digest+'.png')).write_bytes(payload)
    return digest

def prepare_materials(gltf, folder, asset_id):
    result=[]
    def texture(index):
        definition=gltf['images'][gltf['textures'][index]['source']]
        path=(folder/definition['uri']).resolve()
        assert path.is_relative_to(folder.resolve())
        return Image.open(path).copy()
    for index, original in enumerate(gltf.get('materials', [])):
        pbr=original.get('pbrMetallicRoughness',{})
        factor=pbr.get('baseColorFactor',[1,1,1,1])
        entry=dict(id=f'ph-{asset_id}-{index}',name=original.get('name','Acabamento do modelo'),
                   baseColor=factor[:3],roughness=pbr.get('roughnessFactor',1),
                   metallic=pbr.get('metallicFactor',1),transmission=0,opacity=factor[3],ior=1.45,
                   procedural='none',modelUV=True,textureScale=1000)
        if 'baseColorTexture' in pbr:
            image=texture(pbr['baseColorTexture']['index']).convert('RGBA')
            if factor != [1,1,1,1]:
                values=np.asarray(image,dtype=float)/255
                rgb=values[:,:,:3]
                rgb=np.where(rgb<=.04045,rgb/12.92,((rgb+.055)/1.055)**2.4)*factor[:3]
                values[:,:,:3]=np.where(rgb<=.0031308,rgb*12.92,1.055*np.maximum(rgb,0)**(1/2.4)-.055)
                values[:,:,3]*=factor[3]
                image=Image.fromarray(np.clip(values*255,0,255).astype('uint8'),'RGBA')
            entry['baseColorTexture']=save_map(image)
            entry['baseColor']=[round((v/255)**2.2,5) for v in ImageStat.Stat(image.convert('RGB')).mean]
        if 'metallicRoughnessTexture' in pbr:
            packed=texture(pbr['metallicRoughnessTexture']['index']).convert('RGB')
            rough=packed.getchannel('G').point(lambda value:round(value*entry['roughness']))
            entry['roughnessTexture']=save_map(rough)
            entry['metallic']=round(ImageStat.Stat(packed.getchannel('B')).mean[0]/255*entry['metallic'],4)
        if 'normalTexture' in original:
            entry['normalTexture']=save_map(texture(original['normalTexture']['index']).convert('RGB'))
            entry['normalStrength']=original['normalTexture'].get('scale',1)
        if original.get('alphaMode') == 'MASK':
            entry['alphaCutoff']=original.get('alphaCutoff',.5)
        result.append(entry)
    return result

def prepare_model(path, asset_id):
    gltf=json.loads(path.read_text(encoding='utf-8'))
    buffers=[]
    for entry in gltf['buffers']:
        local=(path.parent/entry['uri']).resolve()
        assert local.is_relative_to(path.parent.resolve())
        buffers.append(local.read_bytes())
    parts=[]
    def visit(index,parent):
        node=gltf['nodes'][index]
        matrix=parent@node_matrix(node)
        if 'mesh' in node:
            for primitive in gltf['meshes'][node['mesh']]['primitives']:
                assert primitive.get('mode',4)==4
                attrs=primitive['attributes']
                vertices=read_accessor(gltf,buffers,attrs['POSITION']).astype(float)
                vertices=vertices@matrix[:3,:3].T+matrix[:3,3]
                vertices=np.stack((vertices[:,0],-vertices[:,2],vertices[:,1]),axis=1)
                indices=read_accessor(gltf,buffers,primitive['indices']).reshape((-1,3))
                if np.linalg.det(matrix[:3,:3]) < 0:
                    indices=indices[:,[0,2,1]]
                uv=read_accessor(gltf,buffers,attrs['TEXCOORD_0']).astype(float)
                uv[:,1]=1-uv[:,1]
                part=dict(material=f'ph-{asset_id}-{primitive.get("material",0)}',
                          vertices=vertices,triangles=indices.tolist(),uvs=uv.round(7).tolist())
                if 'NORMAL' in attrs:
                    normal=read_accessor(gltf,buffers,attrs['NORMAL'])@np.linalg.inv(matrix[:3,:3])
                    normal=np.stack((normal[:,0],-normal[:,2],normal[:,1]),axis=1)
                    normal/=np.maximum(np.linalg.norm(normal,axis=1,keepdims=True),1e-12)
                    part['normals']=normal.round(7).tolist()
                parts.append(part)
        for child in node.get('children',[]):
            visit(child,matrix)
    for index in gltf['scenes'][gltf.get('scene',0)]['nodes']:
        visit(index,np.eye(4))
    all_vertices=np.concatenate([p['vertices'] for p in parts])
    low,high=all_vertices.min(axis=0),all_vertices.max(axis=0)
    extent=high-low
    assert min(extent)>1e-6
    # Authored normals describe the physical source, so store in normalized-mesh space.
    for part in parts:
        part['vertices']=((part['vertices']-low)/extent).round(7).tolist()
        if 'normals' in part:
            normal=np.array(part['normals'])*extent
            normal/=np.maximum(np.linalg.norm(normal,axis=1,keepdims=True),1e-12)
            part['normals']=normal.round(7).tolist()
    materials=prepare_materials(gltf,path.parent,asset_id)
    payload=json.dumps(dict(schema=1,parts=parts,materials=materials),separators=(',',':')).encode()
    assert len(payload)<=4*1024*1024, (asset_id,len(payload))
    assert sum(len(p['triangles']) for p in parts)<=200000
    return payload,[round(v*1000,1) for v in extent]

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--cache',type=Path,default=ROOT/'build/modern-model-research')
    args=parser.parse_args()
    TARGET.mkdir(exist_ok=True)
    download('https://api.polyhaven.com/assets?t=models',args.cache/'assets.json')
    metadata=json.loads((args.cache/'assets.json').read_text(encoding='utf-8'))
    with ThreadPoolExecutor(max_workers=4) as pool:
        sources=list(pool.map(lambda row:source_files(row[0],args.cache),SELECTION))
    catalog,provenance=[],[]
    for row,(path,files) in zip(SELECTION,sources):
        asset_id,name,category,placement=row
        payload,source_dimensions=prepare_model(path,asset_id)
        dimensions=DIMENSION_OVERRIDES.get(asset_id,source_dimensions)
        filename='modern-'+asset_id+'.json'
        (TARGET/filename).write_bytes(payload)
        width,depth,height=dimensions
        digest=hashlib.sha256(payload).hexdigest()
        author=', '.join(metadata[asset_id].get('authors',{}))
        catalog.append(dict(id='modern-'+asset_id,name=name,category=category,width=width,height=height,depth=depth,
                            tags=f'apartamento atual moderno contemporâneo detalhado {name}',license='CC0-1.0',
                            author=author,origin='https://polyhaven.com/a/'+asset_id,date='2026-10-01',
                            recipe=dict(type='MeshObject',material='white',modelFile=filename,
                                        source='https://polyhaven.com/a/'+asset_id,author=author,
                                        collection='apartment-modern',parameters=dict(meshAsset=digest,
                                        originalMaterials=True,placement=placement,defaultElevation=0))))
        provenance.append(dict(id=asset_id,author=author,license='CC0-1.0',
                               source='https://polyhaven.com/a/'+asset_id,sources=files,
                               preparedSha256=digest,dimensionsMillimeters=dimensions,
                               sourceDimensionsMillimeters=source_dimensions,
                               dimensionBasis='suggested' if asset_id in DIMENSION_OVERRIDES else 'source'))
        print(f'{asset_id}: {dimensions}, {len(payload)} bytes',flush=True)
    (TARGET/'modern-catalog.json').write_text(json.dumps(catalog,ensure_ascii=False,indent=2),encoding='utf-8')
    (TARGET/'modern-provenance.json').write_text(json.dumps(provenance,ensure_ascii=False,indent=2),encoding='utf-8')
    print(f'{len(catalog)} detailed models prepared',flush=True)

if __name__=='__main__':
    main()
