import unreal,json
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
mat=unreal.load_asset('/Game/Foundryline/World/Materials/M_FlatForestGround')
if mat:
    unreal.MaterialEditingLibrary.set_material_usage(mat,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
paths=['/Game/Factory/Meshes/Tree/SM_Grass_01','/Game/Factory/Meshes/Tree/SM_Grass_02','/Game/StyleHex_Studio/Free_Packs/FREE_Stylized_Forest_Sample/Meshes/Foliage/SM_Fol_Grass_Clump_1']
data=[]
for path in paths:
    mesh=unreal.load_asset(path)
    if mesh:
        b=mesh.get_bounds()
        data.append(dict(path=path,origin=[b.origin.x,b.origin.y,b.origin.z],size=[b.box_extent.x*2,b.box_extent.y*2,b.box_extent.z*2]))
(root/'Saved/Setup/parking_green_assets.json').write_text(json.dumps(data,indent=2))
