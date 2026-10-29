from pathlib import Path
import json
import unreal
root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
assets = ['/Game/Factory/Meshes/Road/SM_Lane_100', '/Game/Factory/Meshes/Road/SM_Lane_400', '/Game/Factory/Meshes/Road/Border/SM_Border_100', '/Game/Factory/Meshes/Road/Border/SM_Border_400']
data = []
for path in assets:
    mesh = unreal.load_asset(path)
    b = mesh.get_bounds()
    data.append({'path': path, 'origin': str(b.origin), 'extent': str(b.box_extent), 'materials':[str(x.material_interface.get_path_name()) for x in mesh.static_materials]})
(root/'Saved/Setup/construction_assets.json').write_text(json.dumps(data, indent=2))
