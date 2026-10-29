import bpy,math,json
from pathlib import Path
from mathutils import Vector,Matrix
ROOT=Path(r'C:\Users\curly\Documents\Foundryline Game');OUT=ROOT/'SourceAssets/AssemblyReviews/TankerTractor'
bpy.ops.wm.open_mainfile(filepath=str(OUT/'Foundryline_Tanker_Tractor.blend'));bpy.context.scene.frame_set(1);bpy.context.view_layer.update()
if any(m and m.name.startswith('DetailRubber') for m in bpy.data.objects['Tanker_Wheel_0'].data.materials):
 print('Details already present; keeping existing geometry.');raise SystemExit(0)
# The vendor FBXs used different UV-layer names on different tractor parts.
# Joining padded the unused layers with zeroes. Consolidate the authored UVs.
for o in bpy.data.objects:
 if o.type!='MESH' or not o.name.startswith('Tractor_') or len(o.data.uv_layers)<2:continue
 layers=list(o.data.uv_layers);base=layers[0]
 for i in range(len(base.data)):
  valid=next((u.data[i].uv.copy() for u in layers if u.data[i].uv.length>1e-7),Vector((0,0)))
  base.data[i].uv=valid
 for u in layers[1:]:o.data.uv_layers.remove(u)
 base.name='UVMap';base.active_render=True

def material(name,color,rough=.5,metal=0):
 m=bpy.data.materials.get(name) or bpy.data.materials.new(name);m.use_nodes=True;n=next(n for n in m.node_tree.nodes if n.type=='BSDF_PRINCIPLED');n.inputs['Base Color'].default_value=(*color,1);n.inputs['Roughness'].default_value=rough;n.inputs['Metallic'].default_value=metal;return m
rubber=material('DetailRubber',(.025,.03,.035),.9);metal=material('DetailMetal',(.34,.39,.44),.3,.7);red=material('DetailRed',(.65,.02,.012),.35);white=material('DetailWhite',(.85,.88,.84),.4)
new=[]
def box(parent,loc,size,mat,rot=(0,0,0)):
 bpy.ops.mesh.primitive_cube_add(size=1);o=bpy.context.object;o.parent=parent;o.location=loc;o.rotation_euler=rot;o.scale=size;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(mat);new.append(o);return o
def cylinder(parent,loc,radius,depth,mat,rot=(math.pi/2,0,0),vertices=12):
 bpy.ops.mesh.primitive_cylinder_add(vertices=vertices,radius=radius,depth=depth);o=bpy.context.object;o.parent=parent;o.location=loc;o.rotation_euler=rot;o.data.materials.append(mat);new.append(o);return o
def merge(parent):
 children=[o for o in new if o.parent==parent];new.clear()
 if not children:return
 bpy.context.view_layer.update();bpy.ops.object.select_all(action='DESELECT');parent.select_set(True)
 for o in children:o.select_set(True)
 bpy.context.view_layer.objects.active=parent;bpy.ops.object.join()
for o in list(bpy.data.objects):
 if not o.name.startswith('Tanker_Wheel_'):continue
 sign=1 if o.location.y>0 else -1
 for band in [-.18,.18]:
  for j in range(28):
   a=j*math.tau/28
   box(o,(math.sin(a)*.477,band,math.cos(a)*.477),(.065,.145,.018),rubber,(0,a,0))
 cylinder(o,(0,sign*.327,0),.105,.045,metal)
 for j in range(8):
  a=j*math.tau/8;cylinder(o,(math.sin(a)*.17,sign*.327,math.cos(a)*.17),.022,.022,metal,vertices=6)
 merge(o)
body=bpy.data.objects['Tanker_Body']
for side in [-1,1]:
 for j in range(16):box(body,(-6.15+j*.2,side*1.12,.92),(.19,.014,.065),red if j%2 else white)
 cylinder(body,(-5.8,side*1.08,1.5),.03,6.0,metal,(0,math.pi/2,0))
 cylinder(body,(-7.7,side*1.14,1.27),.10,.025,red,vertices=12)
 for k in [-1,1]:box(body,(-10.66,side*.87+k*.11,.67),(.055,.14,.09),red)
 box(body,(-10.69,side*.95,.92),(.018,.16,.05),white)
box(body,(-10.7,0,.72),(.025,.44,.16),white)
merge(body)
for i in range(2):
 o=bpy.data.objects['Tanker_Leg_'+str(i)];bottom=min(v.co.z for v in o.data.vertices)
 for a in [-.1,.1]:cylinder(o,(a,0,bottom+.036),.017,.012,metal,(0,0,0),6)
 merge(o)
# Export copies with their own pivots, leaving the original animated hierarchy intact.
for p in json.loads((OUT/'parts.json').read_text())['parts']:
 o=bpy.data.objects[p['name']];copy=o.copy();copy.data=o.data.copy();copy.animation_data_clear();bpy.context.scene.collection.objects.link(copy);copy.parent=None;copy.matrix_world=Matrix.Identity(4)
 bpy.ops.object.select_all(action='DESELECT');copy.select_set(True);bpy.context.view_layer.objects.active=copy
 bpy.ops.export_scene.fbx(filepath=str(ROOT/'SourceAssets'/p['source_file']),use_selection=True,object_types={'MESH'},add_leaf_bones=False,bake_anim=False,axis_forward='-Y',axis_up='Z',apply_unit_scale=True)
 bpy.data.objects.remove(copy,do_unlink=True)
scene=bpy.context.scene
for f,label in [(1,'Detached'),(150,'Coupled')]:scene.frame_set(f);scene.render.filepath=str(OUT/('Review_'+label+'.png'));bpy.ops.render.render(write_still=True)
scene.frame_set(1);bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Foundryline_Tanker_Tractor.blend'));print('DETAILS_AND_UV_COMPLETE')
