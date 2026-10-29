import bpy,math,json
from pathlib import Path
from mathutils import Vector,Matrix
root=Path(r'C:\Users\curly\Documents\Foundryline Game')
bpy.ops.wm.read_factory_settings(use_empty=True)
for idx,name in enumerate(['3_axle_Tanker_1','3_axle_Tanker_2']):
 before=set(bpy.data.objects);bpy.ops.import_scene.fbx(filepath=str(root/'SourceAssets/MegapolyTrucks'/(name+'.fbx')))
 new=set(bpy.data.objects)-before
 for o in new:
  if o.type!='MESH':continue
  mat=o.matrix_world.copy()
  if o.parent:mat=Matrix.Rotation(-math.pi/2,4,'X')@Matrix.Scale(.01,4)@mat
  o.parent=None;o.matrix_world=mat;o.location.y+=idx*7
  for m in o.data.materials:
   if m:
    m.use_nodes=True;bs=next((n for n in m.node_tree.nodes if n.type=='BSDF_PRINCIPLED'),None)
    if bs:bs.inputs['Base Color'].default_value=(.75,.77,.8,1)
bpy.ops.object.camera_add(location=(22,-24,19));cam=bpy.context.object;cam.rotation_euler=(Vector((0,3,1.5))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=27;bpy.context.scene.camera=cam
bpy.ops.object.light_add(type='AREA',location=(0,-5,15));bpy.context.object.data.energy=2500;bpy.context.object.data.shape='DISK';bpy.context.object.data.size=12
sc=bpy.context.scene;sc.render.engine='CYCLES';sc.cycles.samples=16;sc.world=bpy.data.worlds.new('ReviewWorld');sc.world.color=(.3,.3,.3);sc.render.resolution_x=1000;sc.render.resolution_y=720;sc.render.resolution_percentage=100;sc.render.filepath=str(root/'Saved/Logs/TankerComparison.png');bpy.ops.render.render(write_still=True)
