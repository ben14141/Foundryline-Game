import unreal,json
from pathlib import Path
names=['SM_Window_Roof_400','SM_Roof_600','SM_Roof_400','SM_Roof_200']
rows=[]
for n in names:
 m=unreal.load_asset('/Game/Factory/Meshes/Roof/'+n)
 b=m.get_bounding_box()
 rows.append({'name':n,'min':str(b.min),'max':str(b.max),'materials':[{'slot':str(x.material_slot_name),'path':str(x.material_interface.get_path_name()) if x.material_interface else ''} for x in m.static_materials]})
Path(unreal.Paths.project_saved_dir()+'Logs/RoofAssetBounds.json').write_text(json.dumps(rows,indent=2))

folder='/Game/Foundryline/Construction/Materials'
path=folder+'/M_WindowGlass'
mat=unreal.load_asset(path)
if not mat: mat=unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_WindowGlass',folder,unreal.Material,unreal.MaterialFactoryNew())
edit=unreal.MaterialEditingLibrary
edit.delete_all_material_expressions(mat)
mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
mat.set_editor_property('two_sided',True)
mat.set_editor_property('translucency_lighting_mode',unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
color=edit.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector,-200,0)
color.set_editor_property('constant',unreal.LinearColor(.65,.8,.85,1))
edit.connect_material_property(color,'',unreal.MaterialProperty.MP_BASE_COLOR)
for prop,value in [(unreal.MaterialProperty.MP_OPACITY,.16),(unreal.MaterialProperty.MP_ROUGHNESS,.12)]:
 node=edit.create_material_expression(mat,unreal.MaterialExpressionConstant,-200,200)
 node.set_editor_property('r',value)
 edit.connect_material_property(node,'',prop)
edit.set_material_usage(mat,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
edit.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat)
