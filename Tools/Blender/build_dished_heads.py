"""Author complete dished tank heads at real scale."""
import bpy,sys,json,bmesh
from pathlib import Path
from mathutils import Vector
sys.path.insert(0,str(Path(__file__).parent))
from tank_head_geometry import make_head
exec((Path(__file__).parent/'build_material_handling_assets.py').read_text().split('def import_factory')[0])
OUT=R/'SourceAssets/Products/Components/DishedHeads';OUT.mkdir(parents=True,exist_ok=True)
assets=[];objects=[]
for name in ['TankHead_ASME_FD']:
 o,spec=make_head(name,sheetmat);uv_planar(o);bpy.context.view_layer.update()
 bm=bmesh.new();bm.from_mesh(o.data);assert all(e.is_manifold for e in bm.edges),name;bm.free()
 dims=list(o.dimensions*100);assert abs(dims[0]-235)<.05,name
 spec.update(name=name,dimensions_cm=dims,triangles=sum(len(p.vertices)-2 for p in o.data.polygons));assets.append(spec);objects.append(o)
 bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o
 bpy.ops.export_scene.fbx(filepath=str(OUT/(name+'.fbx')),use_selection=True,object_types={'MESH'},bake_anim=False,axis_forward='-Y',axis_up='Z',mesh_smooth_type='FACE')
# A single complete head is the production inventory item.
objects[0].location=(0,0,0)
bpy.ops.mesh.primitive_plane_add(size=100,location=(0,0,-.012));bpy.context.object.data.materials.append(mat('HeadReviewFloor',(.075,.095,.12),.8))
bpy.ops.object.camera_add(location=(5,-7,7));camera=bpy.context.object;camera.rotation_euler=(Vector((0,0,.15))-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.type='ORTHO';camera.data.ortho_scale=3.6;scene.camera=camera
for loc,power,size in [((0,-3,7),2400,5),((-5,1,5),1800,5),((4,5,6),2400,4)]:
 bpy.ops.object.light_add(type='AREA',location=loc);l=bpy.context.object;l.data.energy=power;l.data.size=size;l.rotation_euler=(-l.location).to_track_quat('-Z','Y').to_euler()
scene.world=bpy.data.worlds.new('HeadReview');scene.world.color=(.22,.22,.22);scene.render.engine='CYCLES';scene.cycles.samples=48;scene.render.resolution_x=1600;scene.render.resolution_y=1100;scene.render.resolution_percentage=100;scene.render.filepath=str(OUT/'DishedHeads_Review.png');bpy.ops.render.render(write_still=True)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Foundryline_DishedHeads.blend'))
(OUT/'manifest.json').write_text(json.dumps({'assets':assets,'notes':'Complete heads only. Each tanker uses two whole heads. Forming process is not implemented by this asset script.'},indent=2));print('DISHED_HEADS_COMPLETE')
