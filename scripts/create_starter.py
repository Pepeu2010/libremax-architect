"""Author-owned procedural recipes. No duplicates added to meet catalogue counts."""
import json
from pathlib import Path

entries = []
def add(id, name, category, w, h, d, type, family, **parameters):
    entries.append(dict(id=id, name=name, category=category, width=w, height=h, depth=d,
                        tags=f'{category} {family} {w} {h} {d}',
                        recipe=dict(type=type, material='fabric' if family in ('sofa', 'rug', 'chair') else 'white',
                                    parameters=dict(family=family, **parameters)),
                        license='CC0-1.0', author='LibreMax contributors',
                        origin='LibreMax procedural generators', date='2026-09-29'))
for id, name, w, h, d, family, doors, legs in [
    ('base-1','Balcão 1 porta',600,720,550,'cabinet',1,100),
    ('base-2','Balcão 2 portas',800,720,550,'cabinet',2,100),
    ('base-3','Balcão 3 portas',1200,720,550,'cabinet',3,100),
    ('drawer','Gaveteiro',600,720,550,'drawer',1,100),
    ('upper-1','Aéreo 1 porta',600,700,320,'cabinet',1,0),
    ('upper-2','Aéreo 2 portas',800,700,320,'cabinet',2,0),
    ('tower','Torre de armazenamento',600,2100,550,'cabinet',2,100),
    ('niche','Nicho aberto',600,400,320,'niche',1,0),
    ('island','Ilha 3 portas',1500,850,700,'cabinet',3,100)]:
    add(id,name,'Cozinha',w,h,d,'FurnitureModule',family,doors=doors,legs=legs,carcass='oak',handle='bar',glass=False)
for id, name, w, h, d, family, doors, legs in [
    ('wardrobe-2','Roupeiro 2 portas',1200,2200,600,'cabinet',2,80),
    ('wardrobe-3','Roupeiro 3 portas',1800,2200,600,'cabinet',3,80),
    ('wardrobe-4','Roupeiro 4 portas',2400,2200,600,'cabinet',4,80),
    ('nightstand','Criado com gavetas',500,600,420,'drawer',1,80),
    ('shelf','Estante aberta',800,1800,320,'shelf',1,80)]:
    add(id,name,'Dormitório',w,h,d,'FurnitureModule',family,doors=doors,legs=legs,carcass='oak',handle='point',glass=False)
for id,name,category,w,h,d,family in [
    ('bed','Cama com cabeceira','Dormitório',1600,450,2000,'bed'),
    ('sofa','Sofá 3 lugares','Sala',2100,850,850,'sofa'),
    ('chair','Cadeira de jantar','Sala',450,850,450,'chair'),
    ('table','Mesa de jantar','Sala',1600,750,850,'table'),
    ('desk','Mesa de escritório','Escritório',1200,750,600,'desk'),
    ('fridge','Geladeira duplex','Eletrodomésticos',700,1800,650,'fridge'),
    ('oven','Forno de embutir','Eletrodomésticos',600,600,550,'oven'),
    ('vase','Vaso cerâmico','Decoração',240,350,240,'vase'),
    ('plant','Planta em vaso','Decoração',450,1100,450,'plant'),
    ('rug','Tapete retangular','Decoração',2200,8,1600,'rug'),
    ('picture','Quadro com moldura','Decoração',700,900,33,'picture')]:
    add(id,name,category,w,h,d,'DecorativeObject',family)
target = Path(__file__).resolve().parents[1] / 'starter-library' / 'catalog.json'
for item in entries:
    p = item['recipe']['parameters']
    item_id = item['id']
    p['placement'] = ('wall' if item_id in ('upper-1', 'upper-2', 'niche', 'picture') else
                      'surface' if item_id == 'vase' else 'rug' if item_id == 'rug' else 'floor')
    p['defaultElevation'] = 1400 if item_id.startswith('upper-') else 1200 if item_id in ('niche', 'picture') else 0
for kind, name, w, h, sill in [('Door','Porta de madeira',800,2100,0), ('Window','Janela com vidro',1200,1000,1000)]:
    entries.append(dict(id=kind.lower()+'-ready', name=name, category='Portas e janelas', width=w,height=h,depth=120,tags=name,
                        recipe=dict(type=kind,material='oak',parameters=dict(placement='wall',sill=sill,offset=0,openAngle=0,hinge='left')),
                        license='CC0-1.0',author='LibreMax contributors',origin='LibreMax procedural generators',date='2026-09-30'))
target.parent.mkdir(exist_ok=True)
target.write_text(json.dumps(entries, ensure_ascii=False, indent=2), encoding='utf-8')
print(f'{len(entries)} assets written')
