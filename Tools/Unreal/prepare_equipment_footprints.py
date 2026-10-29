"""Create grid-aligned placement definitions with separate machine and working bounds."""
import unreal,json,math
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
dest='/Game/Foundryline/Production/Equipment/Placement';tools=unreal.AssetToolsHelpers.get_asset_tools();edit=unreal.MaterialEditingLibrary
for name,color in [('Grid',(.04,.45,1.0)),('Boundary',(1.0,.34,.035))]:
 path=dest+'/M_Equipment'+name;m=unreal.load_asset(path) or tools.create_asset('M_Equipment'+name,dest,unreal.Material,unreal.MaterialFactoryNew());edit.delete_all_material_expressions(m);m.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
 n=edit.create_material_expression(m,unreal.MaterialExpressionConstant3Vector);n.constant=unreal.LinearColor(*color,1);edit.connect_material_property(n,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR);edit.set_material_usage(m,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES);edit.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m)
specs=[('PressBrake',(-200,-300,0),(400,350,400)),('PlasmaTable',(-350,-400,0),(400,400,350)),('SheetRoller',(-150,-350,0),(350,300,250))]
parent=unreal.load_asset('/Game/Foundryline/Production/Equipment/Welder/VFX/M_WeldingGlow')
assert parent,'Missing shared machine glow shader'
for name,color in [('Grid',(.04,.45,1.0)),('Boundary',(1.0,.34,.035))]:
 mi=unreal.load_asset(dest+'/MI_Equipment'+name) or tools.create_asset('MI_Equipment'+name,dest,unreal.MaterialInstanceConstant,unreal.MaterialInstanceConstantFactoryNew())
 edit.set_material_instance_parent(mi,parent);edit.set_material_instance_vector_parameter_value(mi,'GlowColor',unreal.LinearColor(*color,1));edit.set_material_instance_scalar_parameter_value(mi,'Strength',8000.0);unreal.EditorAssetLibrary.save_loaded_asset(mi)
rows=[]
for name,lo,hi in specs:
 mesh=unreal.load_asset('/Game/Foundryline/Production/Equipment/'+name+'/SM_'+name+'_Complete')
 assert mesh,'Missing complete preview mesh '+name
 b=mesh.get_bounds();p=b.origin;e=b.box_extent
 minimum=[math.floor(v/25)*25 for v in [p.x-e.x,p.y-e.y,0]]
 maximum=[math.ceil(v/25)*25 for v in [p.x+e.x,p.y+e.y,p.z+e.z]]
 assert all(a<=b for a,b in zip(lo,minimum)) and all(a>=b for a,b in zip(hi,maximum)),(name,minimum,maximum,lo,hi)
 factory=unreal.DataAssetFactory();factory.set_editor_property('data_asset_class',unreal.FoundrylinePlaceableDefinition)
 asset=unreal.load_asset(dest+'/DA_'+name) or tools.create_asset('DA_'+name,dest,unreal.FoundrylinePlaceableDefinition,factory)
 asset.set_editor_property('definition_id','Equipment_'+name);asset.set_editor_property('function_id',name);asset.set_editor_property('mesh',mesh);asset.set_editor_property('snap',unreal.FoundrylineSnap.FINE25CM)
 solid=unreal.FoundrylineGridBounds();solid.min=unreal.IntVector(*minimum);solid.max=unreal.IntVector(*maximum)
 work=unreal.FoundrylineGridBounds();work.min=unreal.IntVector(*lo);work.max=unreal.IntVector(*hi)
 asset.set_editor_property('footprint',solid);asset.set_editor_property('working_clearance',work);asset.set_editor_property('allowed_surfaces',['ShopFloor','StarterLand']);unreal.EditorAssetLibrary.save_loaded_asset(asset)
 rows.append({'equipment':name,'asset':asset.get_path_name(),'solid_min_cm':minimum,'solid_max_cm':maximum,'work_min_cm':lo,'work_max_cm':hi,'working_size_m':[(hi[i]-lo[i])/100 for i in [0,1]],'snap_cm':25})
(root/'Saved/Logs/EquipmentFootprints.json').write_text(json.dumps(rows,indent=2));unreal.log('EQUIPMENT_FOOTPRINTS_READY')
