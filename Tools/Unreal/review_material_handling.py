"""Repair the existing roof input, then open a local material-handling review."""
import unreal, time
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
if not (root/'Foundryline.uproject').is_file():raise RuntimeError('Foundryline only')
roof=unreal.load_asset('/Game/Foundryline/Buildings/Roofs/M_RoofFinish')
edit=unreal.MaterialEditingLibrary
# The roof graph is small; rebuilding it also avoids depending on Python graph enumeration.
edit.delete_all_material_expressions(roof)
def tint(name,color,y):
 n=edit.create_material_expression(roof,unreal.MaterialExpressionVectorParameter,-400,y);n.set_editor_property('parameter_name',name);n.set_editor_property('default_value',unreal.LinearColor(*color));return n
inside=tint('InteriorTint',(.9,.9,.87,1),0);outside=tint('ExteriorTint',(.03,.04,.05,1),100)
normal=edit.create_material_expression(roof,unreal.MaterialExpressionPixelNormalWS,-600,250)
mask=edit.create_material_expression(roof,unreal.MaterialExpressionComponentMask,-450,250)
for key,value in [('r',False),('g',False),('b',True),('a',False)]:mask.set_editor_property(key,value)
assert edit.connect_material_expressions(normal,'',mask,'')
alpha=edit.create_material_expression(roof,unreal.MaterialExpressionSaturate,-250,250)
assert edit.connect_material_expressions(mask,'',alpha,'')
blend=edit.create_material_expression(roof,unreal.MaterialExpressionLinearInterpolate,0,50)
assert edit.connect_material_expressions(inside,'',blend,'A')
assert edit.connect_material_expressions(outside,'',blend,'B')
assert edit.connect_material_expressions(alpha,'',blend,'Alpha')
edit.connect_material_property(blend,'',unreal.MaterialProperty.MP_BASE_COLOR)
edit.recompile_material(roof);unreal.EditorAssetLibrary.save_loaded_asset(roof)
for asset in unreal.EditorAssetLibrary.list_assets('/Game/Foundryline/Shared/Materials/MaterialHandling',False):
 material=unreal.load_asset(asset)
 if isinstance(material,unreal.Material):
  edit.set_material_usage(material,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
  edit.recompile_material(material);unreal.EditorAssetLibrary.save_loaded_asset(material)
start=time.monotonic()
def tick(dt):
 if time.monotonic()-start>3:
  unreal.unregister_slate_post_tick_callback(handle)
  unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
handle=unreal.register_slate_post_tick_callback(tick)
