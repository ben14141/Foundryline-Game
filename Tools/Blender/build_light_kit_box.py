"""Original labelled light-kit carton, built in isolated background Blender. Units: metres."""
import bpy,math,json
from pathlib import Path
from mathutils import Vector
root=Path(r'C:/Users/curly/Documents/Foundryline Game');dest=root/'SourceAssets/Products/Components/LightKit';dest.mkdir(parents=True,exist_ok=True)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
s=bpy.context.scene;s.unit_settings.system='METRIC';s.unit_settings.scale_length=1
def mat(name,color,metal=0,rough=.45):
 m=bpy.data.materials.new(name);m.diffuse_color=(*color,1);m.use_nodes=True;p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*color,1);p.inputs['Metallic'].default_value=metal;p.inputs['Roughness'].default_value=rough;return m
card=mat('Kit_Cardboard',(.48,.31,.16));ink=mat('Kit_Graphite',(.035,.045,.052));orange=mat('Kit_Orange',(.93,.24,.035));white=mat('Kit_Label',(.94,.94,.88));lens=mat('Kit_LampLens',(.77,.86,.9),.25,.2);steel=mat('Kit_Hardware',(.28,.31,.34),.8,.24)
def box(name,at,size,material,bevel=.002):
 bpy.ops.mesh.primitive_cube_add(size=1,location=at);o=bpy.context.object;o.name=name;o.dimensions=size;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(material)
 if bevel:b=o.modifiers.new('Soft manufacturing edges','BEVEL');b.width=bevel;b.segments=2
 return o
def text(value,at,size,rotation=(math.pi/2,0,0),material=white):
 c=bpy.data.curves.new('Printed '+value,'FONT');c.body=value;c.align_x='CENTER';c.align_y='CENTER';c.size=size;c.extrude=.0003;c.resolution_u=3;o=bpy.data.objects.new(c.name,c);s.collection.objects.link(o);o.location=at;o.rotation_euler=rotation;o.data.materials.append(material);return o
box('Double wall carton',(0,0,.13),(.48,.32,.26),card,.006)
box('Folded lid',(0,0,.263),(.484,.324,.012),card)
box('Orange seal tape',(0,0,.271),(.065,.325,.002),orange,.0004)
for side in [-1,1]:
 y=side*.1625;rot=(math.pi/2,0,math.pi if side==1 else 0)
 box('Graphite product sleeve',(0,y,.14),(.454,.003,.205),ink,.001)
 box('Orange identity band',(0,y+side*.002,.229),(.454,.002,.026),orange,.0005)
 text('LIGHT KIT',(0,y+side*.003,.176),.052,rot)
 text('LED WORK LAMP',(0,y+side*.003,.125),.024,rot)
 text('LAMP + HARNESS',(0,y+side*.003,.072),.018,rot)
 # Large recognizable lamp symbol and light rays on the lower corner.
 x=side*.165
 box('Lamp pictogram',(x,y+side*.003,.107),(.041,.002,.029),white,.002)
 box('Pictogram lens',(x,y+side*.004,.107),(.029,.002,.018),orange,.001)
 for dx in [-.024,0,.024]:box('Lamp ray',(x+dx,y+side*.003,.078),(.004,.002,.012),white,.0003)
for side in [-1,1]:
 x=side*.2425;rot=(math.pi/2,0,side*math.pi/2)
 box('End label',(x,0,.143),(.003,.27,.17),ink,.001)
 text('LIGHT KIT',(x+side*.003,0,.175),.034,rot)
 text('LK-01',(x+side*.003,0,.125),.025,rot,orange)
 text('HANDLE WITH CARE',(x+side*.003,0,.08),.012,rot)
# Sealed flat lid: the contents are identified by the carton labels and pictogram.
for y in [-.1,.1]:box('Carton seam',(0,y,.27),(.478,.002,.001),ink,.0001)
text('FOUNDRYLINE',(.105,0,.274),.023,(0,0,math.pi/2),ink)
# Export one mesh/material slots; every label is geometry, so no external fonts/textures are needed in Unreal.
for o in list(s.objects):
 bpy.context.view_layer.objects.active=o;o.select_set(True)
 if o.type=='FONT':bpy.ops.object.convert(target='MESH')
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.convert(target='MESH');bpy.ops.object.join();o=bpy.context.object;o.name='SM_LightKitBox';s.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
o.scale.x=o.scale.y=.85;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
for mod in list(o.modifiers):bpy.ops.object.modifier_apply(modifier=mod.name)
bpy.ops.export_scene.fbx(filepath=str(dest/'SM_LightKitBox.fbx'),use_selection=True,apply_unit_scale=True,apply_scale_options='FBX_SCALE_ALL',axis_forward='-Y',axis_up='Z',bake_anim=False,mesh_smooth_type='FACE',add_leaf_bones=False)
bpy.ops.wm.save_as_mainfile(filepath=str(dest/'LightKitBox.blend'))
(dest/'manifest.json').write_text(json.dumps({'bounds_cm':[41.8,28.475,27.5],'pivot':'carton bottom centre','labels':['LIGHT KIT','LED WORK LAMP','LAMP + HARNESS','LK-01'],'geometry':'sealed flat lid, original standalone materials, no vendor dependencies'},indent=2))
# Transparent menu thumbnail and larger phone-readable modelling review.
bpy.ops.object.camera_add(location=(.85,-1.1,.8));cam=bpy.context.object;cam.rotation_euler=(Vector((0,0,.15))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=.66;s.camera=cam
for at,power in [((1,-1,2),90),((-1,-.5,1),50),((0,1,1.5),70)]:
 bpy.ops.object.light_add(type='AREA',location=at);l=bpy.context.object;l.data.energy=power;l.data.size=1.5;l.rotation_euler=(Vector((0,0,.15))-l.location).to_track_quat('-Z','Y').to_euler()
s.render.engine='CYCLES';s.cycles.samples=24;s.cycles.use_denoising=True;s.render.film_transparent=True;s.render.image_settings.file_format='PNG';s.render.image_settings.color_mode='RGBA';s.render.resolution_percentage=100;s.render.resolution_x=1200;s.render.resolution_y=900;s.render.filepath=str(dest/'LightKitBox_Review.png');bpy.ops.render.render(write_still=True)
s.render.resolution_x=512;s.render.resolution_y=320;cam.data.ortho_scale=.78;s.render.filepath=str(root/'SourceAssets/UI/BuildCatalogue/T_Catalog_Part15.png');bpy.ops.render.render(write_still=True)
print('LIGHT_KIT_READY',flush=True)
