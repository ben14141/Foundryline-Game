import runpy,unreal
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
for file in ['import_plasma_table_assets.py','import_press_brake_assets.py','import_sheet_roller_assets.py']:
 unreal.log('EQUIPMENT_QUALITY_IMPORT_START '+file);runpy.run_path(str(root/'Tools/Unreal'/file),run_name='__main__')
unreal.log('EQUIPMENT_QUALITY_IMPORT_ALL_COMPLETE')
