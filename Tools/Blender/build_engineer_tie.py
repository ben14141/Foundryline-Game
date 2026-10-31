import bpy,json
from pathlib import Path
r=Path(__file__).resolve().parents[2];out=r/'SourceAssets/Employees/Office';src=r/'SourceAssets/Employees/OfficeOutfitReview'
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False);bpy.ops.import_scene.fbx(filepath=str(src/'SK_Torso_2_SE.fbx'));arm=next(o for o in bpy.data.objects if o.type=='ARMATURE')
for o in list(bpy.data.objects):
 if o.type=='MESH':bpy.data.objects.remove(o,do_unlink=True)
# Native ref pose uses -Y as chest/front. Dimensions are actual centimetres.
verts=[(-1.6,-14.8,144),(1.6,-14.8,144),(2.1,-14.9,140),(0,-15,138),(-2.1,-14.9,140),(-1.2,-15.1,139),(1.2,-15.1,139),(2.8,-15,117),(0,-15,114),(-2.8,-15,117)]
m=bpy.data.meshes.new('OfficeTie');m.from_pydata([tuple(v/100 for v in p) for p in verts],[],[(0,1,2,3,4),(5,6,7,8,9)]);o=bpy.data.objects.new('OfficeEngineerTie',m);bpy.context.collection.objects.link(o);uv=m.uv_layers.new();coord=json.loads((out/'palette.json').read_text())['navy_uv']
for loop in uv.data:loop.uv=coord
mat=bpy.data.materials.new('Office_Engineer_Navy');mat.diffuse_color=(.025,.035,.06,1);m.materials.append(mat)
g=o.vertex_groups.new(name='spine_03');g.add(list(range(len(verts))),1,'REPLACE');mod=o.modifiers.new('Engineer skeleton','ARMATURE');mod.object=arm;o.parent=arm;o.matrix_parent_inverse=arm.matrix_world.inverted()
bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o
bpy.ops.export_scene.fbx(filepath=str(out/'OfficeEngineerTie.fbx'),use_selection=True,object_types={'MESH'},add_leaf_bones=False,bake_anim=False,apply_unit_scale=True,axis_forward='-Z',axis_up='Y')
