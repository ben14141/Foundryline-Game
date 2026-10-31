"""Render the five original wall decorations at authored scale in an isolated scene."""
import bpy,math
from pathlib import Path
from mathutils import Vector
R=Path(__file__).resolve().parents[2]
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
for name,p in [('WallClock',(-2.25,.04,2)),('FramedCertificate',(-.95,.04,1.7)),('IndustrialWallArt',(1.1,.04,1.8)),('EngineeringWhiteboard',(-1.45,.04,.45)),('OfficeNoticeboard',(1.1,.04,.5))]:
 before=set(bpy.data.objects);bpy.ops.import_scene.fbx(filepath=str(R/'SourceAssets/Office/Decor'/('SM_'+name+'.fbx')))
 for o in set(bpy.data.objects)-before:
  if not o.parent:o.location+=Vector(p)
def box(p,s,c):
 bpy.ops.mesh.primitive_cube_add(size=1,location=p);o=bpy.context.object;o.dimensions=s;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);m=bpy.data.materials.new('ReviewSurface');m.diffuse_color=(*c,1);m.use_nodes=True;m.node_tree.nodes.get('Principled BSDF').inputs['Base Color'].default_value=(*c,1);o.data.materials.append(m)
box((0,-.08,1.65),(6.5,.12,3.3),(.77,.76,.72));box((0,1,-.08),(12,8,.15),(.15,.17,.19));box((0,0,.13),(6.5,.08,.09),(.04,.05,.06))
scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=24;scene.render.resolution_x=1600;scene.render.resolution_y=1000;scene.render.resolution_percentage=100;scene.world.color=(.35,.35,.35)
bpy.ops.object.camera_add(location=(2.9,8,3.4));c=bpy.context.object;c.rotation_euler=(Vector((0,0,1.65))-c.location).to_track_quat('-Z','Y').to_euler();c.data.type='ORTHO';c.data.ortho_scale=7;scene.camera=c
for pos,power,size in [((0,4,6),1300,5),((-4,3,3),700,3),((4,1,4),500,3)]:
 bpy.ops.object.light_add(type='AREA',location=pos);l=bpy.context.object;l.data.energy=power;l.data.shape='DISK';l.data.size=size;l.rotation_euler=(Vector((0,0,1.5))-l.location).to_track_quat('-Z','Y').to_euler()
scene.render.filepath=str(R/'Saved/Screenshots/OfficeWallDecor.png');bpy.ops.render.render(write_still=True)
