"""Game-owned construction surfaces. Vendor materials are never modified."""
from pathlib import Path
import unreal
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
if not (root/'Foundryline.uproject').is_file():
    raise RuntimeError('Foundryline only')
folder='/Game/Foundryline/Construction/Materials'
palette={'Asphalt':(0.045,0.052,0.06),'Concrete':(0.34,0.36,0.37),
         'Wall':(0.64,0.66,0.63),'White':(0.85,0.87,0.83),'Yellow':(0.95,0.55,0.025),
         'Metal':(0.085,0.12,0.15),'Door':(0.045,0.23,0.35),'Glass':(0.055,0.20,0.28),
         'Preview':(0.03,0.45,1)}
edit=unreal.MaterialEditingLibrary
for name,color in palette.items():
    path=folder+'/M_'+name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        existing=unreal.load_asset(path)
        edit.set_material_usage(existing,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
        edit.recompile_material(existing)
        unreal.EditorAssetLibrary.save_loaded_asset(existing)
        continue
    mat=unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_'+name,folder,unreal.Material,unreal.MaterialFactoryNew())
    if not mat:
        raise RuntimeError('Material creation failed: '+name)
    edit.set_material_usage(mat,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
    tint=edit.create_material_expression(mat,unreal.MaterialExpressionVectorParameter,0,0)
    tint.set_editor_property('parameter_name','Tint')
    tint.set_editor_property('default_value',unreal.LinearColor(*color,1))
    if name=='Preview':
        mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
        mat.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
        mat.set_editor_property('two_sided',True)
        strength=edit.create_material_expression(mat,unreal.MaterialExpressionMultiply,200,0)
        strength.set_editor_property('const_b',3500.0)
        edit.connect_material_expressions(tint,'',strength,'A')
        edit.connect_material_property(strength,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        alpha=edit.create_material_expression(mat,unreal.MaterialExpressionConstant,0,200)
        alpha.set_editor_property('r',0.24)
        edit.connect_material_property(alpha,'',unreal.MaterialProperty.MP_OPACITY)
    else:
        edit.connect_material_property(tint,'',unreal.MaterialProperty.MP_BASE_COLOR)
        rough=edit.create_material_expression(mat,unreal.MaterialExpressionConstant,0,200)
        rough.set_editor_property('r',0.18 if name=='Glass' else 0.45 if name in ('Metal','Door') else 0.9)
        edit.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
        if name in ('Metal','Glass'):
            metal=edit.create_material_expression(mat,unreal.MaterialExpressionConstant,0,350)
            metal.set_editor_property('r',0.55 if name=='Metal' else 0.35)
            edit.connect_material_property(metal,'',unreal.MaterialProperty.MP_METALLIC)
    edit.recompile_material(mat)
    if not unreal.EditorAssetLibrary.save_loaded_asset(mat):
        raise RuntimeError('Material save failed')
unreal.log('FOUNDRYLINE_KIT_MATERIALS_READY')
