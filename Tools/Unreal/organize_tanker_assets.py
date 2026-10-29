import unreal,json
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve();data=json.loads((root/'SourceAssets/AssemblyReviews/TankerTractor/parts.json').read_text());report=[]
for p in data['parts']:
 old='/Game/Foundryline/Vehicles/TankerRig/SM_'+p['name'];new=p['asset_path']
 if unreal.EditorAssetLibrary.does_asset_exist(old):
  if not unreal.EditorAssetLibrary.rename_asset(old,new):raise RuntimeError('Rename failed '+old)
 mesh=unreal.load_asset(new)
 if not mesh:raise RuntimeError('Missing '+new)
 unreal.EditorAssetLibrary.save_loaded_asset(mesh);report.append(new)
for name in ['DetailRubber','DetailMetal','DetailRed','DetailWhite']:
 old='/Game/Foundryline/Vehicles/TankerRig/Materials/M_'+name;new='/Game/Foundryline/Vehicles/Shared/Materials/M_'+name
 if unreal.EditorAssetLibrary.does_asset_exist(old):
  if not unreal.EditorAssetLibrary.rename_asset(old,new):raise RuntimeError('Material rename failed')
for p in data['parts']:
 unreal.EditorAssetLibrary.save_asset(p['asset_path'],False)
(root/'Saved/Logs/TankerAssetOrganization.json').write_text(json.dumps(report,indent=2))
