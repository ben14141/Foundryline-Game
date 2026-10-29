import unreal,json
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve();reportpath=root/'Saved/Logs/SheetRollerImport.json';previous=json.loads(reportpath.read_text());path=root/'Tools/Unreal/import_sheet_roller_assets.py';code=path.read_text().replace("for a in data['assets']:","for a in [a for a in data['assets'] if a['name']=='SheetRoller_UpperCarriers']:")
exec(compile(code,str(path),'exec'));fixed=json.loads(reportpath.read_text());(root/'Saved/Logs/RollerCarrierFixedImport.json').write_text(json.dumps(fixed,indent=2))
for a in previous['assets']:
 if 'SM_SheetRoller_UpperCarriers.' in a['path']:a.update(fixed['assets'][0])
actual=fixed['assets'][0]['dimensions_cm'];assert actual[1]>350 and actual[2]<60,actual
reportpath.write_text(json.dumps(previous,indent=2));unreal.log('SINGLE_ROLLER_CARRIER_IMPORT_FIXED_COMPLETE')
