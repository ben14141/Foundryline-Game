"""Render catalogue cutouts in an isolated Blender process; never writes source models."""
import bpy,math,json,os
from pathlib import Path
from mathutils import Vector,Matrix
R=Path(r'C:/Users/curly/Documents/Foundryline Game');O=R/'SourceAssets/UI/BuildCatalogue';O.mkdir(parents=True,exist_ok=True)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
s=bpy.context.scene;s.render.engine='CYCLES';s.cycles.samples=12;s.cycles.use_denoising=True
s.render.resolution_x=512;s.render.resolution_y=320;s.render.resolution_percentage=100;s.render.film_transparent=True;s.render.image_settings.file_format='PNG';s.render.image_settings.color_mode='RGBA';s.world.color=(.4,.4,.4)
s.view_settings.view_transform='AgX'
def mat(n,c,metal=0):
 m=bpy.data.materials.new(n);m.diffuse_color=(*c,1);m.use_nodes=True;p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*c,1);p.inputs['Metallic'].default_value=metal;p.inputs['Roughness'].default_value=.32;return m
orange=mat('Foundry orange',(.9,.25,.045));grey=mat('Graphite',(.12,.16,.19));concrete=mat('Concrete',(.58,.6,.6));blue=mat('Parts blue',(.07,.36,.67));green=mat('Finished green',(.1,.55,.32));glass=mat('Glazing',(.18,.58,.7),.35);white=mat('Paint white',(.92,.94,.92));yellow=mat('Safety yellow',(.98,.7,.03))
def box(n,loc,dim,m):
 bpy.ops.mesh.primitive_cube_add(size=1,location=loc);o=bpy.context.object;o.name=n;o.dimensions=dim;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(m);b=o.modifiers.new('Edge highlights','BEVEL');b.width=.025;b.segments=2;return o
def clear():
 for o in list(s.objects):bpy.data.objects.remove(o,do_unlink=True)
def append(path):
 with bpy.data.libraries.load(str(R/path),link=False) as (a,b):b.objects=a.objects
 for o in b.objects:
  if o and o.type in {'MESH','CURVE'} and not any(t in o.name.lower() for t in ['studioground','dimension','text_']):s.collection.objects.link(o)
def fbx(path):bpy.ops.import_scene.fbx(filepath=str(R/path))
def render(name):
 if os.environ.get('CATALOG_ONLY') and os.environ['CATALOG_ONLY']!=name:return
 obs=[o for o in s.objects if o.type in {'MESH','CURVE'} and not o.hide_render]
 pts=[o.matrix_world@Vector(c) for o in obs for c in o.bound_box];lo=Vector(tuple(min(p[i] for p in pts) for i in range(3)));hi=Vector(tuple(max(p[i] for p in pts) for i in range(3)));center=(lo+hi)/2;radius=max((hi-lo).length,.1)
 bpy.ops.object.camera_add(location=center+Vector((1.25,-1.7,1.2)).normalized()*radius*2);cam=bpy.context.object;cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';s.camera=cam
 bpy.context.view_layer.update();inv=cam.matrix_world.inverted();q=[inv@p for p in pts];w=max(p.x for p in q)-min(p.x for p in q);h=max(p.y for p in q)-min(p.y for p in q);cam.data.ortho_scale=max(w,h*1.6)*1.18
 for pos,power,size in [((1,-2,3),85,4),((-2,-1,1),45,3),((1,2,2),70,3)]:
  bpy.ops.object.light_add(type='AREA',location=center+Vector(pos)*radius*.65);l=bpy.context.object;l.data.energy=power*radius*radius;l.data.shape='DISK';l.data.size=size*radius*.5;l.rotation_euler=(center-l.location).to_track_quat('-Z','Y').to_euler()
 s.render.filepath=str(O/('T_Catalog_'+name+'.png'));bpy.ops.render.render(write_still=True);print('CATALOG_RENDERED '+name,flush=True)
def sample(kind):
 if kind in ['Building','Shop2','OfficeArea','OfficeRoom','Columns']:
  h=3 if kind in ['OfficeArea','OfficeRoom'] else 6 if kind!='Shop2' else 10
  box('Floor',(0,0,.1),(10,8,.2),concrete)
  for x in [-5,5]:box('Exterior',(x,0,h/2),(.2,8,h),orange if kind not in ['OfficeArea','OfficeRoom'] else white);box('Cap',(x,0,h),(.3,8,.2),grey)
  box('Rear',(0,4,h/2),(10,.2,h),orange)
  for x in [-3,0,3]:box('Upper windows',(x,3.86,h*.75),(2,.08,.8),glass)
  if kind=='OfficeRoom':box('Partition',(0,0,1.5),(.15,7,3),white)
  if kind=='Columns':
   for y in [-3,0,3]:box('Column',(-4.7,y,h/2),(.3,.3,h),grey)
 elif kind in ['Personnel','RoomDoor','Garage','TruckDoor','LoadingBay','RoomWindow','OfficeGlass']:
  window=kind in ['RoomWindow','OfficeGlass'];w=1.2 if kind in ['Personnel','RoomDoor'] else 4;h=2.3 if w<2 else 3.8
  if kind=='RoomWindow':h=1.6
  box('Frame',(0,0,h/2),(w+.2,.28,h+.2),grey);box('Panel',(0,-.16,h/2),(w,.08,h),glass if window else orange if w<2 else concrete)
  if w<2:box('Handle',(w*.35,-.25,1),(.06,.12,.25),grey)
  elif not window:
   for z in range(1,9):box('Door seam',(0,-.22,z*h/9),(w,.015,.025),grey)
  if kind=='LoadingBay':
   box('Bay apron',(0,-2,-.08),(5,4,.16),concrete)
   for x in [-2.3,2.3]:box('Bollard',(x,-.6,.6),(.16,.16,1.2),yellow);box('Paint',(x,-2,.015),(.08,3,.025),yellow)
 else:
  box('Paved sample',(0,0,-.08),(8,6,.16),grey if kind!='Walkway' else concrete)
  if kind in ['Rect','Polygon']:return
  if kind in ['Mark0','Parking']:
   for x in [-3,-1,1,3]:box('Space line',(x,1,.015),(.06,3,.025),white)
  elif kind in ['Mark1','Mark2']:box('Paint',(0,0,.015),(7,.07,.025),white if kind=='Mark1' else yellow)
  elif kind in ['Mark4','RoadOneWay']:
   box('Arrow stem',(0,0,.02),(3,.12,.03),white)
   for a in [-.7,.7]:o=box('Arrow tip',(1.2,a*.5,.02),(1.1,.12,.03),white);o.rotation_euler.z=a
  elif kind in ['Overlay','NetNew','NetPanel']:
   for x in [-2.5,0,2.5]:
    bpy.ops.mesh.primitive_uv_sphere_add(segments=16,ring_count=8,radius=.26,location=(x,0,.25));bpy.context.object.data.materials.append(green)
   box('Route',(0,0,.1),(5,.14,.16),green)
  else:
   for y in [-2.7,2.7]:box('Curb',(0,y,.06),(8,.2,.22),concrete)
   for x in [-3,-1,1,3]:box('Road paint',(x,0,.02),(.9,.08,.025),yellow)
   if kind=='ToolForklift':
    for y in [-1.6,1.6]:box('Lane edge',(0,y,.02),(8,.08,.025),yellow)
manifest=[]
for n in ['Building','Shop2','OfficeArea','OfficeRoom','Columns','Personnel','RoomDoor','Garage','TruckDoor','LoadingBay','RoomWindow','OfficeGlass','Rect','Polygon','RoadStandard','RoadOneWay','RoadTruck','Parking','Walkway','Mark0','Mark1','Mark2','Mark4','Overlay','NetNew','NetPanel','ToolForklift']:
 clear();sample(n);render(n);manifest.append(n)
for levels in [1,2,3]:
 for typ,folder in [('Parts','ModularPalletRacking'),('Goods','WideBayPalletRacking')]:
  clear();append(Path('SourceAssets/Inventory/Storage')/folder/('PalletRack_'+('1Level' if levels==1 else str(levels)+'Levels')+'.blend'));render(typ+str(levels));manifest.append(typ+str(levels))
for n,path in [('Sign','SourceAssets/Buildings/ManufacturingSign/Foundryline_ManufacturingSign.blend'),('Workstation','SourceAssets/Production/Equipment/WeldingBench/Foundryline_StandardWeldingBench.blend')]:
 clear()
 if n=='Workstation':
  def add_fit(path,width,target):
   old=set(s.objects);fbx(Path(path));bpy.context.view_layer.update();objs=[o for o in s.objects if o not in old and o.type=='MESH'];pts=[o.matrix_world@Vector(c) for o in objs for c in o.bound_box];lo=Vector(tuple(min(p[i] for p in pts) for i in range(3)));hi=Vector(tuple(max(p[i] for p in pts) for i in range(3)));f=width/(hi.x-lo.x);center=Vector(((lo.x+hi.x)/2,(lo.y+hi.y)/2,lo.z));trans=Matrix.Translation(Vector(target))@Matrix.Scale(f,4)@Matrix.Translation(-center);matrices=[(o,trans@o.matrix_world.copy()) for o in objs]
   for o,m in matrices:o.parent=None;o.matrix_world=m
  add_fit('SourceAssets/Production/Equipment/WeldingBench/StandardWeldingBench_SeparateComponents.fbx',2.44,(0,0,0))
  add_fit('SourceAssets/Production/Equipment/Welder/QualityReview_2026-10-29/Welder_Complete_Review.fbx',.7,(1.8,.2,0))
  add_fit('SourceAssets/Production/AssemblySupply/AssemblySupplyBin_4.fbx',1.1,(-1.8,.3,0))
  for x in [-2.8,2.8]:box('Zone boundary',(x,0,.012),(.07,3.9,.024),yellow)
  for y in [-1.95,1.95]:box('Zone boundary',(0,y,.012),(5.6,.07,.024),yellow)
 else:append(Path(path))
 render(n);manifest.append(n)
clear();fbx(Path('SourceAssets/Vehicles/Forklift/QualityReview_2026-10-29/Forklift_Complete_Review.fbx'));render('Forklift');manifest.append('Forklift')
for n,path in [('Stored1','SourceAssets/Production/Equipment/PlasmaTable/PlasmaTable_Complete.fbx'),('Stored2','SourceAssets/Production/Equipment/PressBrake/PressBrake_Complete.fbx'),('Stored3','SourceAssets/Production/Equipment/SheetRoller/SheetRoller_Complete.fbx')]:
 clear();fbx(Path(path));render(n);manifest.append(n)
(O/'manifest.json').write_text(json.dumps(manifest,indent=2))
