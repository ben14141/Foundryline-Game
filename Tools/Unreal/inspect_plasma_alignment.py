import unreal,json
from pathlib import Path
mesh=unreal.load_asset('/Game/Foundryline/Production/Equipment/PlasmaTable/SM_PlasmaTable_Torch')
b=mesh.get_bounds()
out={'origin':list(b.origin.to_tuple()),'extent':list(b.box_extent.to_tuple())}
(Path(unreal.Paths.project_saved_dir())/'Logs/PlasmaAlignment.json').write_text(json.dumps(out,indent=2))
