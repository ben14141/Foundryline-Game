"""Editable rigid-part employee review rig. Runtime adapter remains authoritative art implementation."""
import bpy,math,json
from pathlib import Path
from mathutils import Vector
R=Path(r'C:/Users/curly/Documents/Foundryline Game');OUT=R/'SourceAssets/Employees/Prototype';OUT.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True);s=bpy.context.scene;s.unit_settings.system='METRIC';s.unit_settings.scale_length=1;s.render.fps=30;s.frame_end=60
root=bpy.data.objects.new('Employee_Control_Root',None);s.collection.objects.link(root);root.empty_display_type='ARROWS';root.empty_display_size=.25
materials={}
for name,col in [('Graphite',(.045,.06,.075)),('FoundryOrange',(.82,.24,.04)),('Skin',(.48,.28,.16))]:
 m=bpy.data.materials.new(name);m.diffuse_color=(*col,1);m.use_nodes=True;m.node_tree.nodes.get('Principled BSDF').inputs['Base Color'].default_value=(*col,1);materials[name]=m

def joint(name,parent,loc):
 o=bpy.data.objects.new(name,None);s.collection.objects.link(o);o.parent=parent;o.location=Vector(loc)/100;o.empty_display_size=.07;return o

def box(name,parent,loc,size,mat):
 bpy.ops.mesh.primitive_cube_add(size=1);o=bpy.context.object;o.name=name;o.parent=parent;o.location=Vector(loc)/100;o.scale=Vector(size)/100;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(materials[mat]);return o

channels=[];slotmaps={}
def drive(o,path,index,key,scale=1):
 root[key]=0.;channels.append((o,path,index,key,scale))
def activate(mode):
 action=bpy.data.actions['Employee_'+mode]
 for o in [root]+list(dict.fromkeys(c[0] for c in channels)):
  o.animation_data_create();o.animation_data.action=action;o.animation_data.action_slot=slotmaps[mode][o.name]

hip=joint('Pelvis',root,(0,0,92));drive(hip,'location',2,'hip_height_cm',.01)
box('Hips',hip,(0,0,4),(24,30,20),'Graphite');box('HiVisTorso',hip,(0,0,32),(24,39,38),'FoundryOrange');box('VestStripe',hip,(12.2,0,33),(.6,40,3),'Graphite');box('Neck',hip,(0,0,54),(10,10,8),'Skin');box('Head',hip,(0,0,69),(19,19,24),'Skin');box('Face',hip,(10,0,70),(2,12,4),'Graphite');box('Hardhat',hip,(0,0,85),(24,24,6),'FoundryOrange');box('Brim',hip,(3,0,81),(31,28,2),'FoundryOrange')
for i,side in enumerate([-1,1]):
 tag=['Left','Right'][i];t=joint(tag+'Thigh',hip,(0,side*10,0));box(tag+'TrouserThigh',t,(0,0,-21.5),(17,17,46),'Graphite');k=joint(tag+'Knee',t,(0,0,-43));box(tag+'TrouserShin',k,(0,0,-21.5),(15,15,45),'Graphite');f=joint(tag+'Ankle',k,(0,0,-43));box(tag+'Boot',f,(5,0,-1.5),(29,15,11),'Graphite')
 a=joint(tag+'Shoulder',hip,(0,side*24,51));box(tag+'Sleeve',a,(0,0,-15.5),(13,13,34),'FoundryOrange');e=joint(tag+'Elbow',a,(0,0,-31));box(tag+'Forearm',e,(0,0,-14),(11,11,30),'FoundryOrange');h=joint(tag+'Wrist',e,(0,0,-28));box(tag+'Glove',h,(0,0,-5),(10,10,12),'Graphite')
 for obj,key in [(t,'thigh'),(k,'knee'),(f,'ankle'),(a,'shoulder'),(e,'elbow'),(h,'wrist')]:drive(obj,'rotation_euler',1,tag+'_'+key,-math.pi/180)
 drive(a,'rotation_euler',2,tag+'_shoulder_yaw',math.pi/180)

def leg(hip,x,z):
 h=hip-z;d=min(85.99,max(.1,math.hypot(x,h)));a=math.atan2(x,h);b=math.acos(d/86);return [math.degrees(a+b),math.degrees(-2*b),math.degrees(-a+b)]
def arm(hip,i,wrist):
 side=[-1,1][i];dx=wrist[0];dy=wrist[1]-side*24;h=hip+51-wrist[2];x=math.hypot(dx,dy);d=min(58.99,max(3.01,math.hypot(x,h)));a=math.atan2(x,h);b=math.acos(max(-1,min(1,(31*31+d*d-28*28)/(62*d))));k=math.pi-math.acos(max(-1,min(1,(31*31+28*28-d*d)/(2*31*28))));p=math.degrees(a-b);e=math.degrees(k);return p,e,90-p-e,math.degrees(math.atan2(dy,dx))
checks={'standing_height_cm':180,'actions':[],'walking_foot_contact':True,'runtime_import_required':False}
for mode in ['Idle','Walk','Operate','Seated']:
 action=bpy.data.actions.new('Employee_'+mode);action.use_fake_user=True;slotmaps[mode]={}
 for o in [root]+list(dict.fromkeys(c[0] for c in channels)):slotmaps[mode][o.name]=action.slots.new('OBJECT',o.name)
 activate(mode)
 for frame in range(1,62):
  phase=(frame-1)/60;hipz=148 if mode=='Seated' else 88 if mode=='Walk' else 92;root['hip_height_cm']=hipz
  for i,tag in enumerate(['Left','Right']):
   p=(phase+i*.5)%1;x=0;z=7
   if mode=='Walk':
    if p<.5:x=27.5-110*p
    else:t=(p-.5)*2;x=-27.5+55*t;z=7+8*math.sin(math.pi*t)
   if mode=='Seated':x=50;z=90
   angles=leg(hipz,x,z)
   for key,value in zip(['thigh','knee','ankle'],angles):root[tag+'_'+key]=value
   if mode in ['Operate','Seated']:
    tap=1.5*math.sin(phase*5+i*math.pi);wrist=(43,[-1,1][i]*18,174) if mode=='Seated' else (36+tap,[-1,1][i]*15,113+tap);ap=arm(hipz,i,wrist)
   else:ap=(-25*math.sin(2*math.pi*phase+i*math.pi) if mode=='Walk' else 0,8,0,0)
   for key,value in zip(['shoulder','elbow','wrist','shoulder_yaw'],ap):root[tag+'_'+key]=value
  for o,path,index,key,scale in channels:
   getattr(o,path)[index]=root[key]*scale;o.keyframe_insert(data_path=path,index=index,frame=frame)
  for key in root.keys():root.keyframe_insert(data_path='["'+key+'"]',frame=frame)
 checks['actions'].append({'name':action.name,'frames':61,'duration_s':2})
# Validate the actual evaluated hierarchy, not just the input trigonometry.
contacts=[];knees=[]
activate('Walk')
for frame in range(1,61):
 s.frame_set(frame);bpy.context.view_layer.update()
 z=[bpy.data.objects[tag+'Ankle'].matrix_world.translation.z*100-7 for tag in ['Left','Right']]
 contacts.append(z);knees.append(bpy.data.objects['LeftKnee'].rotation_euler.y)
checks['walking_foot_contact']=all(min(z)>-.02 and min(z)<.02 and max(z)<=8.02 for z in contacts)
checks['walking_knee_actually_moves']=max(knees)-min(knees)>.1
activate('Idle');s.frame_set(1);bpy.context.view_layer.update()
points=[o.matrix_world@Vector(v) for o in bpy.data.objects if o.type=='MESH' for v in o.bound_box]
checks['measured_standing_height_cm']=round((max(v.z for v in points)-min(v.z for v in points))*100,3)
checks['scale_is_180cm']=abs(checks['measured_standing_height_cm']-180)<.02
assert checks['walking_foot_contact'] and checks['walking_knee_actually_moves'] and checks['scale_is_180cm'],checks
# Source review instructions travel with the .blend, including all animation controls.
text=bpy.data.texts.new('README');text.write('FOUNDRYLINE prototype employee\n180cm standing, +X forward, root on floor.\nSelect Employee_Control_Root; use Action Editor to select Employee_Idle / Walk / Operate / Seated.\nRigid articulated parts have layered action slots. Root properties record reference pose values; edit joint rotations and action curves directly. Use the included ActivatePose.py text to switch all slots together. Walk is in-place for game distance-driven travel.\nSeated root is at forklift X=-106cm; cushion141cm, floor83cm, hips148cm.\nRuntime uses equivalent procedural parts, not this .blend. Final skeletal character plugs into EmployeeAppearance.\n')
switch=bpy.data.texts.new('ActivatePose.py');switch.write('import bpy\nmode="Walk" # Idle, Walk, Operate, Seated\na=bpy.data.actions["Employee_"+mode]\nfor o in bpy.data.objects:\n slot=next((s for s in a.slots if s.identifier=="OB"+o.name),None)\n if slot:\n  o.animation_data_create();o.animation_data.action=a;o.animation_data.action_slot=slot\nbpy.context.scene.frame_set(1)\n')
# Neutral floor, camera and studio illumination for independent source inspection.
box('ReviewFloor',None,(0,0,-2),(500,500,4),'Graphite')
bpy.ops.object.camera_add(location=(3,-4,2.7));camera=bpy.context.object;camera.rotation_euler=(Vector((0,0,.95))-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.type='ORTHO';camera.data.ortho_scale=2.6;s.camera=camera
for loc,power,size in [((2,-3,4),700,4),((-2,1,3),500,3)]:bpy.ops.object.light_add(type='AREA',location=loc);bpy.context.object.data.energy=power;bpy.context.object.data.shape='DISK';bpy.context.object.data.size=size
s.render.engine='CYCLES';s.cycles.samples=24;s.render.resolution_x=700;s.render.resolution_y=800;s.render.resolution_percentage=100;s.world=bpy.data.worlds.new('ReviewWorld');s.world.color=(.18,.18,.18)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Foundryline_PrototypeEmployee.blend'));s.render.filepath=str(OUT/'PrototypeEmployee_Idle.png');bpy.ops.render.render(write_still=True)
(OUT/'rig_manifest.json').write_text(json.dumps(checks,indent=2))
