import bpy
from pathlib import Path
from mathutils import Vector
r=Path(r'C:/Users/curly/Documents/Foundryline Game')
bpy.ops.wm.open_mainfile(filepath=str(r/'SourceAssets/MaterialHandling/Authored/Foundryline_MaterialHandling.blend'))
for o in bpy.data.objects:
 if not (o.name.startswith('Forklift_') or o.type=='LIGHT'):o.hide_render=True
with bpy.data.libraries.load(str(r/'SourceAssets/Employees/Prototype/Foundryline_PrototypeEmployee.blend')) as (f,t):
 t.objects=[n for n in f.objects if n not in ['ReviewFloor','Camera'] and not n.startswith('Area')];t.actions=['Employee_Seated']
for o in t.objects:
 if o and o.name not in bpy.context.scene.objects:bpy.context.scene.collection.objects.link(o)
a=t.actions[0]
for o in t.objects:
 if not o:continue
 slot=next((s for s in a.slots if s.identifier=='OB'+o.name),None)
 if slot:o.animation_data_create();o.animation_data.action=a;o.animation_data.action_slot=slot
root=bpy.data.objects['Employee_Control_Root'];root.location=(-1.06,0,0);s=bpy.context.scene;s.frame_set(1);bpy.ops.object.camera_add(location=(1.1,-4.5,2.1));c=bpy.context.object;c.rotation_euler=(Vector((-1.1,0,1.5))-c.location).to_track_quat('-Z','Y').to_euler();c.data.type='ORTHO';c.data.ortho_scale=2.7;s.camera=c;s.render.resolution_x=1000;s.render.resolution_y=900;s.render.resolution_percentage=100;s.cycles.samples=16;s.render.filepath=str(r/'SourceAssets/Employees/Prototype/PrototypeEmployee_ForkSeat.png');bpy.ops.render.render(write_still=True)
