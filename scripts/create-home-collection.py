"""Original CC0 designs for contemporary apartments. Run in Blender background."""
import json
from pathlib import Path
import runpy
import bpy

ROOT=Path(__file__).resolve().parents[1]
api=runpy.run_path(str(ROOT/'scripts/create-apartment-models.py'))
box,cylinder,rail,legs=(api[key] for key in ('box','cylinder','rail','legs'))
for key,(_title,color,roughness,metallic,_procedure) in api['MATERIALS'].items():
    mat=bpy.data.materials.new(key);mat.diffuse_color=(*color,1);mat.roughness=roughness;mat.metallic=metallic

def cabinet(width=.8,height=.85,depth=.60,sliding=False):
    box((width/2,depth/2,height/2),(width,depth,height),'oak',.008)
    for i in range(2):
        x=width*(i+.5)/2
        box((x,depth+.015,height/2),(width/2-.008,.028,height-.025),'white',.004)
        rail((x+width/8,depth+.035,height*.35),(x+width/8,depth+.035,height*.65),.006,'brass')
    if sliding:
        box((width/2,depth+.05,.04),(width,.025,.02),'black',.002)
        box((width/2,depth+.05,height-.04),(width,.025,.02),'black',.002)

def kitchen_island():
    cabinet(1.6,.85,.8)
    box((.8,.43,.89),(1.70,.96,.06),'stone',.015)

def appliance(kind):
    w,d,h=(.60,.64,.86) if kind in ('washer','dishwasher') else (.70,.70,1.85)
    box((w/2,d/2,h/2),(w,d,h),'white',.035)
    box((w/2,d+.009,h*.94),(w-.04,.025,.065),'black',.006)
    if kind=='washer':
        outer=cylinder((w/2,d+.032,h*.48),.225,.028,'black');outer.rotation_euler.x=1.5707963
        inner=cylinder((w/2,d+.05,h*.48),.17,.03,'mirror');inner.rotation_euler.x=1.5707963
        cylinder((w*.78,d+.04,h*.94),.022,.012,'brass').rotation_euler.x=1.5707963
    elif kind=='dishwasher':
        rail((.06,d+.045,h-.11),(w-.06,d+.045,h-.11),.01,'black')
        box((w/2,d+.014,h/2),(w-.018,.022,h-.14),'stone',.005)
    else:
        for z,height in ((.5,.95),(1.42,.78)):
            box((w/2,d+.015,z),(w-.015,.03,height),'stone',.018)
            rail((.08,d+.05,z-height*.3),(.08,d+.05,z+height*.3),.01,'black')

def cooktop():
    box((.30,.26,.035),(.60,.52,.04),'black',.01)
    for x in (.15,.45):
        for y in (.14,.39):cylinder((x,y,.06),.092,.003,'mirror')
    for x in (.20,.28,.36,.44):cylinder((x,.04,.061),.012,.005,'white')

def hood():
    box((.4,.22,.05),(.8,.44,.10),'stone',.018)
    box((.4,.17,.37),(.3,.24,.60),'black',.014)
    for x in (.1,.7):cylinder((x,.38,.015),.025,.006,'white')

def vanity(width=.90):
    cabinet(width,.56,.46)
    box((width/2,.25,.60),(width+.025,.52,.06),'stone',.018)
    cylinder((width/2,.26,.66),.17,.06,'white',scale=(1.3,.85,1))
    rail((width*.75,.10,.63),(width*.75,.10,.86),.016,'brass')
    rail((width*.75,.10,.86),(width*.75,.27,.86),.016,'brass')

def toilet(wall_hung=False):
    if not wall_hung:box((.19,.26,.18),(.28,.35,.36),'white',.12)
    cylinder((.19,.37,.41),.19,.12,'white',scale=(1,1.4,1))
    cylinder((.19,.37,.477),.185,.018,'stone',scale=(1,1.4,1))
    if not wall_hung:
        box((.19,.10,.61),(.37,.19,.41),'white',.065)
        cylinder((.19,.10,.825),.025,.008,'brass')

def shower():
    rail((.07,.025,.30),(.07,.025,2.03),.015,'brass')
    rail((.07,.025,2.03),(.07,.32,2.03),.015,'brass')
    cylinder((.07,.32,2.00),.12,.025,'black')
    rail((.03,.05,.95),(.12,.05,.95),.018,'black')

def shower_screen():
    # A fixed panel with a fine frame; the translucent finish is assigned below.
    box((.45,.015,1.05),(.9,.03,2.1),'mirror',.005)
    for x in (.015,.885):box((x,.015,1.05),(.025,.035,2.1),'black',.003)

def curtain(width=2.40):
    rail((0,.08,2.65),(width,.08,2.65),.018,'black')
    for i in range(28):
        x=width*(i+.5)/28
        box((x,.10+(.04 if i%2 else 0),1.32),(width/28+.009,.035,2.56),'linen',.015)

def air_conditioner():
    box((.44,.12,.15),(.88,.24,.30),'white',.045)
    box((.44,.235,.07),(.76,.019,.052),'black',.008)
    for i in range(7):rail((.44,.248,.06+i*.004),(.80,.248,.06+i*.004),.001,'white')

def desk():
    legs(1.2,.55,.72,'black')
    box((.6,.275,.755),(1.2,.55,.04),'oak',.022)
    box((.98,.275,.50),(.36,.52,.41),'white',.01)
    for z in (.39,.53,.66):box((.98,.547,z),(.33,.022,.115),'stone',.008)

def dining_chair():
    legs(.48,.50,.44,'oak',.05)
    box((.24,.26,.465),(.49,.48,.09),'linen',.055)
    box((.24,.045,.71),(.49,.08,.40),'linen',.045)

def dining_rectangle():
    legs(1.6,.86,.73,'oak',.13)
    box((.8,.43,.77),(1.6,.86,.06),'stone',.028)

def shoe_storage():
    cabinet(.8,.95,.30)
    box((.4,.15,.98),(.82,.32,.035),'stone',.01)

def wall_shelf():
    for z in (0,.32,.64):
        box((.60,.13,z+.02),(1.20,.26,.04),'oak',.012)
    for x in (.06,1.14):box((x,.13,.35),(.035,.24,.70),'black',.004)

def laundry_sink():
    cabinet(.60,.79,.58)
    box((.3,.29,.82),(.65,.62,.07),'stone',.02)
    cylinder((.3,.3,.86),.20,.045,'white',scale=(1.1,.85,1))
    rail((.50,.12,.88),(.50,.12,1.11),.017,'black')
    rail((.50,.12,1.11),(.5,.29,1.11),.017,'black')

def pendant():
    rail((.16,.16,.25),(.16,.16,.70),.003,'black')
    cylinder((.16,.16,.15),.16,.25,'linen')
    cylinder((.16,.16,.025),.14,.008,'white')

def towel_rail():
    for x in (.025,.575):rail((x,.015,0),(x,.065,0),.012,'brass')
    rail((.025,.065,0),(.575,.065,0),.009,'brass')
    box((.25,.085,-.23),(.31,.03,.46),'linen',.014)

def main():
    rows=[
        ('home-kitchen-island','Ilha de cozinha com tampo claro','Cozinha','floor',0,kitchen_island),
        ('home-kitchen-base','Armário de cozinha com puxador fino','Cozinha','floor',0,cabinet),
        ('home-kitchen-tall','Torre de cozinha contemporânea','Cozinha','floor',0,lambda:cabinet(.60,2.20,.60)),
        ('home-kitchen-upper','Armário aéreo de cozinha','Cozinha','wall',1400,lambda:cabinet(.90,.65,.34)),
        ('home-wardrobe-sliding','Guarda-roupa com portas de correr','Dormitório','floor',0,lambda:cabinet(1.80,2.30,.64,True)),
        ('home-wardrobe-compact','Guarda-roupa compacto de duas portas','Dormitório','floor',0,lambda:cabinet(1.20,2.20,.60)),
        ('home-washer','Máquina de lavar frontal','Área de serviço','floor',0,lambda:appliance('washer')),
        ('home-dishwasher','Lava-louças com painel claro','Cozinha','floor',0,lambda:appliance('dishwasher')),
        ('home-fridge','Geladeira com acabamento claro','Cozinha','floor',0,lambda:appliance('fridge')),
        ('home-cooktop','Cooktop de indução com quatro zonas','Cozinha','surface',0,cooktop),
        ('home-hood','Coifa de linhas retas','Cozinha','wall',1600,hood),
        ('home-vanity','Gabinete de banheiro com cuba e torneira','Banheiro','wall',350,vanity),
        ('home-vanity-small','Gabinete compacto para lavabo','Banheiro','wall',350,lambda:vanity(.65)),
        ('home-toilet','Vaso sanitário contemporâneo','Banheiro','floor',0,toilet),
        ('home-toilet-wall','Vaso sanitário suspenso','Banheiro','wall',220,lambda:toilet(True)),
        ('home-shower','Chuveiro com coluna e controle','Banheiro','wall',250,shower),
        ('home-shower-screen','Painel de vidro para box','Banheiro','floor',0,shower_screen),
        ('home-curtain','Cortina de linho com pregas','Decoração','wall',0,curtain),
        ('home-curtain-small','Cortina de linho para janela compacta','Decoração','wall',0,lambda:curtain(1.50)),
        ('home-air-conditioner','Ar-condicionado de parede','Decoração','wall',2100,air_conditioner),
        ('home-desk','Escrivaninha compacta com gaveteiro','Escritório','floor',0,desk),
        ('home-chair','Cadeira de jantar estofada','Sala','floor',0,dining_chair),
        ('home-table','Mesa de jantar retangular com pedra','Sala','floor',0,dining_rectangle),
        ('home-shoe-storage','Sapateira estreita para entrada','Decoração','floor',0,shoe_storage),
        ('home-wall-shelf','Prateleiras de madeira com estrutura fina','Decoração','wall',1000,wall_shelf),
        ('home-laundry-sink','Tanque compacto com armário','Área de serviço','floor',0,laundry_sink),
        ('home-pendant','Pendente de tecido para sala','Decoração','ceiling',0,pendant),
        ('home-towel-rail','Toalheiro com toalha de linho','Banheiro','wall',1100,towel_rail),
    ]
    for row in rows:api['export_model'](*row)
    # Glass is separate from a mirror: preserve transmission in both editor and Cycles.
    for entry in api['CATALOG']:
        entry['date']='2026-10-03'
        if entry['id']=='home-shower-screen':
            import hashlib
            path=ROOT/'starter-models'/entry['recipe']['modelFile']
            model=json.loads(path.read_bytes())
            for part in model['parts']:
                if part['material']=='lmx-current-mirror':part['material']='lmx-home-glass'
            for mat in model['materials']:
                if mat['id']=='lmx-current-mirror':mat.update(id='lmx-home-glass',name='Vidro transparente',metallic=0,transmission=1,roughness=.04)
            payload=json.dumps(model,separators=(',',':')).encode();path.write_bytes(payload)
            entry['recipe']['parameters']['meshAsset']=hashlib.sha256(payload).hexdigest()
    for proof in api['PROVENANCE']:
        proof['generator']='scripts/create-home-collection.py'
        entry=next(e for e in api['CATALOG'] if e['id']==proof['id'])
        proof['preparedSha256']=entry['recipe']['parameters']['meshAsset']
    (ROOT/'starter-models/home-catalog.json').write_text(json.dumps(api['CATALOG'],ensure_ascii=False,indent=2),encoding='utf-8')
    (ROOT/'starter-models/home-provenance.json').write_text(json.dumps(api['PROVENANCE'],ensure_ascii=False,indent=2),encoding='utf-8')

if __name__=='__main__':main()
