import unreal
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
if not (root/'Foundryline.uproject').is_file(): raise RuntimeError('Foundryline only')
folder='/Game/Foundryline/Buildings/Lighting';name='M_CeilingGlow'
mat=unreal.load_asset(folder+'/'+name)
if not mat: mat=unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,folder,unreal.Material,unreal.MaterialFactoryNew())
edit=unreal.MaterialEditingLibrary;edit.delete_all_material_expressions(mat)
mat.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
node=edit.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector,-200,0)
node.set_editor_property('constant',unreal.LinearColor(6000,5700,5200,1))
edit.connect_material_property(node,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
edit.set_material_usage(mat,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES);edit.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat)
roof_folder='/Game/Foundryline/Buildings/Roofs';roof_name='M_RoofFinish'
roof=unreal.load_asset(roof_folder+'/'+roof_name)
if not roof: roof=unreal.AssetToolsHelpers.get_asset_tools().create_asset(roof_name,roof_folder,unreal.Material,unreal.MaterialFactoryNew())
edit.delete_all_material_expressions(roof)
def tint(name,color,y):
 n=edit.create_material_expression(roof,unreal.MaterialExpressionVectorParameter,-400,y);n.set_editor_property('parameter_name',name);n.set_editor_property('default_value',color);return n
inside=tint('InteriorTint',unreal.LinearColor(.9,.9,.87,1),0)
outside=tint('ExteriorTint',unreal.LinearColor(.03,.04,.05,1),100)
normal=edit.create_material_expression(roof,unreal.MaterialExpressionPixelNormalWS,-600,250)
mask=edit.create_material_expression(roof,unreal.MaterialExpressionComponentMask,-450,250)
for key,value in [('r',False),('g',False),('b',True),('a',False)]: mask.set_editor_property(key,value)
edit.connect_material_expressions(normal,'',mask,'')
# Saturated world normal Z selects light undersides and dark exterior tops/sides.
alpha=edit.create_material_expression(roof,unreal.MaterialExpressionSaturate,-250,250);edit.connect_material_expressions(mask,'',alpha,'')
blend=edit.create_material_expression(roof,unreal.MaterialExpressionLinearInterpolate,0,50)
edit.connect_material_expressions(inside,'',blend,'A');edit.connect_material_expressions(outside,'',blend,'B');edit.connect_material_expressions(alpha,'',blend,'Alpha');edit.connect_material_property(blend,'',unreal.MaterialProperty.MP_BASE_COLOR)
edit.set_material_usage(roof,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES);edit.recompile_material(roof);unreal.EditorAssetLibrary.save_loaded_asset(roof)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
levels.load_level('/Game/Foundryline/Maps/L_ConstructionSite')
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
 if actor.get_actor_label()=='Exposure_Daylight':
  s=actor.get_editor_property('settings')
  settings={'auto_exposure_method':unreal.AutoExposureMethod.AEM_HISTOGRAM,'auto_exposure_min_brightness':3.0,'auto_exposure_max_brightness':16.0,'auto_exposure_low_percent':30.0,'auto_exposure_high_percent':80.0,'auto_exposure_speed_up':3.0,'auto_exposure_speed_down':2.0,'auto_exposure_bias':0.0}
  for key,value in settings.items():
   s.set_editor_property('override_'+key,True);s.set_editor_property(key,value)
  actor.set_editor_property('settings',s)
if not levels.save_current_level(): raise RuntimeError('Exposure save failed')
unreal.log('FOUNDRYLINE_INTERIOR_EXPOSURE_AND_LAMP_GLOW_SAVED')
