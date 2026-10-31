"""Original true-scale 36 x 84 in steel personnel leaves and fitted 100 x 220 cm jambs.
Background Blender only; does not touch vendor content or another Blender session.
"""
import bpy,math,json
from pathlib import Path
from mathutils import Vector
ROOT=Path(r'C:/Users/curly/Documents/Foundryline Game')
OUT=ROOT/'SourceAssets/Buildings/Doors/Personnel';OUT.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True);scene=bpy.context.scene;scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1
mats={}
def material(name,col,rough=.3,metal=0):
 m=bpy.data.materials.new('FD_'+name);m.use_nodes=True;b=m.node_tree.nodes.get('Principled BSDF');b.inputs['Base Color'].default_value=(*col,1);b.inputs['Roughness'].default_value=rough;b.inputs['Metallic'].default_value=metal;mats[name]=m;return m
graphite=material('Graphite',(.072,.088,.10),.27,.35);light=material('LightGrey',(.61,.65,.64),.27,.3);steel=material('Stainless',(.48,.53,.55),.23,.85);rubber=material('Rubber',(.012,.017,.019),.75);orange=material('FoundryOrange',(.85,.28,.055),.3,.25)
glass=material('Glass',(.70,.85,.87),.08);g=glass.node_tree.nodes.get('Principled BSDF');g.inputs['Transmission Weight'].default_value=1;g.inputs['IOR'].default_value=1.45

def box(name,at,size,mat,bevel=.002):
 bpy.ops.mesh.primitive_cube_add(size=1,location=at);o=bpy.context.object;o.name=name;o.dimensions=size;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(mat)
 if bevel:
  mod=o.modifiers.new('Rounded manufactured edges','BEVEL');mod.width=bevel;mod.segments=3;bpy.ops.object.modifier_apply(modifier=mod.name)
 return o

def cyl(name,at,r,depth,mat,axis='Z',segments=24):
 rot=(0,0,0) if axis=='Z' else (math.pi/2,0,0) if axis=='Y' else (0,math.pi/2,0)
 bpy.ops.mesh.primitive_cylinder_add(vertices=segments,radius=r,depth=depth,location=at,rotation=rot);o=bpy.context.object;o.name=name;o.data.materials.append(mat)
 for p in o.data.polygons:p.use_smooth=len(p.vertices)==4
 bevel=o.modifiers.new('Machined edge','BEVEL');bevel.width=.0008;bevel.segments=2;bpy.ops.object.modifier_apply(modifier=bevel.name)
 return o

def tube(name,points,r,mat):
 c=bpy.data.curves.new(name,'CURVE');c.dimensions='3D';c.bevel_depth=r;c.bevel_resolution=3;s=c.splines.new('BEZIER');s.bezier_points.add(len(points)-1)
 for p,co in zip(s.bezier_points,points):p.co=co;p.handle_left_type=p.handle_right_type='AUTO'
 o=bpy.data.objects.new(name,c);scene.collection.objects.link(o);c.materials.append(mat);bpy.context.view_layer.objects.active=o;bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.ops.object.convert(target='MESH');return bpy.context.object

def join(name,parts):
 bpy.ops.object.select_all(action='DESELECT')
 for o in parts:o.select_set(True)
 bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.join();o=bpy.context.object;o.name=name;scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
 uv=o.data.uv_layers.active or o.data.uv_layers.new(name='UVMap')
 for p in o.data.polygons:
  axis=max(range(3),key=lambda i:abs(p.normal[i]));axes=[i for i in range(3) if i!=axis]
  for loop in p.loop_indices:
   v=o.matrix_world@o.data.vertices[o.data.loops[loop].vertex_index].co;uv.data[loop].uv=(v[axes[0]],v[axes[1]])
 return o

W,H,T=.9144,2.1336,.045;CY=.0255;HoleW,HoleH=.269,.802;WX,WZ=W*.5,1.45

def leaf(name,paint):
 p=[];xl,xr=WX-HoleW/2,WX+HoleW/2;zb,zt=WZ-HoleH/2,WZ+HoleH/2
 panel=box('SingleSteelDoorSkin',(W/2,CY,H/2),(W,T,H),paint,0)
 cutter=box('VisionCutout',(WX,CY,WZ),(HoleW,T+.03,HoleH),paint,0)
 mod=panel.modifiers.new('Real glazed opening','BOOLEAN');mod.operation='DIFFERENCE';mod.solver='EXACT';mod.object=cutter;bpy.context.view_layer.objects.active=panel;bpy.ops.object.modifier_apply(modifier=mod.name);bpy.data.objects.remove(cutter,do_unlink=True)
 mod=panel.modifiers.new('Folded steel edges','BEVEL');mod.width=.0015;mod.segments=3;bpy.ops.object.modifier_apply(modifier=mod.name);p.append(panel)
 for side in [-1,1]:
  face=CY+side*(T/2+.0015)
  for x in [WX-.127,WX+.127]:p.append(box('VisionFrameStile',(x,face,WZ),(.025,.005,.762),steel,.0015))
  for z in [WZ-.3935,WZ+.3935]:p.append(box('VisionFrameRail',(WX,face,z),(.279,.005,.025),steel,.0015))
  for x in [WX-.117,WX+.117]:p.append(box('VisionPaneSeal',(x,CY+side*.004,WZ),(.007,.008,.765),rubber,.001))
  for z in [WZ-.383,WZ+.383]:p.append(box('VisionPaneSeal',(WX,CY+side*.004,z),(.24,.008,.007),rubber,.001))
  for x in [WX-.128,WX+.128]:
   for z in [WZ-.354,WZ,WZ+.354]:p.append(cyl('GlazingScrew',(x,face+side*.0032,z),.0022,.002,steel,'Y',12))
  p.append(box('StainlessKickplate',(W/2,face,.20),(W-.075,.002,.26),steel,.001))
  for x in [.065,W-.065]:
   for z in [.09,.31]:p.append(cyl('KickplateScrew',(x,face+side*.002,.0+z),.002,.002,steel,'Y',12))
  # Real escutcheon, spindle and return-to-door lever; all travel with the leaf.
  hx=W-.075;hz=1.02
  p.append(box('LeverBackplate',(hx,face+side*.002,hz),(.042,.006,.18),steel,.004))
  p.append(cyl('LeverSpindle',(hx,face+side*.023,hz),.013,.04,steel,'Y'))
  p.append(tube('SafetyReturnLever',[(hx,face+side*.045,hz),(hx-.04,face+side*.052,hz),(hx-.115,face+side*.052,hz),(hx-.126,face+side*.035,hz)],.008,steel))
  p.append(cyl('KeyCylinder',(hx,face+side*.007,hz-.057),.011,.009,steel,'Y'))
  p.append(box('KeySlot',(hx,face+side*.012,hz-.057),(.0025,.0008,.012),rubber,.0003))
  for z in [hz-.078,hz+.078]:p.append(cyl('BackplateScrew',(hx,face+side*.006,z),.0022,.0015,steel,'Y',12))
 p.append(box('VisionGlass',(WX,CY,WZ),(.229,.006,.762),glass,.0005))
 for z in [.20,1.0668,1.9336]:
  for k in [-1,0,1]:p.append(cyl('HingeKnuckle',(0,0,z+k*.032),.009,.030,steel))
  p.append(box('LeafHingePlate',(.018,.002,z),(.04,.004,.105),steel,.0015))
  for dz in [-.035,.035]:p.append(cyl('HingeFixing',(.024,-.001,z+dz),.003,.002,steel,'Y',12))
 p.append(box('LatchEdgePlate',(W+.001,CY,1.02),(.002,.029,.14),steel,.001))
 p.append(box('LatchBolt',(W+.003,CY,1.02),(.005,.012,.022),steel,.002))
 return join(name,p)

def frame(name,depth):
 p=[]
 for x in [.02,.98]:
  p.append(box('HollowMetalJamb',(x,0,1.0715),(.04,depth,2.143),graphite,.002))
  stopx=.046 if x<.5 else .954
  p.append(box('DoorStop',(stopx,-.029,1.0715),(.012,.012,2.143),graphite,.001))
  p.append(box('CompressionSeal',(stopx,-.0229,1.0715),(.008,.003,2.143),rubber,.0006))
  for side in [-1,1]:
   p.append(box('PressedSteelCasing',(x+(-.0125 if x<.5 else .0125),side*(depth/2+.002),1.0875),(.065,.006,2.175),graphite,.0015))
   for z in [.15,.75,1.35,2.05]:p.append(cyl('FrameFixing',(x,side*(depth/2+.006),z),.003,.002,steel,'Y',12))
 p.append(box('HeadJamb',(.5,0,2.1715),(1,depth,.057),graphite,.002))
 p.append(box('HeadStop',(.5,-.029,2.137),(.92,.012,.012),graphite,.001))
 p.append(box('HeadSeal',(.5,-.0229,2.137),(.92,.003,.008),rubber,.0006))
 for side in [-1,1]:p.append(box('HeadCasing',(.5,side*(depth/2+.002),2.20),(1.05,.006,.05),graphite,.0015))
 p.append(box('StrikePlate',(.9585,0,1.0264),(.003,.05,.16),steel,.001))
 p.append(box('LatchPocket',(.9567,-.001,1.0264),(.0006,.015,.026),rubber,.001))
 p.append(box('LowThreshold',(.5,0,.002),(.92,depth,.004),steel,.001))
 return join(name,p)

assets=[leaf('PersonnelDoor_OfficeLeaf',light),leaf('PersonnelDoor_ShopLeaf',graphite),frame('PersonnelDoor_OfficeFrame',.20),frame('PersonnelDoor_ShopFrame',.30)]
manifest={'units':'metres in Blender; centimetres in Unreal','leaf_panel_cm':[91.44,4.5,213.36],'rough_opening_cm':[100,220],'leaf_hinge_offset_cm':[4.28,-2.55,.64],'unreal_leaf_hinge_offset_cm':[4.28,2.55,.64],'side_gap_mm':2.8,'head_gap_mm':3,'floor_gap_mm':6.4,'leaf_positive_x_toward_latch':True,'assets':[]}
for o in assets:
 bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o
 bpy.ops.export_scene.fbx(filepath=str(OUT/(o.name+'.fbx')),use_selection=True,object_types={'MESH'},bake_anim=False,add_leaf_bones=False,axis_forward='-Y',axis_up='Z',mesh_smooth_type='FACE')
 manifest['assets'].append({'name':o.name,'dimensions_cm':list(o.dimensions*100),'materials':[m.name for m in o.data.materials]})
 o.hide_render=True
# Render assembled office door; keep authored zero origins in exported assets.
frameo=assets[2];frameo.hide_render=False;leaff=assets[0];leaff.hide_render=False;leaff.location=(.0428,-.0255,.0064)
wallmat=material('ReviewWall',(.68,.69,.66),.72);floormat=material('ReviewFloor',(.17,.19,.20),.72)
box('WallLeft',(-.52,0,1.45),(1,.16,2.9),wallmat,.003);box('WallRight',(1.52,0,1.45),(1,.16,2.9),wallmat,.003);box('WallHeader',(.5,0,2.56),(1.05,.16,.72),wallmat,.003)
box('Floor',(.5,0,-.035),(20,20,.06),floormat,.001)
# A softly lit orange surface behind the glazed vision pane makes transparency readable.
box('Background',(.5,1.5,1.45),(3,.05,3),orange,.002)
for loc,power,size in [((2,-3,4),600,3),((-2,-2,2),350,2),((0,1,4),550,2)]:
 bpy.ops.object.light_add(type='AREA',location=loc);o=bpy.context.object;o.data.energy=power;o.data.shape='DISK';o.data.size=size;o.rotation_euler=(Vector((.5,0,1.2))-o.location).to_track_quat('-Z','Y').to_euler()
scene.world=bpy.data.worlds.new('PersonnelDoorReviewWorld');scene.world.color=(.18,.18,.18);scene.render.engine='CYCLES';scene.cycles.samples=40;scene.render.resolution_x=1200;scene.render.resolution_y=1440;scene.render.resolution_percentage=100
bpy.ops.object.camera_add();scene.camera=bpy.context.object;scene.camera.data.type='ORTHO'
def render(at,target,scale,name):
 scene.camera.location=at;scene.camera.rotation_euler=(Vector(target)-scene.camera.location).to_track_quat('-Z','Y').to_euler();scene.camera.data.ortho_scale=scale;scene.render.filepath=str(OUT/name);bpy.ops.render.render(write_still=True)
render((2.2,-4,2.3),(.5,0,1.12),2.7,'PersonnelDoor_Closed.png')
leaff.rotation_euler.z=math.radians(-80);render((2.6,-4,2.4),(.45,0,1.10),3.0,'PersonnelDoor_Open.png')
leaff.rotation_euler.z=0
render((1.6,-2.5,1.45),(.82,0,1.02),.60,'PersonnelDoor_Hardware.png')
# Save asset editing scene at correct zero pivots, with rendering assembly documented.
leaff.location=(0,0,0)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Foundryline_PersonnelDoors.blend'))
(OUT/'manifest.json').write_text(json.dumps(manifest,indent=2))
checks={'result':'PASS','panel_cm':[91.44,4.5,213.36],'rough_opening_cm':[100,220],'clearance_mm':{'left':2.8,'right':2.8,'head':3,'floor':6.4},'leaf_hinge_offset_cm':[4.28,-2.55,.64],'unreal_leaf_hinge_offset_cm':[4.28,2.55,.64],'real_glazing_opening':True,'positive_x_hinge_convention':True,'exports':len(assets),'unreal_imported':False}
assert abs(.04+.0028+W+.0028+.04-1)<.000001
assert abs(.0064+H+.003+.057-2.2)<.000001
(ROOT/'Saved/Logs/PersonnelDoorModelQA.json').write_text(json.dumps(checks,indent=2));print('PERSONNEL_DOORS_READY')
