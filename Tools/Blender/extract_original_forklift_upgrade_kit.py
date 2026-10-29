"""Extract ONLY our procedural addon objects; never redistribute licensed base meshes."""
import bpy,json
from pathlib import Path
R=Path(__file__).resolve().parents[2];SOURCE=R/'SourceAssets/Vehicles/Forklift/QualityReview_2026-10-29/Forklift_QualityReview.blend';O=R/'SourceAssets/Vehicles/Forklift/PublicUpgradeKit_2026-10-29';O.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(SOURCE));S=bpy.context.scene;S.frame_set(1);names=['Forklift_Body','Forklift_MastOuter','Forklift_MastInner','Forklift_Carriage','Forklift_Wheel_FL','Forklift_Wheel_FR','Forklift_Wheel_RL','Forklift_Wheel_RR'];roots=[];removed={}
for name in names:
 old=bpy.data.objects[name];assert old.type=='MESH';removed[name]=len(old.data.vertices);root=bpy.data.objects.new('Attachment_'+name,None);S.collection.objects.link(root);root.matrix_world=old.matrix_world.copy();root['licensed_local_target']=name;roots.append(root)
 for child in list(old.children):
  M=child.matrix_world.copy();child.parent=root;child.matrix_world=M
 bpy.data.objects.remove(old,do_unlink=True)
assert all(o.type!='MESH' or not o.name.startswith(tuple(names)) for o in bpy.data.objects)
# Only original addon descendants survive; remove any unrelated scene objects.
keep={o for root in roots for o in [root,*root.children_recursive]}
for o in list(bpy.data.objects):
 if o not in keep:bpy.data.objects.remove(o,do_unlink=True)
bpy.data.orphans_purge(do_local_ids=True,do_linked_ids=True,do_recursive=True)
assert not bpy.data.libraries
assert not any(i.source=='FILE' for i in bpy.data.images)
assert all(not m.name.startswith('MH_') for m in bpy.data.materials),[m.name for m in bpy.data.materials]
assert all(not any(n.type=='TEX_IMAGE' for n in m.node_tree.nodes) for m in bpy.data.materials if m.use_nodes)
def export(path,objects):
 bpy.ops.object.select_all(action='DESELECT')
 for o in objects:o.select_set(True)
 bpy.context.view_layer.objects.active=next(o for o in objects if o.type=='MESH');bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'MESH','EMPTY','OTHER'},apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',bake_anim=False,add_leaf_bones=False,mesh_smooth_type='FACE')
rows=[]
for root in roots:
 objects=[root,*root.children_recursive];meshes=[o for o in objects if o.type in {'MESH','CURVE'}]
 if not meshes:continue
 name=root['licensed_local_target']+'_OriginalAddons';export(O/(name+'.fbx'),objects);rows.append({'file':name+'.fbx','attach_to_local_licensed_component':root['licensed_local_target'],'attachment_matrix':[list(v) for v in root.matrix_world],'objects':[o.name for o in meshes]})
export(O/'Forklift_OriginalUpgradeKit.fbx',list(bpy.data.objects));bpy.ops.wm.save_as_mainfile(filepath=str(O/'Forklift_OriginalUpgradeKit.blend'))
report={'original_authorship':True,'no_licensed_geometry':True,'removed_licensed_base_vertex_counts':removed,'original_addon_objects':sum(o.type in {'MESH','CURVE'} for o in bpy.data.objects),'no_external_images':True,'no_linked_libraries':True,'material_names':[m.name for m in bpy.data.materials],'component_addons':rows,'units':'metres source / centimetres Unreal','installation':'Optional addon attachment source; for complete local vehicle revision run the licensed-input build/refine/label scripts. Never publish the generated complete licensed-derived vehicle.'}
(O/'manifest.json').write_text(json.dumps(report,indent=2));print('ORIGINAL_FORKLIFT_UPGRADE_KIT_SAFE',report['original_addon_objects'],'objects; removed ALL8 licensed meshes')
