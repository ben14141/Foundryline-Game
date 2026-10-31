import unreal
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve()
assert (root/'Foundryline.uproject').is_file()
E=unreal.MaterialEditingLibrary;A=unreal.AssetToolsHelpers.get_asset_tools();folder='/Game/Foundryline/Construction/Materials'
for name,color,rough,metal in [('M_OfficeStairPaint',(.79,.78,.73),.62,0),('M_OfficeStairRail',(.025,.032,.038),.32,.35),('M_OfficeStairOak',(.42,.23,.105),.5,0),('M_OfficeStairGlass',(.75,.87,.93),.12,0)]:
 path=folder+'/'+name
 if unreal.EditorAssetLibrary.does_asset_exist(path):continue
 m=A.create_asset(name,folder,unreal.Material,unreal.MaterialFactoryNew());m.set_editor_property('two_sided',True)
 c=E.create_material_expression(m,unreal.MaterialExpressionConstant3Vector,0,0);c.set_editor_property('constant',unreal.LinearColor(*color,1));E.connect_material_property(c,'',unreal.MaterialProperty.MP_BASE_COLOR)
 if name.endswith('Oak'):
  pos=E.create_material_expression(m,unreal.MaterialExpressionWorldPosition,-600,0);grain=E.create_material_expression(m,unreal.MaterialExpressionCustom,-300,0);inp=unreal.CustomInput();inp.set_editor_property('input_name','P');grain.set_editor_property('inputs',[inp]);grain.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT3);grain.set_editor_property('code','float g=sin(P.x*1.1+sin(P.y*.027)*1.7);float f=sin(P.x*9+sin(P.y*.04));return lerp(float3(.24,.11,.045),float3(.49,.29,.14),.55+g*.12+f*.025);');E.connect_material_expressions(pos,'',grain,'P');E.connect_material_property(grain,'',unreal.MaterialProperty.MP_BASE_COLOR)
 for value,prop in [(rough,unreal.MaterialProperty.MP_ROUGHNESS),(metal,unreal.MaterialProperty.MP_METALLIC)]:
  n=E.create_material_expression(m,unreal.MaterialExpressionConstant,0,200);n.set_editor_property('r',value);E.connect_material_property(n,'',prop)
 if name.endswith('Glass'):
  m.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT);n=E.create_material_expression(m,unreal.MaterialExpressionConstant,0,400);n.set_editor_property('r',.12);E.connect_material_property(n,'',unreal.MaterialProperty.MP_OPACITY)
 E.recompile_material(m)
 if not unreal.EditorAssetLibrary.save_loaded_asset(m):raise RuntimeError('Could not save '+name)
unreal.log('OFFICE_STAIR_MATERIALS_READY')
