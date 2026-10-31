"""Original per-instance cooling spark material; no vendor textures or geometry."""
import unreal
edit=unreal.MaterialEditingLibrary
dest='/Game/Foundryline/Production/Equipment/Welder/VFX'
m=unreal.load_asset(dest+'/M_WeldingSpark') or unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_WeldingSpark',dest,unreal.Material,unreal.MaterialFactoryNew())
edit.delete_all_material_expressions(m)
m.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
channels=[]
for index,default in enumerate([1,.4,.025,18000]):
 n=edit.create_material_expression(m,unreal.MaterialExpressionPerInstanceCustomData,-700,index*100)
 n.set_editor_property('data_index',index);n.set_editor_property('const_default_value',default);channels.append(n)
a=edit.create_material_expression(m,unreal.MaterialExpressionAppendVector,-450,0)
b=edit.create_material_expression(m,unreal.MaterialExpressionAppendVector,-250,0)
assert edit.connect_material_expressions(channels[0],'',a,'A')
assert edit.connect_material_expressions(channels[1],'',a,'B')
assert edit.connect_material_expressions(a,'',b,'A')
assert edit.connect_material_expressions(channels[2],'',b,'B')
mult=edit.create_material_expression(m,unreal.MaterialExpressionMultiply,-50,0)
assert edit.connect_material_expressions(b,'',mult,'A')
assert edit.connect_material_expressions(channels[3],'',mult,'B')
interp=edit.create_material_expression(m,unreal.MaterialExpressionVertexInterpolator,150,0)
assert edit.connect_material_expressions(mult,'',interp,'VS')
assert edit.connect_material_property(interp,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
edit.set_material_usage(m,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
edit.recompile_material(m);assert unreal.EditorAssetLibrary.save_loaded_asset(m)
unreal.log('WELDING_SPARK_MATERIAL_READY')
