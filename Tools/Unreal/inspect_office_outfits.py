import unreal,json
from pathlib import Path
r=Path(unreal.Paths.project_dir()).resolve();out=r/'SourceAssets/Employees/OfficeOutfitReview';out.mkdir(parents=True,exist_ok=True)
q={}
a=unreal.load_asset('/Game/Foundryline/Employees/Handyman/DA_HandymanEmployees')
q['outfits']=[[str(x.get_path_name()) for x in o.get_editor_property('parts')] for o in a.get_editor_property('outfits')]
for kind,count in [('Torso',3),('Legs',3),('Body',1),('Head',1),('Boots',1),('Hair',1)]:
 for i in range(1,count+1):
  name=f'SK_{kind}_{i}_SE';m=unreal.load_asset('/Game/Handyman/Mesh/Handyman_SE/Parts/'+name)
  if not m:continue
  t=unreal.AssetExportTask();t.object=m;t.filename=str(out/(name+'.fbx'));t.automated=True;t.replace_identical=True;t.prompt=False;unreal.Exporter.run_asset_export_task(t)
  q[name]=json.loads(unreal.FoundrylineHandymanAssetTools.inspect_mesh(m))
t=unreal.AssetExportTask();t.object=unreal.load_asset('/Game/Handyman/Textures/T_Main_colors');t.filename=str(out/'Palette.png');t.automated=True;t.replace_identical=True;t.prompt=False;unreal.Exporter.run_asset_export_task(t)
(r/'Saved/Logs/OfficeOutfitInspection.json').write_text(json.dumps(q,indent=2));unreal.SystemLibrary.quit_editor()
