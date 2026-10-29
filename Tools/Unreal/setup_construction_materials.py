import unreal
from pathlib import Path
root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
if not (root/'Foundryline.uproject').is_file():
    raise RuntimeError('Foundryline only')
folder = '/Game/Foundryline/Construction/Materials'
tools = unreal.AssetToolsHelpers.get_asset_tools()
edit = unreal.MaterialEditingLibrary
for name, color, ghost in [('M_RoadTest', (0.055,0.065,0.075), False), ('M_ConcreteTest', (0.32,0.35,0.36), False), ('M_BuildPreview', (0.05,0.6,1), True)]:
    path = folder+'/'+name
    if unreal.EditorAssetLibrary.does_asset_exist(path) and not ghost:
        continue
    mat = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else tools.create_asset(name,folder,unreal.Material,unreal.MaterialFactoryNew())
    edit.delete_all_material_expressions(mat)
    tint = edit.create_material_expression(mat,unreal.MaterialExpressionVectorParameter,0,0)
    tint.set_editor_property('parameter_name','Tint')
    tint.set_editor_property('default_value',unreal.LinearColor(*color,1))
    if ghost:
        intensity=edit.create_material_expression(mat,unreal.MaterialExpressionMultiply,200,0)
        intensity.set_editor_property('const_b',5000.0)
        edit.connect_material_expressions(tint,'',intensity,'A')
        edit.connect_material_property(intensity,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    else:
        edit.connect_material_property(tint,'',unreal.MaterialProperty.MP_BASE_COLOR)
    if ghost:
        mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
        mat.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
        opacity=edit.create_material_expression(mat,unreal.MaterialExpressionConstant,0,200)
        opacity.set_editor_property('r',0.38)
        edit.connect_material_property(opacity,'',unreal.MaterialProperty.MP_OPACITY)
    else:
        rough=edit.create_material_expression(mat,unreal.MaterialExpressionConstant,0,200)
        rough.set_editor_property('r',0.9)
        edit.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
    edit.recompile_material(mat)
    if not unreal.EditorAssetLibrary.save_loaded_asset(mat):
        raise RuntimeError('Material save failed: '+name)
unreal.log('FOUNDRYLINE_CONSTRUCTION_MATERIALS_READY')
