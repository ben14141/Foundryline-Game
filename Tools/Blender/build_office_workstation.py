"""Rebuild only OfficeWorkstation, preserving other operations assets."""
import bpy,json,math
from pathlib import Path
R=Path(r'C:/Users/curly/Documents/Foundryline Game')
source=(R/'Tools/Blender/build_operations_assets.py').read_text();exec(source.split('# Floor-mounted loading tablet.')[0])
exec((R/'Tools/Blender/office_workstation_geometry.py').read_text())
# Keep individual bevelled parts in a hidden editable collection before joining for UE.
auth=bpy.data.collections.new('Editable_Workstation_Parts');bpy.context.scene.collection.children.link(auth)
for o in group:
 c=o.copy();c.data=o.data.copy();c.name='Authoring__'+o.name;auth.objects.link(c);c.hide_render=True;c.hide_set(True)
auth.hide_render=True;auth.hide_viewport=True
export('OfficeWorkstation','UI/Operations');obj=bpy.data.objects['OfficeWorkstation'];obj.hide_set(False)
scene=bpy.context.scene;scene.world=bpy.data.worlds.new('OfficeReviewWorld');scene.world.color=(.18,.18,.18)
bpy.ops.object.camera_add(location=(2.3,3.3,2.1));camera=bpy.context.object;camera.rotation_euler=(Vector((0,0,.85))-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.type='ORTHO';camera.data.ortho_scale=2.55;scene.camera=camera
for loc,power,size in [((1,2,3.5),600,3),((-2,-1,2.5),450,3)]:bpy.ops.object.light_add(type='AREA',location=loc);bpy.context.object.data.energy=power;bpy.context.object.data.size=size
scene.render.engine='CYCLES';scene.cycles.samples=24;scene.render.resolution_x=1300;scene.render.resolution_y=1000;scene.render.resolution_percentage=100
OUT2=OUT/'OfficeWorkstation';OUT2.mkdir(exist_ok=True);scene.render.filepath=str(OUT2/'OfficeWorkstation_TripleReview.png')
verts=[obj.matrix_world@Vector(v) for v in obj.bound_box];lo=[min(v[k] for v in verts)*100 for k in range(3)];hi=[max(v[k] for v in verts)*100 for k in range(3)]
report={'units':'metres in Blender; centimetres in Unreal','bounds_min_cm':lo,'bounds_max_cm':hi,'screens':screen_specs,'material_slots':[m.name for m in obj.data.materials],'vertices':len(obj.data.vertices),'polygons':len(obj.data.polygons),'editable_parts':len(auth.objects)}
assert len(screen_specs)==3 and max(abs(lo[0]),abs(hi[0]))<84 and hi[2]<132,report
(OUT2/'manifest.json').write_text(json.dumps(report,indent=2));bpy.ops.wm.save_as_mainfile(filepath=str(OUT2/'Foundryline_OfficeWorkstation.blend'));bpy.ops.render.render(write_still=True)
