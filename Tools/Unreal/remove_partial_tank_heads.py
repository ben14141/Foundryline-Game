"""Remove superseded standalone partial-head assets after checking references."""
import unreal
paths=['/Game/Foundryline/Products/Components/DishedHeads/SM_TankHead_ASME_FD_Half',
       '/Game/Foundryline/Products/Components/Fabrication/SM_EndCapHalf']
for path in paths:
 if not unreal.EditorAssetLibrary.does_asset_exist(path):continue
 references=unreal.EditorAssetLibrary.find_package_referencers_for_asset(path,True)
 assert not references,(path,references)
 assert unreal.EditorAssetLibrary.delete_asset(path),path
for path in paths:assert not unreal.EditorAssetLibrary.does_asset_exist(path),path
unreal.log('WHOLE_HEADS_ONLY_VERIFIED')
