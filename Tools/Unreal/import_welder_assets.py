"""Import only the game-owned MIG welder; vendor assets and maps are untouched."""
import unreal,json
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
src=root/'SourceAssets/Production/Equipment/Welder';data=json.loads((src/'manifest.json').read_text())
dest='/Game/Foundryline/Production/Equipment/Welder';shared='/Game/Foundryline/Shared/Materials/MaterialHandling'
tools=unreal.AssetToolsHelpers.get_asset_tools();edit=unreal.MaterialEditingLibrary
materials={}
for name in ['FoundryOrange','Graphite','MachinedSteel','DarkSteel','Rubber','White','Red','SafetyYellow','Glass']:
 m=unreal.load_asset(shared+'/M_MH_'+name)
 if not m:raise RuntimeError('Missing shared material '+name)
 materials['mh_'+name.lower()]=m
copper=unreal.load_asset(dest+'/Materials/M_MH_Copper') or tools.create_asset('M_MH_Copper',dest+'/Materials',unreal.Material,unreal.MaterialFactoryNew())
edit.delete_all_material_expressions(copper)
n=edit.create_material_expression(copper,unreal.MaterialExpressionConstant3Vector);n.constant=unreal.LinearColor(.48,.22,.085,1);edit.connect_material_property(n,'',unreal.MaterialProperty.MP_BASE_COLOR)
for prop,value in [(unreal.MaterialProperty.MP_METALLIC,.82),(unreal.MaterialProperty.MP_ROUGHNESS,.3)]:
 n=edit.create_material_expression(copper,unreal.MaterialExpressionConstant);n.set_editor_property('r',value);edit.connect_material_property(n,'',prop)
edit.set_material_usage(copper,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES);edit.recompile_material(copper);unreal.EditorAssetLibrary.save_loaded_asset(copper);materials['mh_copper']=copper
report={'assets':[],'errors':[],'gameplay':'Model only; no welding production logic.'}
for a in data['assets']:
 if '-GunOnly' in unreal.SystemLibrary.get_command_line() and a['name'] not in ['Welder_MIGGun','Welder_Complete']:continue
 o=unreal.FbxImportUI();o.import_materials=False;o.import_textures=False;o.import_as_skeletal=False;o.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH;o.automated_import_should_detect_type=False
 d=o.static_mesh_import_data;d.combine_meshes=True;d.convert_scene=True;d.convert_scene_unit=True;d.auto_generate_collision=a['name'] in ['Welder_Machine','Welder_Complete'];d.generate_lightmap_u_vs=True
 t=unreal.AssetImportTask();t.filename=str(src/(a['name']+'.fbx'));t.destination_path=dest;t.destination_name='SM_'+a['name'];t.automated=True;t.replace_existing=True;t.save=True;t.options=o;t.factory=unreal.FbxFactory();tools.import_asset_tasks([t])
 mesh=unreal.load_asset(dest+'/SM_'+a['name'])
 if not mesh:raise RuntimeError('Missing mesh '+a['name'])
 for i,slot in enumerate(mesh.get_editor_property('static_materials')):
  key=next((key for key in materials if key in str(slot.material_slot_name).lower()),None)
  if not key:raise RuntimeError('Unmatched material '+str(slot.material_slot_name))
  mesh.set_material(i,materials[key])
 settings=unreal.EditorStaticMeshLibrary.get_lod_build_settings(mesh,0);settings.recompute_tangents=True;settings.use_mikk_t_space=False;unreal.EditorStaticMeshLibrary.set_lod_build_settings(mesh,0,settings);unreal.EditorAssetLibrary.save_loaded_asset(mesh)
 size=mesh.get_bounds().box_extent*2
 actual=[size.x,size.y,size.z];expected=a['dimensions_cm']
 if any(abs(x-y)>.3 for x,y in zip(actual,expected)):raise RuntimeError('Scale mismatch '+a['name']+str(actual)+str(expected))
 report['assets'].append({'path':mesh.get_path_name(),'dimensions_cm':actual,'assembly_position_cm':a['position_cm']})
(root/'Saved/Logs/WelderImport.json').write_text(json.dumps(report,indent=2));unreal.log('WELDER_IMPORT_COMPLETE '+str(len(report['assets'])))
