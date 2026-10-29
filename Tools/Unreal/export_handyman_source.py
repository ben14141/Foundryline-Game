import unreal,json
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))/'SourceAssets/Employees/Handyman/Imported'
root.mkdir(parents=True,exist_ok=True)
assets=unreal.AssetRegistryHelpers.get_asset_registry().get_assets_by_path('/Game/Handyman/Mesh/Handyman',True)
results=[]
for data in assets:
 if str(data.asset_class_path.asset_name)!='SkeletalMesh':continue
 obj=data.get_asset();task=unreal.AssetExportTask();task.object=obj;task.filename=str(root/(obj.get_name()+'.fbx'));task.automated=True;task.prompt=False;task.replace_identical=True;task.exporter=unreal.SkeletalMeshExporterFBX();task.options=unreal.FbxExportOption();task.options.ascii=False
 results.append({'asset':obj.get_path_name(),'ok':unreal.Exporter.run_asset_export_task(task),'errors':list(task.errors)})
for name in ['T_Main_colors','T_Atribute','T_Emission_colors']:
 obj=unreal.load_asset('/Game/Handyman/Textures/'+name);task=unreal.AssetExportTask();task.object=obj;task.filename=str(root/(name+'.png'));task.automated=True;task.prompt=False;task.replace_identical=True;task.exporter=unreal.TextureExporterPNG();unreal.Exporter.run_asset_export_task(task)
for name,path in [('Walk','/Game/Handyman/Animations/Handyman/anim_Walk_1'),('WalkInPlace','/Game/Handyman/Demo/Animations/anim_MM_Walk_InPlace'),('Idle','/Game/Handyman/Animations/Handyman/anim_Idle'),('Fixing','/Game/Handyman/Animations/Handyman/anim_Fixing')]:
 obj=unreal.load_asset(path);task=unreal.AssetExportTask();task.object=obj;task.filename=str(root/(name+'.fbx'));task.automated=True;task.prompt=False;task.replace_identical=True;task.exporter=unreal.AnimSequenceExporterFBX();task.options=unreal.FbxExportOption();task.options.export_preview_mesh=True
 results.append({'asset':obj.get_path_name(),'ok':unreal.Exporter.run_asset_export_task(task),'errors':list(task.errors)})
(root/'ExportReport.json').write_text(json.dumps(results,indent=2))
