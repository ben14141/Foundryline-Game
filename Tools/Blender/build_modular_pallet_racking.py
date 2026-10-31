"""Original Foundryline rack geometry, metres in Blender / centimetres in UE.
No imported geometry, images, vendor materials, or gameplay storage mutations.
Run Blender --background --threads 2 --python this_file.
"""
import bpy, math, json
from pathlib import Path
from mathutils import Vector

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'SourceAssets/Inventory/Storage/ModularPalletRacking'
OUT.mkdir(parents=True,exist_ok=True)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
scene=bpy.context.scene;scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1
scene.render.threads_mode='FIXED';scene.render.threads=2
def material(name,color,metal=0,rough=.45):
 m=bpy.data.materials.new(name);m.use_nodes=True;p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*color,1);p.inputs['Metallic'].default_value=metal;p.inputs['Roughness'].default_value=rough;return m
orange=material('FL_Rack_Orange',(.8,.205,.035),.25,.32)
graphite=material('FL_Rack_Graphite',(.045,.06,.072),.45,.42)
steel=material('FL_Rack_Steel',(.43,.48,.53),.8,.3)
yellow=material('FL_Rack_SafetyYellow',(.9,.58,.035),.2,.4)
white=material('FL_Rack_LabelWhite',(.85,.87,.83),.05,.48)
wood=material('FL_Review_Wood',(.42,.25,.11),0,.75)
pieces=[]
def apply(o):
 bpy.context.view_layer.objects.active=o;o.select_set(True);bpy.ops.object.transform_apply(location=True,rotation=True,scale=True);o.select_set(False)
def box(name,at,size,mat,bevel=.003):
 bpy.ops.mesh.primitive_cube_add(size=1,location=at);o=bpy.context.object;o.name=name;o.dimensions=size;o.data.materials.append(mat);apply(o)
 if bevel:
  mod=o.modifiers.new('Small manufactured edge','BEVEL');mod.width=bevel;mod.segments=1;bpy.context.view_layer.objects.active=o;bpy.ops.object.modifier_apply(modifier=mod.name)
 pieces.append(o);return o
def rod(name,a,b,r,mat,verts=8):
 a,b=Vector(a),Vector(b);bpy.ops.mesh.primitive_cylinder_add(vertices=verts,radius=r,depth=(b-a).length,location=(a+b)/2);o=bpy.context.object;o.name=name;o.rotation_euler=(b-a).to_track_quat('Z','Y').to_euler();o.data.materials.append(mat);apply(o);pieces.append(o);return o
def text(name,body,at,size=.035,mat=white):
 bpy.ops.object.text_add(location=at,rotation=(math.pi/2,0,0));o=bpy.context.object;o.name=name;o.data.body=body;o.data.size=size;o.data.extrude=.0003;o.data.materials.append(mat);bpy.ops.object.convert(target='MESH');o=bpy.context.object;apply(o);pieces.append(o);return o
def combine(name,objects):
 bpy.ops.object.select_all(action='DESELECT')
 for o in objects:o.select_set(True)
 bpy.context.view_layer.objects.active=objects[0];bpy.ops.object.join();o=bpy.context.object;o.name=name;scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR');apply(o);return o
def export(o,name):
 bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o
 bpy.ops.export_scene.fbx(filepath=str(OUT/(name+'.fbx')),use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',apply_unit_scale=True,bake_anim=False,add_leaf_bones=False,mesh_smooth_type='FACE')
def frame(height):
 global pieces
 pieces=[]
 # One low continuous side rail, rather than four tall independent pedestals.
 box('Continuous anchor rail',(0,0,.012),(.15,1.48,.024),graphite)
 for y in [-.66,.66]:
  for x in [-.033,.033]:box('C channel flange',(x,y,height/2),(.014,.08,height),graphite)
  box('C channel web',(0,y+.033,height/2),(.08,.014,height),graphite)
  # Bolted foot plates sit directly under each upright, inside the original low-rail envelope.
  box('Upright foot plate',(0,y,.018),(.15,.16,.036),graphite,.002)
  for bx in [-.054,.054]:
   for by in [-.045,.045]:
    rod('Anchor washer',(bx,y+by,.036),(bx,y+by,.040),.014,steel,12)
    rod('Anchor hex nut',(bx,y+by,.040),(bx,y+by,.051),.010,steel,6)
    rod('Anchor stud tip',(bx,y+by,.051),(bx,y+by,.057),.0055,graphite,8)

  for z in [.25,.55,.85,1.15,1.45,1.75,2.05,2.35,2.65,2.95,3.25,3.55,3.85,4.15,4.45]:
   if z<height-.1:box('Stamped beam slot',(.041,y-.008,z),(.002,.022,.045),steel,0)
 for z in [i*.7+.08 for i in range(math.ceil(height/.7))]:
  end=min(z+.7,height-.06)
  if end>z:
   rod('Diagonal side brace',(0,-.62,z),(0,.62,end),.014,graphite);rod('Side horizontal',(0,-.62,z),(0,.62,z),.014,graphite)
   for by,bz in [(-.62,z),(.62,end)]:
    rod('Brace fastener washer',(.016,by,bz),(.020,by,bz),.014,steel,12)
    rod('Brace fastener hex head',(.020,by,bz),(.030,by,bz),.008,steel,6)
 box('Side post cap front',(0,-.66,height-.008),(.09,.09,.016),graphite)
 box('Side post cap back',(0,.66,height-.008),(.09,.09,.016),graphite)
 # Low corner guard only outside the fork entry.
 box('Front impact guard',(0,-.704,.19),(.12,.035,.32),yellow)
 return combine('UprightFrame',pieces)
def shelf():
 global pieces
 pieces=[]
 for y in [-.60,.60]:
  box('Orange load beam',(0,y,-.06),(2.82,.12,.12),orange)
  for x in [-1.40,1.40]:
   box('Beam hook plate',(x,y,-.065),(.04,.145,.16),orange)
   face=-1 if y<0 else 1
   for z in [-.078,-.122]:rod('Beam connector hex head',(x,y+face*.0725,z),(x,y+face*.0845,z),.008,steel,6)
   rod('Beam locking pin',(x,y+face*.064,-.027),(x,y+face*.104,-.027),.005,steel,10)
   bpy.ops.mesh.primitive_torus_add(major_segments=12,minor_segments=6,location=(x,y+face*.103,-.046),rotation=(math.pi/2,0,0),major_radius=.015,minor_radius=.0025)
   clip=bpy.context.object;clip.name='Beam locking pin retaining ring';clip.data.materials.append(steel);apply(clip);pieces.append(clip)
 for x in [-1.10,-.70,-.35,0,.35,.70,1.10]:box('Depth support',(x,0,-.035),(.035,1.12,.055),graphite)
 for x in [i*.14-1.33 for i in range(20)]:box('Steel mesh deck longitudinal',(x,0,-.004),(.012,1.08,.008),steel,0)
 for y in [-.50,-.25,0,.25,.50]:box('Steel mesh deck crosswire',(0,y,-.009),(2.74,.012,.01),steel,0)
 return combine('ShelfDeck',pieces)

shared=shelf();export(shared,'PalletRack_ShelfDeck');shared.hide_render=True;shared.hide_set(True)
manifest={'original_authorship':True,'units':'Blender metres / Unreal centimetres','materials':{m.name:{'base_color':list(m.node_tree.nodes.get('Principled BSDF').inputs['Base Color'].default_value)[:3],'metallic':m.node_tree.nodes.get('Principled BSDF').inputs['Metallic'].default_value,'roughness':m.node_tree.nodes.get('Principled BSDF').inputs['Roughness'].default_value} for m in [orange,graphite,steel,yellow,white]},'assets':[],'variants':[]}
qa={'checks':{},'dimensions_cm':{},'pallet_cm':[117.974,115.706,17.557],'payload_max_height_cm':100,'fork_lift_clearance_cm':8,'fork_blade_centres_cm':101.702,'fork_blade_width_cm':11.677,'fork_max_lift_cm':330}
racks=[]
for levels in [1,2,3]:
 h=1.524*levels;f=frame(h);export(f,'PalletRack_UprightFrame_'+str(levels)+'Level');f.hide_render=True;f.hide_set(True)
 parts=[]
 for x in [-1.45,1.45]:
  c=f.copy();c.data=f.data.copy();scene.collection.objects.link(c);c.hide_render=False;c.hide_set(False);c.location.x=x;apply(c);parts.append(c)
 for level in range(1,levels):
  c=shared.copy();c.data=shared.data.copy();scene.collection.objects.link(c);c.hide_render=False;c.hide_set(False);c.location.z=1.524*level;apply(c);parts.append(c)
 pieces=[]
 rod('Rear top crossrail',(-1.41,.66,h-.06),(1.41,.66,h-.06),.025,graphite)
 for z in [i*1.524 for i in range(levels)]:
  rod('Rear diagonal',(-1.41,.69,z+.1),(1.41,.69,z+1.42),.012,graphite)
  rod('Rear diagonal',(1.41,.69,z+.1),(-1.41,.69,z+1.42),.012,graphite)
 box('Level identification plate',(-1.45,-.731,.75),(.12,.014,.19),orange)
 text('Level count',str(levels),(-1.495,-.74,.70),.095)
 parts+=pieces
 name='PalletRack_'+('1Level' if levels==1 else str(levels)+'Levels');rack=combine(name,parts);export(rack,name);racks.append(rack)
 bpy.context.view_layer.update();dims=[round(v*100,4) for v in rack.dimensions];qa['dimensions_cm'][name]=dims
 heights=[round(i*152.4,3) for i in range(levels)];slots=[{'id':i*2+j+1,'position_cm':[x,0,z]} for i,z in enumerate(heights) for j,x in enumerate([-70,70])]
 manifest['variants'].append({'name':name,'levels':levels,'usable_pallet_slots':2*levels,'dimensions_cm':dims,'shelf_top_cm':heights,'slot_bottoms':slots,'minimum_clear_ceiling_cm':round(h*100+30,3),'maximum_pallet_payload_height_cm':100,'fork_access':'Open front; floor is level1; no front floor crossbeam','floor_level_included':True})
 manifest['assets'].append({'name':name,'file':name+'.fbx','destination':'/Game/Foundryline/Inventory/Storage/ModularPalletRacking/SM_'+name})
 qa['checks'][name+'_expected_height']=abs(dims[2]-h*100)<.2
 qa['checks'][name+'_two_positions_per_level']=len(slots)==2*levels
 qa['checks'][name+'_pallets_clear_uprights']=70+117.974/2<141
 qa['checks'][name+'_pallets_do_not_overlap']=140-117.974>0
 qa['checks'][name+'_rear_brace_clear']=115.706/2<66-2.5
 qa['checks'][name+'_fork_envelope_clear']=101.702+11.677<140
 qa['checks'][name+'_fork_top_deck_accessible']=heights[-1]+8<=330
 qa['checks'][name+'_loaded_pallet_has_lift_clearance']=152.4-12-(17.557+100+8)>0
 # Save isolated editable variant; review population is added only afterward.
 for other in racks:other.hide_render=other!=rack;other.hide_set(other!=rack)
 bpy.ops.wm.save_as_mainfile(filepath=str(OUT/(name+'.blend')))
 rack.location.x=(levels-2)*3.6
 f.hide_render=True

for rack in racks:rack.hide_render=False;rack.hide_set(False)
# Fully original simplified review pallets and plate stacks, not vendor geometry.
pieces=[]
for variant,rack in zip(manifest['variants'],racks):
 for z in variant['shelf_top_cm']:
  for x in [-.70,.70]:
   atx=rack.location.x+x;base=z/100
   for px in [-.43,0,.43]:box('Review pallet support',(atx+px,0,base+.077785),(.14,1.15706,.15557),wood)
   for py in [-.49,-.25,0,.25,.49]:box('Review pallet top',(atx,py,base+.16557),(1.17974,.18,.02),wood)
   for i in range(6):box('Review cut plate stack',(atx,0,base+.17557+.012+i*.027),(1.02,.92,.024),steel,.001)
qa['checks']['all_variants_same_width_depth']=len({tuple(v['dimensions_cm'][:2]) for v in manifest['variants']})==1
qa['minimum_vertical_lift_gap_cm']=round(152.4-12-17.557-100-8,3)
qa['fork_blade_envelope_cm']=round(101.702+11.677,3)
assert all(qa['checks'].values()),qa
(OUT/'manifest.json').write_text(json.dumps(manifest,indent=2));(OUT/'geometry_qa.json').write_text(json.dumps(qa,indent=2))
box('Review floor',(0,0,-.035),(13,7,.05),material('FL_Review_Floor',(.09,.11,.13),.1,.6),0)
for x,power,size in [(-5,1700,6),(4,2100,5),(0,1300,5)]:
 bpy.ops.object.light_add(type='AREA',location=(x,-4,7));o=bpy.context.object;o.data.energy=power;o.data.shape='DISK';o.data.size=size;o.rotation_euler=(Vector((0,0,2))-o.location).to_track_quat('-Z','Y').to_euler()
scene.world=bpy.data.worlds.new('OriginalRackReview');scene.world.color=(.25,.25,.25)
bpy.ops.object.camera_add(location=(9,-16,9));cam=bpy.context.object;cam.rotation_euler=(Vector((0,0,2.1))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=13.4;scene.camera=cam
scene.render.engine='CYCLES';scene.cycles.device='CPU';scene.cycles.samples=12;scene.render.resolution_x=1400;scene.render.resolution_y=900;scene.render.resolution_percentage=100
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'PalletRack_AllLevels_Review.blend'))
scene.render.filepath=str(OUT/'PalletRack_AllLevels.png');bpy.ops.render.render(write_still=True)
print('MODULAR_PALLET_RACKING_COMPLETE',len(manifest['assets']),len(qa['checks']))
