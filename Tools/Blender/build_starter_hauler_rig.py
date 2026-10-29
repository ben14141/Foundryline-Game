"""Build the editable rigid-part control rig, matching the earlier tanker workflow.
Unreal uses the same exported rest meshes and drives motion from distance/state.
"""
import bpy,math,json
from pathlib import Path
from mathutils import Vector
ROOT=Path(r'C:/Users/curly/Documents/Foundryline Game');SRC=ROOT/'SourceAssets/StarterHauler';OUT=ROOT/'SourceAssets/AssemblyReviews/StarterHauler';OUT.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(SRC/'StarterFlatbed.blend'))
scene=bpy.context.scene;scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1;scene.render.fps=30;scene.frame_start=1;scene.frame_end=420
root=bpy.data.objects.new('StarterHauler_ROOT',None);scene.collection.objects.link(root);root.empty_display_type='ARROWS';root.empty_display_size=1
for name,value,lo,hi in [('travel_metres',0.,-100.,100.),('steering_degrees',0.,-35.,35.),('loading_gate_open',0.,0.,1.)]:
 root[name]=value;root.id_properties_ui(name).update(min=lo,max=hi,description={'travel_metres':'Signed wheel travel in metres; wheel radius is from authored geometry','steering_degrees':'Front axle steering angle in degrees','loading_gate_open':'0 closed for travel, 1 open for loading'}[name])
body=bpy.data.objects['StarterFlatbed'];body.name='Hauler_Body';body.parent=root
parts=[]
def imported(file,name,parent,at):
 before=set(bpy.data.objects);bpy.ops.import_scene.fbx(filepath=str(file));objs=[o for o in set(bpy.data.objects)-before if o.type=='MESH'];assert len(objs)==1,(file,len(objs));o=objs[0];o.name=name;o.parent=parent;o.location=at;o.rotation_euler=(0,0,0);o.scale=(1,1,1);return o

def drive(owner,path,index,prop,expression):
 f=owner.driver_add(path,index);v=f.driver.variables.new();v.name='control';v.type='SINGLE_PROP';v.targets[0].id=root;v.targets[0].data_path='["'+prop+'"]';f.driver.expression=expression

manifest=json.loads((SRC/'manifest.json').read_text());wheels=[]
for i,row in enumerate(manifest['wheels']):
 at=Vector(row['centre_cm'])/100;pivot=bpy.data.objects.new('Steer_FL' if i==0 else 'Steer_FR' if i==1 else 'RearAxle_'+str(i),None);scene.collection.objects.link(pivot);pivot.parent=root;pivot.location=at;pivot.empty_display_type='CIRCLE';pivot.empty_display_size=.25
 if i<2:drive(pivot,'rotation_euler',2,'steering_degrees','control*pi/180')
 o=imported(SRC/f'Wheel{i}.fbx','Hauler_Wheel_'+str(i),pivot,Vector());radius=row['size_cm'][2]/200;drive(o,'rotation_euler',1,'travel_metres',f'control/{radius:.9f}');wheels.append(o);parts.append({'name':o.name,'pivot_cm':row['centre_cm'],'radius_cm':radius*100,'steers':i<2})
gate=imported(SRC/'LoadingSideGate.fbx','Hauler_LoadingSideGate',root,Vector((-1.344,1.28,1.129513)));gate.data.materials.clear();gate.data.materials.append(bpy.data.materials['TruckCabPaint']);drive(gate,'rotation_euler',0,'loading_gate_open','control*pi')
for name,at in [('LoadAnchor_A',(-1.994,.60,1.13)),('LoadAnchor_B',(-.694,.60,1.13))]:
 o=bpy.data.objects.new(name,None);scene.collection.objects.link(o);o.parent=root;o.location=at;o.empty_display_type='CUBE';o.empty_display_size=.25
# A single named action animates controls. Rigid parts remain independently editable.
keys=[(1,(0,0,0),0,0,0,0),(100,(10,0,0),0,10,0,0),(125,(12.329,0.307,0),15,12.356,31,0),(150,(14.5,1.206,0),30,14.712,31,0),(180,(16.364,2.636,0),45,17.069,0,0),(195,(16.364,2.636,0),45,17.069,0,0),(225,(16.364,2.636,0),45,17.069,0,1),(270,(16.364,2.636,0),45,17.069,0,1),(300,(16.364,2.636,0),45,17.069,0,0),(420,(24,10.272,0),45,27.868,0,0)]
for frame,at,yaw,travel,steer,opened in keys:
 root.location=at;root.rotation_euler.z=math.radians(yaw);root['travel_metres']=travel;root['steering_degrees']=steer;root['loading_gate_open']=opened
 for path in ['location','rotation_euler','["travel_metres"]','["steering_degrees"]','["loading_gate_open"]']:root.keyframe_insert(path,frame=frame)
root.animation_data.action.name='Hauler_Drive_Steer_LoadingGate_Demo'
for frame,label in [(1,'Ready / gate closed'),(100,'Approach turn'),(150,'Front axle steers'),(180,'Parked'),(225,'Gate fully open / load'),(270,'Loading complete'),(300,'Gate closed'),(420,'Departed')]:scene.timeline_markers.new(label,frame=frame)
# Pack materials so the Blender rig is self-contained.
tex=bpy.data.images.load(str(ROOT/'SourceAssets/MegapolySmallTrucks/Textures/Tex_256Gradient.png'));tex.pack()
for m in bpy.data.materials:
 m.use_nodes=True;nodes=m.node_tree.nodes;nodes.clear();bs=nodes.new('ShaderNodeBsdfPrincipled');out=nodes.new('ShaderNodeOutputMaterial');m.node_tree.links.new(bs.outputs['BSDF'],out.inputs['Surface']);bs.inputs['Roughness'].default_value=.5
 if 'glass' in m.name.lower():bs.inputs['Base Color'].default_value=(.025,.055,.08,1);bs.inputs['Roughness'].default_value=.15
 elif 'cabpaint' in m.name.lower():bs.inputs['Base Color'].default_value=(.82,.84,.86,1)
 else:n=nodes.new('ShaderNodeTexImage');n.image=tex;m.node_tree.links.new(n.outputs['Color'],bs.inputs['Base Color'])
# Source validation checks the actual evaluated transforms at several animation frames.
q={};scene.frame_set(1);bpy.context.view_layer.update();a=wheels[0].matrix_world.copy();q['six_wheels']=len(wheels)==6;q['six_tyre_contacts_at_ground']=all(abs(min((w.matrix_world@v.co).z for v in w.data.vertices))<.001 for w in wheels);q['unit_mesh_scale']=all((Vector(o.scale)-Vector((1,1,1))).length<.001 for o in [body,gate]+wheels)
scene.frame_set(150);bpy.context.view_layer.update();q['front_steers']=abs(wheels[0].parent.rotation_euler.z)>.4 and abs(wheels[1].parent.rotation_euler.z)>.4;q['rear_does_not_steer']=all(abs(w.parent.rotation_euler.z)<.001 for w in wheels[2:]);q['wheels_turn_from_distance']=abs(wheels[0].rotation_euler.y)>20
scene.frame_set(225);bpy.context.view_layer.update();q['gate_opens_180_degrees']=abs(abs(gate.rotation_euler.x)-math.pi)<.001
scene.frame_set(300);bpy.context.view_layer.update();q['gate_closes_for_departure']=abs(gate.rotation_euler.x)<.001;q['named_action']=root.animation_data.action.name=='Hauler_Drive_Steer_LoadingGate_Demo'
q['control_drivers']=sum(len(o.animation_data.drivers) if o.animation_data else 0 for o in list(bpy.data.objects))==9
scene.frame_set(1);bpy.context.view_layer.update()
(ROOT/'Saved/Logs/StarterHaulerBlenderRigQA.json').write_text(json.dumps(q,indent=2));assert all(q.values()),q
bpy.ops.mesh.primitive_plane_add(size=120,location=(10,0,-.025));ground=bpy.context.object;ground.name='ReviewGround'
bpy.ops.object.light_add(type='AREA',location=(2,-6,14));bpy.context.object.data.energy=3000;bpy.context.object.data.size=12
bpy.ops.object.camera_add(location=(10,13,8));cam=bpy.context.object;cam.parent=root;cam.location=(10,13,8);cam.rotation_euler=(Vector((0,0,1.4))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=14;scene.camera=cam;scene.world=bpy.data.worlds.new('RigReviewWorld');scene.world.color=(.3,.3,.3);scene.render.engine='CYCLES';scene.cycles.samples=20;scene.render.resolution_x=1100;scene.render.resolution_y=750;scene.render.resolution_percentage=100
bpy.context.view_layer.objects.active=root;bpy.ops.object.select_all(action='DESELECT');root.select_set(True)
for area in bpy.context.screen.areas:
 if area.type=='VIEW_3D':area.spaces.active.region_3d.view_perspective='CAMERA'
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Foundryline_StarterHauler_Rig.blend'))
(OUT/'rig_manifest.json').write_text(json.dumps({'rig':'StarterHauler_ROOT','action':root.animation_data.action.name,'controls':['travel_metres','steering_degrees','loading_gate_open'],'frame_range':[1,420],'parts':parts,'runtime':'Same independent rest meshes; distance, steering and booking state drive them in Unreal'},indent=2))
print('STARTER_HAULER_RIG_QA',q)
