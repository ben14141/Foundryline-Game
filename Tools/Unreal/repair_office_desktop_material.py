"""Repair only desktop display material graph; execute in editor with PIE stopped."""
import unreal,json
from pathlib import Path
e=unreal.MaterialEditingLibrary
m=unreal.load_asset('/Game/Foundryline/UI/Operations/M_OfficeDesktopScreen')
if not m:raise RuntimeError('Missing desktop material')
old=e.get_material_property_input_node(m,unreal.MaterialProperty.MP_EMISSIVE_COLOR)
q={'before_emissive':old.get_class().get_name() if old else None,'before_parameters':[str(n) for n in e.get_texture_parameter_names(m)]}
e.delete_all_material_expressions(m)
m.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
m.set_editor_property('two_sided',False)
t=e.create_material_expression(m,unreal.MaterialExpressionTextureSampleParameter2D,-300,0)
t.set_editor_property('parameter_name','DesktopTexture')
t.set_editor_property('texture',unreal.load_asset('/Game/Foundryline/UI/Brand/T_FoundrylineLogo'))
# Direct emissive avoids an unnecessary intermediate pin and exactly matches the proven logo material.
q['connected']=e.connect_material_property(t,'RGB',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
q['base_connected']=e.connect_material_property(t,'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
e.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m)
node=e.get_material_property_input_node(m,unreal.MaterialProperty.MP_EMISSIVE_COLOR)
q['after_emissive']=node.get_class().get_name() if node else None;q['after_parameters']=[str(n) for n in e.get_texture_parameter_names(m)]
(Path(unreal.Paths.project_dir())/'Saved/Logs/OfficeDesktopMaterialRepair.json').write_text(json.dumps(q,indent=2))
assert q['connected'] and node is not None and 'DesktopTexture' in q['after_parameters'],q
unreal.log('OFFICE_DESKTOP_MATERIAL_REPAIRED '+str(q))
