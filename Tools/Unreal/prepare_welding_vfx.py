import unreal,json
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
if not (root/'Foundryline.uproject').is_file():raise RuntimeError('Foundryline only')
dest='/Game/Foundryline/Production/Equipment/Welder/VFX';edit=unreal.MaterialEditingLibrary
m=unreal.load_asset(dest+'/M_WeldingGlow') or unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_WeldingGlow',dest,unreal.Material,unreal.MaterialFactoryNew())
edit.delete_all_material_expressions(m);m.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
color=edit.create_material_expression(m,unreal.MaterialExpressionVectorParameter,-300,0);color.set_editor_property('parameter_name','GlowColor');color.set_editor_property('default_value',unreal.LinearColor(.18,.48,1,1))
strength=edit.create_material_expression(m,unreal.MaterialExpressionScalarParameter,-300,100);strength.set_editor_property('parameter_name','Strength');strength.set_editor_property('default_value',80)
multiply=edit.create_material_expression(m,unreal.MaterialExpressionMultiply,0,0)
assert edit.connect_material_expressions(color,'',multiply,'A')
assert edit.connect_material_expressions(strength,'',multiply,'B')
edit.connect_material_property(multiply,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
edit.set_material_usage(m,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES);edit.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m)
gun=unreal.load_asset('/Game/Foundryline/Production/Equipment/Welder/SM_Welder_MIGGun')
socket=gun.find_socket('WeldTip')
if not socket:
 socket=unreal.new_object(unreal.StaticMeshSocket,outer=gun);socket.set_editor_property('socket_name','WeldTip');gun.add_socket(socket)
socket.set_editor_property('relative_location',unreal.Vector(23.9,0,13.5))
assert unreal.EditorAssetLibrary.save_loaded_asset(gun,False), 'Gun socket save failed'
(root/'Saved/Logs/WeldingVFXAssets.json').write_text(json.dumps({'material':m.get_path_name(),'gun_tip_cm':[23.9,0,13.5],'socket':str(socket.socket_name)},indent=2))
unreal.log('WELDING_VFX_ASSETS_READY')
