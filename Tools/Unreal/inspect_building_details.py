import unreal,json
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
paths=list((root/'Content/Factory/Meshes/Wall_Outer(Door)').glob('*Door*.uasset'))+list((root/'Content/Factory/Meshes/Wall_Outer(Door)').glob('*Cap*.uasset'))+list((root/'Content/Factory/Meshes/Props').glob('*Beam*.uasset'))
data=[]
for file in paths:
    path='/Game/'+file.relative_to(root/'Content').with_suffix('').as_posix()
    mesh=unreal.load_asset(path)
    if not isinstance(mesh,unreal.StaticMesh):continue
    b=mesh.get_bounds()
    data.append(dict(path=path,origin=[b.origin.x,b.origin.y,b.origin.z],size=[b.box_extent.x*2,b.box_extent.y*2,b.box_extent.z*2]))
(root/'Saved/Setup/building_detail_assets.json').write_text(json.dumps(data,indent=2))
