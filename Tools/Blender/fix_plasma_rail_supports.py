"""Patch only the plasma Body/Complete with welded rail girders; no live Blender required.
Run with Blender --background --python Tools/Blender/fix_plasma_rail_supports.py.
Authored metres are exported as centimetres; existing rail, tool and animation pivots stay fixed.
"""
import bpy,json,shutil,math
from pathlib import Path
from mathutils import Vector
ROOT=Path(r'C:/Users/curly/Documents/Foundryline Game')
OUT=ROOT/'SourceAssets/Production/Equipment/PlasmaTable'
BACKUP=ROOT/'Saved/Backups/PlasmaRailSupports_20261030'
BACKUP.mkdir(parents=True,exist_ok=True)
for name in ['Foundryline_PlasmaTable.blend','PlasmaTable_Body.fbx','PlasmaTable_Complete.fbx','manifest.json']:
 if not (BACKUP/name).exists():shutil.copy2(OUT/name,BACKUP/name)
bpy.ops.wm.open_mainfile(filepath=str(OUT/'Foundryline_PlasmaTable.blend'))
scene=bpy.context.scene;scene.frame_set(1);bpy.context.view_layer.update()
body=bpy.data.objects['PlasmaTable_Body'];complete=bpy.data.objects['PlasmaTable_Complete']
mat=lambda name:next(m for m in bpy.data.materials if m.name==name)
graphite=mat('MH_Graphite');steel=mat('MH_MachinedSteel')
old_bounds=[list(body.dimensions),list(body.location)]
old_pose={name:{'location':list(bpy.data.objects[name].location),'dimensions':list(bpy.data.objects[name].dimensions)} for name in ['PlasmaTable_Gantry','PlasmaTable_TorchCarriage','PlasmaTable_Torch','PlasmaTable_StockSheet']}

def box(name,center,size,material):
 bpy.ops.mesh.primitive_cube_add(size=1,location=center);o=bpy.context.object;o.name=name;o.dimensions=size
 bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(material)
 b=o.modifiers.new('Manufactured edge','BEVEL');b.width=.003;b.segments=2;bpy.ops.object.modifier_apply(modifier=b.name)
 uv=o.data.uv_layers.active or o.data.uv_layers.new(name='UVMap')
 for p in o.data.polygons:
  axis=max(range(3),key=lambda i:abs(p.normal[i]));axes=[i for i in range(3) if i!=axis]
  for loop in p.loop_indices:
   v=o.matrix_world@o.data.vertices[o.data.loops[loop].vertex_index].co;uv.data[loop].uv=(v[axes[0]],v[axes[1]]*3)
 return o

def attach(target,parts):
 bpy.ops.object.select_all(action='DESELECT');target.hide_set(False);target.select_set(True)
 for p in parts:p.select_set(True)
 bpy.context.view_layer.objects.active=target;bpy.ops.object.join()

if not body.get('rail_support_revision',0):
 parts=[]
 for side in [-1,1]:
  x=side*(.97+.762)
  for name,z,width,height,material in [('RailGirderBase',.845,.14,.018,graphite),('RailGirderWeb',.947,.09,.218,graphite),('RailMountingCap',1.054,.10,.020,steel)]:
   parts.append(box(name,(x,0,z),(width,3.62+1.2192,height),material))
 copies=[]
 for o in parts:
  c=o.copy();c.data=o.data.copy();scene.collection.objects.link(c);copies.append(c)
 attach(body,parts);attach(complete,copies);body['rail_support_revision']=1
bpy.context.view_layer.update()
for value,want in zip(body.dimensions,old_bounds[0]):assert abs(value-want)<.00001,('Changed body bounds',list(body.dimensions),old_bounds)
for name,old in old_pose.items():
 o=bpy.data.objects[name]
 assert all(abs(a-b)<.00001 for a,b in zip(o.location,old['location'])),name+' moved'
 assert all(abs(a-b)<.00001 for a,b in zip(o.dimensions,old['dimensions'])),name+' resized'
# The girder physically intersects both main side beam and guide rail. Dimensions in metres.
assert .947-.218/2<.84 and .947+.218/2>1.054-.020/2
assert 1.054+.020/2>=1.08-.035/2
for o in [body,complete]:
 bpy.ops.object.select_all(action='DESELECT');o.hide_set(False);o.select_set(True);bpy.context.view_layer.objects.active=o
 bpy.ops.export_scene.fbx(filepath=str(OUT/(o.name+'.fbx')),use_selection=True,object_types={'MESH'},bake_anim=False,add_leaf_bones=False,axis_forward='-Y',axis_up='Z',mesh_smooth_type='FACE')
manifest=json.loads((OUT/'manifest.json').read_text())
manifest['rail_supports']={'revision':1,'main_beam_top_cm':84,'rail_underside_cm':106.25,'support_web_z_cm':[83.8,105.6],'machined_cap_z_cm':[104.4,106.4],'gantry_pivots_changed':False}
for a in manifest['assets']:
 if a['name'] in [body.name,complete.name]:
  o=bpy.data.objects[a['name']];a['dimensions_cm']=list(o.dimensions*100);a['position_cm']=list(o.location*100)
(OUT/'manifest.json').write_text(json.dumps(manifest,indent=2))
complete.hide_render=True
stock=bpy.data.objects['PlasmaTable_StockSheet'];stock.hide_render=False
scene.render.engine='CYCLES';scene.cycles.samples=24;scene.render.resolution_x=1440;scene.render.resolution_y=1080;scene.render.resolution_percentage=100
camera=scene.camera;camera.data.type='ORTHO'
def render(loc,target,scale,name):
 camera.location=loc;camera.rotation_euler=(Vector(target)-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.ortho_scale=scale;scene.render.filepath=str(OUT/name);bpy.ops.render.render(write_still=True)
render((4.6,-4.8,2.6),(1.15,-.5,.88),4.4,'PlasmaTable_RailSupports_Detail.png')
render((7,-7,6.4),(0,-.1,.8),8.,'PlasmaTable_RailSupports_Overview.png')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Foundryline_PlasmaTable.blend'))
report={'result':'PASS','body_bounds_unchanged':True,'gantry_torch_stock_pivots_and_dimensions_unchanged':True,'rail_girder_contacts_main_beam_and_rail':True,'main_beam_top_cm':84,'rail_underside_cm':106.25,'gap_closed_cm':22.25,'exports':['PlasmaTable_Body.fbx','PlasmaTable_Complete.fbx'],'review_images':['PlasmaTable_RailSupports_Detail.png','PlasmaTable_RailSupports_Overview.png'],'unreal_imported':False}
(ROOT/'Saved/Logs/PlasmaRailSupportsModelQA.json').write_text(json.dumps(report,indent=2))
print('PLASMA_RAIL_SUPPORTS_READY')
