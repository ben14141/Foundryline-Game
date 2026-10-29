"""Import only the game-owned plasma table; vendor assets and maps are untouched."""
import unreal,json
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
src=root/'SourceAssets/Production/Equipment/PlasmaTable';data=json.loads((src/'manifest.json').read_text())
dest='/Game/Foundryline/Production/Equipment/PlasmaTable';shared='/Game/Foundryline/Shared/Materials/MaterialHandling'
tools=unreal.AssetToolsHelpers.get_asset_tools();edit=unreal.MaterialEditingLibrary
materials={}
for name in ['FoundryOrange','Graphite','MachinedSteel','DarkSteel','Rubber','White','Red','SafetyYellow','Glass','CarbonSteel']:
 m=unreal.load_asset(shared+'/M_MH_'+name)
 if not m:raise RuntimeError('Missing shared material '+name)
 materials['mh_'+name.lower()]=m
screenfolder=dest+'/Materials'
t=unreal.AssetImportTask();t.filename=str(src/'T_PlasmaTable_CNC.png');t.destination_path=screenfolder;t.destination_name='T_PlasmaTable_CNC';t.automated=True;t.replace_existing=True;t.save=True
tools.import_asset_tasks([t]);texture=unreal.load_asset(screenfolder+'/T_PlasmaTable_CNC')
if not texture:raise RuntimeError('Missing CNC display texture')
screenmat=unreal.load_asset(screenfolder+'/M_PT_CNCDisplay') or tools.create_asset('M_PT_CNCDisplay',screenfolder,unreal.Material,unreal.MaterialFactoryNew())
edit.delete_all_material_expressions(screenmat)
sample=edit.create_material_expression(screenmat,unreal.MaterialExpressionTextureSample);sample.texture=texture
edit.connect_material_property(sample,'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
edit.connect_material_property(sample,'RGB',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
rough=edit.create_material_expression(screenmat,unreal.MaterialExpressionConstant);rough.set_editor_property('r',.48);edit.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
edit.set_material_usage(screenmat,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES);edit.recompile_material(screenmat);unreal.EditorAssetLibrary.save_loaded_asset(screenmat)
materials['pt_cncdisplay']=screenmat
report={'assets':[],'errors':[],'gameplay':'Model and Blender review animation only; no production gameplay.'}
for a in data['assets']:
 dest='/Game/Foundryline/'+a['domain']
 o=unreal.FbxImportUI();o.import_materials=False;o.import_textures=False;o.import_as_skeletal=False;o.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH;o.automated_import_should_detect_type=False
 d=o.static_mesh_import_data;d.combine_meshes=True;d.convert_scene=True;d.convert_scene_unit=True;d.auto_generate_collision=a['name'] in ['PlasmaTable_Body','PlasmaTable_Complete','PlasmaTable_OperatorConsole','PlasmaTable_PowerUnit','PlasmaTable_Extraction'];d.generate_lightmap_u_vs=True
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
 p=a['position_cm'];report['assets'].append({'path':mesh.get_path_name(),'dimensions_cm':actual,'assembly_position_cm':[p[0],-p[1],p[2]]})
(root/'Saved/Logs/PlasmaTableImport.json').write_text(json.dumps(report,indent=2));unreal.log('PLASMA_TABLE_IMPORT_COMPLETE '+str(len(report['assets'])))
