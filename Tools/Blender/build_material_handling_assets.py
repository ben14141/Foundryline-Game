import bpy,bmesh,math,json
from pathlib import Path
from mathutils import Vector,Matrix
R=Path(r'C:\Users\curly\Documents\Foundryline Game');SRC=R/'SourceAssets/MaterialHandling';OUT=SRC/'Authored';OUT.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True);scene=bpy.context.scene;scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1;scene.render.fps=30;scene.frame_end=240
materials={};manifest=[]
def mat(name,color,rough=.5,metal=0,texture=None):
 m=bpy.data.materials.new(name);m.use_nodes=True;bs=next(n for n in m.node_tree.nodes if n.type=='BSDF_PRINCIPLED');bs.inputs['Base Color'].default_value=(*color,1);bs.inputs['Roughness'].default_value=rough;bs.inputs['Metallic'].default_value=metal
 if texture:
  for suffix,slot in [('BaseColor','Base Color'),('Roughness','Roughness'),('Normal','Normal')]:
   p=SRC/'Textures'/('T_'+texture+'_'+suffix+'.png');n=m.node_tree.nodes.new('ShaderNodeTexImage');n.image=bpy.data.images.load(str(p));n.image.pack()
   if suffix!='BaseColor':n.image.colorspace_settings.name='Non-Color'
   if suffix=='Normal':
    bump=m.node_tree.nodes.new('ShaderNodeNormalMap');bump.inputs['Strength'].default_value=.25;m.node_tree.links.new(n.outputs['Color'],bump.inputs['Color']);m.node_tree.links.new(bump.outputs['Normal'],bs.inputs[slot])
   else:m.node_tree.links.new(n.outputs['Color'],bs.inputs[slot])
 materials[name]=m;return m
orange=mat('MH_FoundryOrange',(.78,.24,.055),.45,.2);graphite=mat('MH_Graphite',(.055,.069,.082),.57,.3);steel=mat('MH_MachinedSteel',(.43,.48,.52),.26,.8);dark=mat('MH_DarkSteel',(.095,.12,.145),.4,.65);rubber=mat('MH_Rubber',(.016,.021,.025),.88);white=mat('MH_White',(.86,.88,.82),.38);red=mat('MH_Red',(.6,.018,.01),.35);yellow=mat('MH_SafetyYellow',(.88,.58,.035),.5);glass=mat('MH_Glass',(.025,.06,.075),.12,.2);wood=mat('MH_PalletWood',(.55,.35,.15),.8,0,'PalletWood');sheetmat=mat('MH_CarbonSteel',(.4,.42,.44),.43,.8,'SteelSheet')

def box(name,loc,size,m,rot=(0,0,0),bevel=0):
 bpy.ops.mesh.primitive_cube_add(size=1,location=loc);o=bpy.context.object;o.name=name;o.dimensions=size;o.rotation_euler=rot;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(m)
 if bevel:
  mod=o.modifiers.new('Soft manufactured edges','BEVEL');mod.width=bevel;mod.segments=2;bpy.context.view_layer.objects.active=o;bpy.ops.object.modifier_apply(modifier=mod.name)
 return o

def cyl(name,loc,radius,depth,m,rot=(math.pi/2,0,0),verts=24):
 bpy.ops.mesh.primitive_cylinder_add(vertices=verts,radius=radius,depth=depth,location=loc,rotation=rot);o=bpy.context.object;o.name=name;o.data.materials.append(m)
 for p in o.data.polygons:p.use_smooth=len(p.vertices)==4
 return o

def join(name,items,pivot=(0,0,0)):
 bpy.ops.object.select_all(action='DESELECT')
 for o in items:o.select_set(True)
 bpy.context.view_layer.objects.active=items[0];bpy.ops.object.join();o=bpy.context.object;o.name=name
 bpy.context.scene.cursor.location=pivot;bpy.ops.object.origin_set(type='ORIGIN_CURSOR');return o

def label(name,text,loc,rot,size=.08,m=None):
 bpy.ops.object.text_add(location=loc,rotation=rot);o=bpy.context.object;o.name=name;o.data.body=text;o.data.size=size;o.data.extrude=.0005;o.data.materials.append(m or white);bpy.ops.object.convert(target='MESH');return bpy.context.object

def uv_planar(o):
 uv=o.data.uv_layers.active or o.data.uv_layers.new(name='UVMap')
 for p in o.data.polygons:
  axis=max(range(3),key=lambda i:abs(p.normal[i]));axes=[i for i in range(3) if i!=axis]
  for loop in p.loop_indices:
   v=o.data.vertices[o.data.loops[loop].vertex_index].co;uv.data[loop].uv=(v[axes[0]],v[axes[1]]*3)

def register(o,folder,kind='static',**extra):
 uv_planar(o);bpy.context.view_layer.update();vs=[o.matrix_world@Vector(c) for c in o.bound_box];lo=Vector([min(v[k] for v in vs) for k in range(3)]);hi=Vector([max(v[k] for v in vs) for k in range(3)])
 entry={'name':o.name,'folder':folder,'kind':kind,'position_cm':[round(v*100,5) for v in o.location],'size_cm':[round(v*100,5) for v in hi-lo],**extra};manifest.append(entry);return o

def import_factory(name):
 before=set(bpy.data.objects);bpy.ops.import_scene.fbx(filepath=str(SRC/'FactorySource'/(name+'.fbx')));bpy.context.view_layer.update();new=set(bpy.data.objects)-before
 for o in list(new):
  if o.type=='MESH' and o.name.startswith(('UCX_','UBX_','USP_','UCP_')):bpy.data.objects.remove(o,do_unlink=True)
 o=next(o for o in bpy.data.objects if o.name==name);M=o.matrix_world.copy();o.parent=None;o.data.transform(M);o.matrix_world=Matrix.Identity(4);return o

# Recover connected source parts for independently pivoted wheels and mast pieces.
source=import_factory('SM_Forklift_01');bm=bmesh.new();bm.from_mesh(source.data);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=.00001);bm.to_mesh(source.data);bm.free()
verts=source.data.vertices;adj=[[] for _ in verts]
for e in source.data.edges:a,b=e.vertices;adj[a].append(b);adj[b].append(a)
seen=set();groups=[]
for i in range(len(adj)):
 if i in seen:continue
 stack=[i];seen.add(i);ids=[]
 while stack:
  n=stack.pop();ids.append(n)
  for j in adj[n]:
   if j not in seen:seen.add(j);stack.append(j)
 v=[verts[j].co for j in ids];lo=Vector([min(c[k] for c in v) for k in range(3)]);hi=Vector([max(c[k] for c in v) for k in range(3)]);groups.append((ids,lo,hi,(lo+hi)/2))
wheelcenters=[c for ids,lo,hi,c in groups if len(ids)==96 and c.z<1 and .6<hi.x-lo.x<.9]
assert len(wheelcenters)==4
buckets={'Body':[],'MastOuter':[],'MastInner':[],'Carriage':[]};wheelgroups=[[] for _ in wheelcenters]
for g in groups:
 ids,lo,hi,c=g;d=hi-lo;w=next((i for i,p in enumerate(wheelcenters) if (c-p).length<.13 and abs(d.x-d.z)<.08 and d.x>.55),None)
 if w is not None:wheelgroups[w].append(g)
 elif hi.x<-.85 and hi.z<1.5:buckets['Carriage'].append(g)
 elif hi.x<0 and c.x<-.44:
  buckets['MastInner' if c.x<-.6 else 'MastOuter'].append(g)
 else:buckets['Body'].append(g)
M=Matrix.Rotation(math.pi,4,'Z');origin=Vector((.334,0,-.018))
def canonical(v):return M@v-origin
folder='Vehicles/Forklift/FL01'
objects={}
def make_groups(name,gs,pivot,default):
 ids=[i for g in gs for i in g[0]];mapping={old:new for new,old in enumerate(ids)};points=[canonical(verts[j].co)-Vector(pivot) for j in ids];faces=[];mats=[];glookup={j:g for g in gs for j in g[0]}
 for p in source.data.polygons:
  if not all(j in mapping for j in p.vertices):continue
  faces.append([mapping[j] for j in p.vertices]);g=glookup[p.vertices[0]];c=g[3];d=g[2]-g[1];m=default
  if name=='Forklift_Body':
   if c.z>2.25 or (c.z>1.1 and abs(c.y)<.35):m=graphite
   elif c.x>.9 and c.z<1.1:m=orange
   elif c.z<1.05:m=orange
   else:m=dark
  if name.startswith('Forklift_Wheel'):m=rubber if len(g[0])==96 else steel
  mats.append(m)
 mesh=bpy.data.meshes.new(name);mesh.from_pydata(points,[],faces);mesh.update();o=bpy.data.objects.new(name,mesh);scene.collection.objects.link(o);o.location=pivot
 slots=[]
 for p,m in zip(mesh.polygons,mats):
  if m not in slots:slots.append(m);mesh.materials.append(m)
  p.material_index=slots.index(m)
 return o
body=make_groups('Forklift_Body',buckets['Body'],(0,0,0),orange)
extras=[body]
for side in [-1,1]:
 extras.append(label('BodyBadge','FOUNDRYLINE',(-1.5,side*.714,1.0),(math.pi/2 if side<0 else math.pi/2,0,0 if side<0 else math.pi),.085,white))
 extras.append(box('Reflector',(-1.94,side*.52,.6),(.025,.14,.07),red))
 extras.append(box('WorkLight',(-.5,side*.48,2.47),(.09,.1,.07),white,bevel=.01))
extras.append(cyl('BeaconBase',(-1.15,0,2.55),.09,.04,graphite,(0,0,0)));extras.append(cyl('Beacon',(-1.15,0,2.63),.067,.12,yellow,(0,0,0)))
body=join('Forklift_Body',extras);register(body,folder,'body');objects['Body']=body
pivot=Vector((.28,0,.16))
for name in ['MastOuter','MastInner','Carriage']:
 o=make_groups('Forklift_'+name,buckets[name],pivot,steel if name=='MastInner' else dark)
 if name=='Carriage':o.data.transform(Matrix.Translation((0,0,-.18)))
 register(o,folder,name.lower(),mast_pivot_cm=list(pivot*100));objects[name]=o
for i,(center,gs) in enumerate(zip(wheelcenters,wheelgroups)):
 p=canonical(center);front=p.x>-.5;side='L' if p.y>0 else 'R';name='Forklift_Wheel_'+('F' if front else 'R')+side
 o=make_groups(name,gs,p,rubber);rad=max(v.co.z for v in o.data.vertices);width=max(v.co.y for v in o.data.vertices)-min(v.co.y for v in o.data.vertices)
 pieces=[o];sign=1 if p.y>0 else -1
 for j in range(24):
  a=j*math.tau/24;pieces.append(box('Tread',p+Vector((math.sin(a)*(rad-.002),0,math.cos(a)*(rad-.002))),(.065,width*.8,.018),rubber,(0,a,0)))
 for j in range(6):
  a=j*math.tau/6;pieces.append(cyl('WheelNut',p+Vector((math.sin(a)*rad*.42,sign*(width/2+.012),math.cos(a)*rad*.42)),.02,.02,steel,verts=6))
 o=join(name,pieces,p);register(o,folder,'wheel',radius_cm=round(rad*100,4),steers=not front);objects[name]=o
bpy.data.objects.remove(source,do_unlink=True)
def center_bottom(o):
 vs=[v.co for v in o.data.vertices];lo=Vector([min(v[k] for v in vs) for k in range(3)]);hi=Vector([max(v[k] for v in vs) for k in range(3)])
 o.data.transform(Matrix.Translation(Vector((-(lo.x+hi.x)/2,-(lo.y+hi.y)/2,-lo.z))))
# Preserve the vendor pallet and rack dimensions. Only game-owned derivatives change.
pallet=import_factory('SM_Pallet_01');pallet.name='Pallet_Wood';center_bottom(pallet);pallet.data.materials.clear();pallet.data.materials.append(wood)
for p in pallet.data.polygons:p.material_index=0
register(pallet,'Inventory/Storage/Pallets','pallet')
upright=import_factory('SM_Rack_01');upright.name='Rack_Upright';center_bottom(upright);upright.data.materials.clear();upright.data.materials.append(dark)
for p in upright.data.polygons:p.material_index=0
register(upright,'Inventory/Storage/SheetRacking','rack_upright')
shelf=import_factory('SM_Rack_400');shelf.name='Rack_Shelf400';center_bottom(shelf);shelf.data.materials.clear();shelf.data.materials.append(orange);shelf.data.materials.append(dark)
for p in shelf.data.polygons:p.material_index=1 if p.normal.z>.7 else 0
register(shelf,'Inventory/Storage/SheetRacking','rack_shelf')
# One physical sheet specification: 60 x 120 x 1/8 inches.
sheet=box('SteelSheet_60x120',(0,0,.003175/2),(1.524,3.048,.003175),sheetmat,bevel=.0005);register(sheet,'Inventory/Materials/Steel','sheet',width_cm=152.4,length_cm=304.8,thickness_cm=.3175)
# Longer plate roller: shafts doubled to 3520 mm; end assemblies retain their proportions.
roller=[]
for side in [-1,1]:
 roller += [box('RollerFoot',(0,side*.99,.075),(1.25,.45,.15),dark,bevel=.015),box('RollerFrame',(0,side*.99,.63),(.82,.30,1.1),orange,bevel=.02),box('RollerPanel',(-.02,side*1.15,.65),(.64,.025,.76),graphite,bevel=.008)]
 for x in [-.27,.27]:
  for z in [.35,.95]:roller.append(cyl('PanelBolt',(x,side*1.173,z),.018,.014,steel,verts=6))
roller += [box('BaseBeam',(0,0,.19),(.45,1.8,.16),dark,bevel=.01),cyl('DriveMotor',(-.34,-1.42,.58),.20,.55,graphite,(math.pi/2,0,0)),cyl('Gearbox',(0,-1.39,.97),.26,.26,dark)]
roller += [box('FootPedal',(.83,-1.1,.05),(.22,.4,.08),graphite,bevel=.008)]
roller += [label('RollerName','FOUNDRYLINE',(.42,-1.155,.86),(math.pi/2,0,math.pi/2),.075,white)]
for part in roller:
 if part.name.startswith('BaseBeam'):part.scale.y*=3.56/1.8
 else:part.location.y+=.88 if part.location.y>0 else -.88
# End bearing blocks, service covers and bolted frame stiffeners.
for side in [-1,1]:
 for x,z in [(-.22,.92),(.22,.92),(0,1.12)]:
  roller.append(cyl('BearingHousing',(x,side*1.86,z),.165,.17,dark))
  roller.append(cyl('BearingCap',(x,side*1.965,z),.105,.045,steel))
  for a in range(4):
   t=a*math.pi/2+math.pi/4
   roller.append(cyl('BearingBolt',(x+math.cos(t)*.132,side*1.963,z+math.sin(t)*.132),.018,.025,steel,verts=6))
  roller.append(cyl('GreaseFitting',(x,side*1.86,z+.17),.017,.035,yellow,(0,0,0),verts=8))
 for x in [-.34,.34]:
  roller.append(box('FrameRib',(x,side*1.87,.54),(.055,.42,.72),dark,bevel=.006))
 for x in [-.46,.46]:
  roller.append(cyl('FloorAnchor',(x,side*1.87,.16),.032,.025,steel,(0,0,0),verts=6))
 roller.append(box('PinchWarningPlate',(.423,side*1.87,.7),(.012,.21,.15),yellow,bevel=.004))
 roller.append(label('WarningMark','!',(.435,side*1.88,.67),(math.pi/2,0,math.pi/2),.12,graphite))
# Vented motor enclosure and a side service panel, kept within the original end envelope.
roller.append(box('DriveGuard',(-.02,-2.29,.92),(.69,.16,.62),graphite,bevel=.025))
for x in [-.23,-.115,0,.115,.23]:
 roller.append(box('GuardVent',(x,-2.378,.90),(.04,.012,.24),dark,bevel=.004))
roller.append(box('ElectricalCabinet',(.44,-1.85,.45),(.16,.38,.46),graphite,bevel=.015))
roller.append(box('CabinetLatch',(.533,-1.85,.49),(.025,.025,.12),steel,bevel=.004))
# Separate operator computer stand. All parts join the static body; rollers stay independent.
roller += [box('ComputerStandFoot',(1.15,-1.75,.06),(.64,.62,.12),dark,bevel=.018),
 box('ComputerStandPost',(1.15,-1.75,.62),(.13,.13,1.05),graphite,bevel=.01),
 box('KeyboardDesk',(1.15,-1.75,1.09),(.68,.57,.09),orange,bevel=.015),
 box('MonitorPost',(1.15,-1.53,1.31),(.06,.06,.37),dark,bevel=.006),
 box('MonitorFrame',(1.15,-1.53,1.52),(.55,.065,.34),graphite,bevel=.012),
 box('MonitorDisplay',(1.15,-1.568,1.52),(.49,.008,.275),glass,bevel=.004),
 box('Keyboard',(1.08,-1.80,1.153),(.39,.17,.028),dark,bevel=.006),
 box('Mouse',(1.39,-1.80,1.16),(.065,.10,.04),graphite,bevel=.018),
 box('ComputerBox',(1.15,-1.75,.83),(.32,.30,.26),graphite,bevel=.012),
 cyl('ComputerEmergencyStop',(1.40,-1.94,1.17),.035,.035,red,(0,0,0))]
for row in range(4):
 for col in range(12):roller.append(box('KeyboardKey',(.91+col*.029,-1.86+row*.034,1.171),(.023,.025,.009),steel,bevel=.002))
for index,length in enumerate([.35,.27,.39,.18]):
 roller.append(box('DisplayReadout',(1.13,-1.575,1.61-index*.057),(length,.006,.017),white))
roller.append(label('ComputerLabel','ROLL CONTROL',(.9,-2.04,1.075),(math.pi/2,0,0),.055,white))
def cable(name,points,radius=.014):
 curve=bpy.data.curves.new(name,'CURVE');curve.dimensions='3D';curve.bevel_depth=radius;curve.bevel_resolution=2
 spline=curve.splines.new('BEZIER');spline.bezier_points.add(len(points)-1)
 for p,co in zip(spline.bezier_points,points):p.co=co;p.handle_left_type='AUTO';p.handle_right_type='AUTO'
 o=bpy.data.objects.new(name,curve);scene.collection.objects.link(o);o.data.materials.append(rubber);bpy.context.view_layer.objects.active=o;o.select_set(True);bpy.ops.object.convert(target='MESH');return bpy.context.object
roller.append(cable('ComputerControlCable',[(1.15,-1.58,.83),(1.0,-1.48,.60),(.96,-1.42,.10),(.78,-1.55,.035),(.60,-1.70,.035),(.55,-1.85,.30)]))
roller.append(cable('MonitorCable',[(1.15,-1.49,1.48),(1.22,-1.48,1.20),(1.18,-1.53,.92)],.007))
base=join('SheetRoller_Body',roller);register(base,'Production/Equipment/SheetRoller','roller_body')
for name,x,z in [('LowerFront',-.22,.92),('LowerRear',.22,.92),('Upper',0,1.12)]:
 o=cyl('SheetRoller_'+name,(x,0,z),.12,3.52,steel);bpy.context.view_layer.objects.active=o;bpy.ops.object.transform_apply(location=False,rotation=True,scale=True);register(o,'Production/Equipment/SheetRoller','roller',axis='Y')
# Curved output blank, separate from the flat sheet inventory representation.
vs=[];faces=[];segments=48;radius=1.15
for i in range(segments+1):
 t=i/segments*3.048/radius
 for y in [-.762,.762]:vs.append((radius*math.sin(t),y,radius*(1-math.cos(t))))
for i in range(segments):faces.append((i*2,i*2+1,i*2+3,i*2+2))
me=bpy.data.meshes.new('RolledBlank');me.from_pydata(vs,[],faces);o=bpy.data.objects.new('SteelSheet_RolledBlank',me);scene.collection.objects.link(o);o.data.materials.append(sheetmat)
mod=o.modifiers.new('True plate thickness','SOLIDIFY');mod.thickness=.003175;bpy.context.view_layer.objects.active=o;bpy.ops.object.modifier_apply(modifier=mod.name);register(o,'Products/Components/Steel','rolled_blank')
# Export each mesh around its authored pivot. Material slots retain semantic names.
for entry in manifest:
 o=bpy.data.objects[entry['name']];copy=o.copy();copy.data=o.data.copy();scene.collection.objects.link(copy);copy.animation_data_clear();copy.parent=None;copy.matrix_world=Matrix.Identity(4)
 dest=SRC/entry['folder'];dest.mkdir(parents=True,exist_ok=True);entry['source_file']=str((dest/(entry['name']+'.fbx')).relative_to(R/'SourceAssets')).replace('\\','/');entry['asset_path']='/Game/Foundryline/'+entry['folder']+'/SM_'+entry['name']
 bpy.ops.object.select_all(action='DESELECT');copy.select_set(True);bpy.context.view_layer.objects.active=copy;bpy.ops.export_scene.fbx(filepath=str(dest/(entry['name']+'.fbx')),use_selection=True,object_types={'MESH'},bake_anim=False,add_leaf_bones=False,axis_forward='-Y',axis_up='Z');bpy.data.objects.remove(copy,do_unlink=True)
# Build a review layout from the exported meshes; game assembly uses metadata.
root=bpy.data.objects.new('Forklift_ROOT',None);scene.collection.objects.link(root)
for k,o in objects.items():o.parent=root
mastroot=bpy.data.objects.new('MastTilt_ROOT',None);scene.collection.objects.link(mastroot);mastroot.parent=root;mastroot.location=pivot
for k in ['MastOuter','MastInner','Carriage']:
 o=objects[k];o.parent=mastroot;o.location=(0,0,0)
for f,x in [(1,0),(60,2.5),(120,2.5),(180,2.5),(240,0)]:root.location.x=x;root.keyframe_insert('location',frame=f)
for e in manifest:
 if e['kind']=='wheel':
  o=bpy.data.objects[e['name']]
  for f,x in [(1,0),(60,2.5),(120,2.5),(180,2.5),(240,0)]:o.rotation_euler.y=x/(e['radius_cm']/100);o.rotation_euler.z=math.radians(-25 if e['steers'] and f in [60,180] else 0);o.keyframe_insert('rotation_euler',frame=f)
for f,lift in [(1,0),(60,0),(120,2.8),(180,2.8),(240,0)]:
 objects['Carriage'].location.z=lift;objects['Carriage'].keyframe_insert('location',frame=f);objects['MastInner'].location.z=max(0,lift-1.2);objects['MastInner'].keyframe_insert('location',frame=f)
for f,angle in [(1,0),(60,-5),(120,-5),(180,0),(240,0)]:mastroot.rotation_euler.y=math.radians(angle);mastroot.keyframe_insert('rotation_euler',frame=f)
# Park storage models beside forklift, leaving originals at authored dimensions.
pallet.location=(0,3.35,0);pallet2=pallet.copy();pallet2.data=pallet.data;scene.collection.objects.link(pallet2);pallet2.location=(0,4.65,0)
upright.location=(0,7,0);shelf.location=(0,9,0);sheet.location=(0,4,.1772)
for z in [.1899,.2058]:
 copy=sheet.copy();copy.data=sheet.data;scene.collection.objects.link(copy);copy.location=(0,4,z)
for e in manifest:
 if e['kind'].startswith('roller') or e['kind']=='rolled_blank':
  o=bpy.data.objects[e['name']];o.location.x+=6;o.location.y+=5
  if e['kind']=='roller':
   for f in [1,240]:o.rotation_euler.y=(f-1)*.12;o.keyframe_insert('rotation_euler',frame=f)
# A second upright and two shelves show the assembled storage rack in Blender.
for y in [7,11.0347]:
 copy=upright.copy();copy.data=upright.data;scene.collection.objects.link(copy);copy.location=(0,y,0)
for z in [.2,1.5,2.8]:
 copy=shelf.copy();copy.data=shelf.data;scene.collection.objects.link(copy);copy.location=(0,9.01735,z)
upright.hide_render=True;shelf.hide_render=True
bpy.ops.mesh.primitive_plane_add(size=45,location=(3,5,-.025));ground=bpy.context.object;ground.name='ReviewFloor';ground.data.materials.append(mat('ReviewFloor',(.12,.135,.15),.85))
bpy.ops.object.camera_add(location=(16,-17,15));cam=bpy.context.object;cam.rotation_euler=(Vector((2,5,1.4))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=22;scene.camera=cam
bpy.ops.object.light_add(type='AREA',location=(1,-3,14));bpy.context.object.data.energy=4000;bpy.context.object.data.size=10
scene.world=bpy.data.worlds.new('ReviewWorld');scene.world.color=(.3,.3,.3);scene.render.engine='CYCLES';scene.cycles.samples=32;scene.render.resolution_x=1440;scene.render.resolution_y=960;scene.render.resolution_percentage=100
for f,name in [(1,'MaterialHandling_Overview'),(120,'Forklift_Raised')]:scene.frame_set(f);scene.render.filepath=str(OUT/(name+'.png'));bpy.ops.render.render(write_still=True)
scene.frame_set(1)
oldloc=cam.location.copy();oldrot=cam.rotation_euler.copy();oldscale=cam.data.ortho_scale
cam.location=(10,0,4);cam.rotation_euler=(Vector((6,5,.9))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.ortho_scale=5.8
bpy.data.objects['SteelSheet_RolledBlank'].hide_render=True;scene.render.filepath=str(OUT/'SheetRoller_Detail.png');bpy.ops.render.render(write_still=True);bpy.data.objects['SteelSheet_RolledBlank'].hide_render=False
cam.location=oldloc;cam.rotation_euler=oldrot;cam.data.ortho_scale=oldscale
scene.frame_set(1);bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Foundryline_MaterialHandling.blend'));(SRC/'manifest.json').write_text(json.dumps({'assets':manifest,'sheet':{'width_cm':152.4,'length_cm':304.8,'thickness_cm':.3175,'density_kg_m3':7850,'default_count':10,'max_visible':3}},indent=2));print('MATERIAL_HANDLING_ASSETS_COMPLETE',len(manifest))
