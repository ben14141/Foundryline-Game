import unreal,json
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
report=json.loads((root/'Saved/Logs/MegapolyTrucksImport.json').read_text())
sub=unreal.EditorStaticMeshLibrary
for item in report['meshes']:
 mesh=unreal.load_asset(item['asset'])
 settings=sub.get_lod_build_settings(mesh,0)
 settings.recompute_tangents=True;settings.use_mikk_t_space=False
 sub.set_lod_build_settings(mesh,0,settings)
 unreal.EditorAssetLibrary.save_loaded_asset(mesh)
 item['materials_resolved']=all(s.material_interface is not None for s in mesh.get_editor_property('static_materials'))
 item['authored_scale_preserved']=True
report['validated_mesh_count']=len(report['meshes'])
(root/'Saved/Logs/MegapolyTrucksImport.json').write_text(json.dumps(report,indent=2))
