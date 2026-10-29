import unreal,json
from pathlib import Path
asset=unreal.load_asset('/Game/Foundryline/Employees/Handyman/DA_HandymanEmployees')
before=asset.get_editor_property('facing_offset')
rotation=unreal.Rotator(pitch=0,yaw=-90,roll=0)
asset.set_editor_property('facing_offset',rotation)
assert abs(rotation.pitch)<.001 and abs(rotation.yaw+90)<.001 and abs(rotation.roll)<.001
assert unreal.EditorAssetLibrary.save_loaded_asset(asset)
report={'before':str(before),'after':str(rotation)}
Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()),'Logs/HandymanFacingFix.json').write_text(json.dumps(report,indent=2))
