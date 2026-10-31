"""Import the game-owned sign meshes; source pivots and centimetres are preserved."""
import unreal,json
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve()
folder='/Game/Foundryline/Buildings/Signs/Manufacturing'
tasks=[]
for part in ['Frame','Panel','Accent','Hardware']:
    name='SM_ManufacturingSign_'+part
    task=unreal.AssetImportTask();task.filename=str(root/'SourceAssets/Buildings/ManufacturingSign'/(name+'.fbx'));task.destination_path=folder;task.destination_name=name
    task.automated=True;task.replace_existing=True;task.save=True
    options=unreal.FbxImportUI();options.import_mesh=True;options.import_materials=False;options.import_textures=False;options.import_as_skeletal=False
    options.static_mesh_import_data.combine_meshes=True
    options.static_mesh_import_data.auto_generate_collision=False
    options.static_mesh_import_data.convert_scene=True
    options.static_mesh_import_data.convert_scene_unit=True
    task.options=options;tasks.append(task)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
report={}
for task in tasks:
    mesh=unreal.load_asset(folder+'/'+task.destination_name)
    if not mesh:raise RuntimeError('Missing sign mesh '+task.destination_name)
    bounds=mesh.get_bounds();report[task.destination_name]={'origin':str(bounds.origin),'extent':str(bounds.box_extent)}
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
(root/'Saved/Logs/ManufacturingSignImport.json').write_text(json.dumps(report,indent=2))
unreal.SystemLibrary.quit_editor()
