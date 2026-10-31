import bpy
from pathlib import Path
from mathutils import Vector
r=Path(__file__).resolve().parents[2];src=r/'SourceAssets/Employees/OfficeOutfitReview'
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
tex=bpy.data.images.load(str(src/'Palette.png'))
for i in range(3):
 for name in [f'SK_Torso_{i+1}_SE','SK_Legs_1_SE','SK_Head_1_SE','SK_Hair_1_SE','SK_Boots_1_SE']:
  before=set(bpy.data.objects);bpy.ops.import_scene.fbx(filepath=str(src/(name+'.fbx')))
  for o in set(bpy.data.objects)-before:
   if not o.parent:o.location.x+=(i-1)*1.7
   if o.type=='MESH':
    for m in o.data.materials:
     if not m:continue
     m.use_nodes=True;nt=m.node_tree;p=nt.nodes.get('Principled BSDF');t=nt.nodes.new('ShaderNodeTexImage');t.image=tex;nt.links.new(t.outputs['Color'],p.inputs['Base Color']);p.inputs['Roughness'].default_value=.65
scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=16;scene.render.resolution_x=1200;scene.render.resolution_y=650;scene.world.color=(.5,.5,.5)
bpy.ops.object.camera_add(location=(3,8,3));c=bpy.context.object;c.rotation_euler=(Vector((0,0,.9))-c.location).to_track_quat('-Z','Y').to_euler();c.data.type='ORTHO';c.data.ortho_scale=5.8;scene.camera=c
for pos,power in [((0,3,5),1000),((-4,0,3),700),((2,-3,4),900)]:
 bpy.ops.object.light_add(type='AREA',location=pos);l=bpy.context.object;l.data.energy=power;l.data.size=5;l.rotation_euler=(Vector((0,0,1))-l.location).to_track_quat('-Z','Y').to_euler()
scene.render.filepath=str(r/'Saved/Screenshots/OfficeOutfitOptions.png');bpy.ops.render.render(write_still=True)
