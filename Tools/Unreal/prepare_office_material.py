import unreal
path='/Game/Foundryline/Construction/Materials'
asset=unreal.load_asset(path+'/M_OfficeFloor')
if not asset:
    asset=unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_OfficeFloor',path,unreal.Material,unreal.MaterialFactoryNew())
    color=unreal.MaterialEditingLibrary.create_material_expression(asset,unreal.MaterialExpressionConstant3Vector,-300,0)
    color.set_editor_property('constant',unreal.LinearColor(0.42,0.39,0.33,1))
    unreal.MaterialEditingLibrary.connect_material_property(color,'',unreal.MaterialProperty.MP_BASE_COLOR)
    rough=unreal.MaterialEditingLibrary.create_material_expression(asset,unreal.MaterialExpressionConstant,-300,200)
    rough.set_editor_property('r',0.85)
    unreal.MaterialEditingLibrary.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
asset.set_editor_property('used_with_instanced_static_meshes',True)
unreal.MaterialEditingLibrary.recompile_material(asset)
unreal.EditorAssetLibrary.save_loaded_asset(asset)
