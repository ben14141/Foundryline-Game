"""Original 1:1 collector cart and modular assembly supply bins. Background Blender only."""
import bpy, math, json
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'SourceAssets/Production/AssemblySupply'
OUT.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
S=bpy.context.scene
S.unit_settings.system='IMPERIAL'
S.unit_settings.scale_length=1
def material(n,c,metal=0,rough=.4):
 m=bpy.data.materials.new(n);m.use_nodes=True
 p=next(n for n in m.node_tree.nodes if n.type=='BSDF_PRINCIPLED')
 p.inputs['Base Color'].default_value=(*c,1);p.inputs['Metallic'].default_value=metal;p.inputs['Roughness'].default_value=rough
 return m
orange=material('Supply_FoundryOrange',(.8,.19,.025),.4,.28)
blue=material('Supply_PartsBlue',(.025,.18,.31),.3,.32)
steel=material('Supply_GalvanizedSteel',(.48,.55,.60),.85,.27)
dark=material('Supply_Graphite',(.035,.045,.055),.5,.33)
rubber=material('Supply_CasterRubber',(.012,.018,.022),0,.7)
white=material('Supply_Label',(.91,.92,.87),0,.5)
groups={}; current=[]; wheel_sets=[]; caster_sets=[]
def box(n,p,d,m,b=.003):
 bpy.ops.mesh.primitive_cube_add(size=1,location=p);o=bpy.context.object;o.name=n;o.dimensions=d
 bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(m)
 if b:
  mod=o.modifiers.new('Manufactured edge','BEVEL');mod.width=b;mod.segments=2
  o.modifiers.new('Weighted face normals','WEIGHTED_NORMAL')
 current.append(o);return o
def cylinder(n,p,r,depth,m,axis='Z',verts=24):
 bpy.ops.mesh.primitive_cylinder_add(vertices=verts,radius=r,depth=depth,location=p)
 o=bpy.context.object;o.name=n
 if axis=='Y':o.rotation_euler.x=math.pi/2
 if axis=='X':o.rotation_euler.y=math.pi/2
 o.data.materials.append(m)
 for f in o.data.polygons:f.use_smooth=len(f.vertices)==4
 current.append(o);return o
def label(text,p,size=.035):
 bpy.ops.object.text_add(location=p,rotation=(math.pi/2,0,0));o=bpy.context.object;o.name='Printed_'+text;o.data.body=text;o.data.size=size;o.data.extrude=0;o.data.align_x='CENTER';o.data.materials.append(white)
 bpy.ops.object.convert(target='MESH');current.append(bpy.context.object)
def bolt(p):cylinder('Hex fixing',p,.007,.006,steel,'Y',6)
def tray(n,x,y,z,w,d,h):
 box(n+'_bottom',(x,y,z+.003),(w,d,.006),blue)
 box(n+'_back',(x,y+d/2-.003,z+h/2),(w,.006,h),blue)
 for sx in [-1,1]:box(n+'_side',(x+sx*(w/2-.003),y,z+h/2),(.006,d,h),blue)
 box(n+'_front',(x,y-d/2+.003,z+h*.23),(w,.006,h*.46),blue)
 box(n+'_label_holder',(x,y-d/2-.004,z+h*.23),(.18,.003,.055),dark,.001)
 label(n,(x,y-d/2-.006,z+h*.23-.012),.022)
def export(name):
 groups[name]=list(current)
 bpy.ops.object.select_all(action='DESELECT')
 for o in current:o.select_set(True)
 bpy.context.view_layer.objects.active=current[0]
 bpy.ops.export_scene.fbx(filepath=str(OUT/(name+'.fbx')),use_selection=True,object_types={'MESH'},apply_unit_scale=True,axis_forward='-Y',axis_up='Z',bake_anim=False)
 current.clear()
# 48 x 28 in cart, 39 in handle. Frame/casters are separate for runtime wheel rotation.
L=1.2192;W=.7112
for x in [-L/2+.085,L/2-.085]:
 for y in [-W/2+.07,W/2-.07]:
  begin=len(current)
  cylinder('Rubber wheel',(x,y,.0762),.0762,.045,rubber,'Y',32)
  cylinder('Wheel hub',(x,y-.025,.0762),.026,.008,steel,'Y')
  wheel_sets.append((list(current[begin:]),Vector((x,y,.0762))))
  begin=len(current)
  for side in [-1,1]:box('Caster fork',(x,y+side*.032,.13),(.065,.009,.11),steel)
  cylinder('Swivel bearing',(x,y,.194),.04,.025,steel)
  caster_sets.append((list(current[begin:]),Vector((x,y,.194))))
for z in [.24,.69]:
 box('Cart deck',(0,0,z),(L,W,.025),steel)
 for y in [-W/2,W/2]:box('Deck edge',(0,y,z+.035),(L,.025,.07),orange)
 for x in [-L/2,L/2]:box('Deck end',(x,0,z+.035),(.025,W,.07),orange)
for x in [-L/2+.025,L/2-.025]:
 for y in [-W/2+.025,W/2-.025]:box('Frame upright',(x,y,.48),(.035,.035,.60),orange)
for y in [-.25,.25]:
 cylinder('Handle post',(-L/2-.04,y,.84),.017,.30,steel)
cylinder('Push handle',(-L/2-.04,0,.99),.022,.55,rubber,'Y')
for x in [-.39,0,.39]:tray('CART BIN',x,0,.715,.36,.56,.19)
box('Capacity plate',(0,-W/2-.015,.57),(.48,.008,.095),dark)
label('PARTS COLLECTOR',(0,-W/2-.02,.575),.035)
label('300 LB PAYLOAD',(0,-W/2-.02,.543),.022)
for x in [-.5,.5]:
 for z in [.29,.73]:bolt((x,-W/2-.018,z))
export('CollectorCart')
cart_original=list(groups['CollectorCart']);moving=[o for pieces,pivot in wheel_sets+caster_sets for o in pieces]
current.extend([o for o in cart_original if o not in moving]);export('CollectorCart_Body')
for name,sets in [('Wheel',wheel_sets),('Caster',caster_sets)]:
 pieces,pivot=sets[0]
 # Shared wheel/caster geometry is exported around its real rotation pivot.
 for o in pieces:o.location-=pivot
 current.extend(pieces);export('CollectorCart_'+name)
 for o in pieces:o.location+=pivot
# Compartment openings remain constant; upgrades add columns and consume floor width.
variants=[]
for count in [4,6,8]:
 cols=count//2;pitch=.462;w=cols*pitch+.06;d=.6096
 for x in [-w/2+.025,w/2-.025]:
  for y in [-d/2+.025,d/2-.025]:
   box('Supply upright',(x,y,.53),(.04,.04,1.06),orange)
   box('Anchored foot',(x,y,.012),(.11,.11,.024),steel)
 for z in [.34,.73,1.08]:
  box('Cross beam',(0,d/2-.02,z),(w,.04,.05),orange)
  box('Front beam',(0,-d/2+.02,z),(w,.04,.05),orange)
  if z<1:box('Shelf',(0,0,z),(w-.05,d-.04,.025),steel)
 for row in range(2):
  for col in range(cols):tray('PART %02d'%(row*cols+col+1),(col-(cols-1)/2)*pitch,0,.36+row*.39,.45,.55,.30)
 box('Supply header',(0,-d/2,1.14),(w,.018,.12),dark)
 label('ASSEMBLY SUPPLY | %d COMPARTMENTS'%count,(0,-d/2-.010,1.13),.031)
 variants.append({'compartments':count,'width_cm':w*100,'depth_cm':d*100,'height_cm':120,'compartment_payload_lb':100,'total_payload_lb':count*100})
 export('AssemblySupplyBin_%d'%count)
# Canonical source: each exported asset remains at its own origin in a separate collection.
for name,objects in groups.items():
 if name.startswith('CollectorCart_'):continue
 coll=bpy.data.collections.new(name);S.collection.children.link(coll)
 for o in objects:
  for old in list(o.users_collection):old.objects.unlink(o)
  coll.objects.link(o)
 coll.hide_render=True
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'AssemblySupply_Original.blend'))
# Review arrangement only; exports and saved canonical source retain correct pivots.
offsets={'CollectorCart':(-2.1,-1.1,0),'AssemblySupplyBin_4':(-2.1,1,0),'AssemblySupplyBin_6':(0,1,0),'AssemblySupplyBin_8':(2.3,1,0)}
for name,objects in groups.items():
 if name.startswith('CollectorCart_'):continue
 bpy.data.collections[name].hide_render=False
 for o in objects:o.location+=Vector(offsets[name])
box('Review ground',(0,0,-.045),(200,200,.08),material('ReviewFloor',(.09,.105,.12)))
def aim(o,p):o.rotation_euler=(Vector(p)-o.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add(location=(5.8,-8.5,6));S.camera=bpy.context.object;aim(S.camera,(.1,.6,.55));S.camera.data.type='ORTHO';S.camera.data.ortho_scale=8.1
for p,power,size in [((0,-3,6),1700,5),((4,3,5),1400,4),((-4,1,4),1100,3)]:
 bpy.ops.object.light_add(type='AREA',location=p);o=bpy.context.object;o.data.energy=power;o.data.shape='DISK';o.data.size=size;aim(o,(0,0,.5))
S.render.engine='CYCLES';S.cycles.device='CPU';S.cycles.samples=24;S.render.threads_mode='FIXED';S.render.threads=2
S.render.resolution_x=1500;S.render.resolution_y=1000;S.render.resolution_percentage=100
S.world=bpy.data.worlds.new('SupplyReviewWorld');S.world.color=(.18,.18,.18);S.render.image_settings.file_format='PNG';S.render.filepath=str(OUT/'AssemblySupply_Overview.png')
bpy.ops.render.render(write_still=True)
manifest={'original_assets_only':True,'units':'metres; FBX exports centimetres via unit scale','cart':{'length_in':48,'width_in':28,'handle_height_in':39,'payload_lb':300},'bins':variants,'exports':list(groups),'notes':['All bin compartments have bottoms, backs, sidewalls and low front lips.','Bin upgrades add physical width.','Cart wheels retain separate object pivots.','Ratings are provisional gameplay values.']}
(OUT/'manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print('ASSEMBLY_SUPPLY_MODELS_READY')
