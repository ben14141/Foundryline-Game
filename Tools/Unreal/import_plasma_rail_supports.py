"""Focused reimport of authored plasma rail supports. Does not load or save a map.
Only SM_PlasmaTable_Body and SM_PlasmaTable_Complete are replaced. Moving components,
CNC materials, stock sheet, vendor content and every other equipment asset are untouched.
"""
import unreal,json
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
src=root/'SourceAssets/Production/Equipment/PlasmaTable'
manifest=json.loads((src/'manifest.json').read_text())
assert manifest.get('rail_supports',{}).get('revision')==1,'Build the revised rail supports first'
dest='/Game/Foundryline/Production/Equipment/PlasmaTable'
shared='/Game/Foundryline/Shared/Materials/MaterialHandling'
tools=unreal.AssetToolsHelpers.get_asset_tools()
materials={}
for name in ['FoundryOrange','Graphite','MachinedSteel','DarkSteel','Rubber','White','Red','SafetyYellow','Glass','CarbonSteel']:
 m=unreal.load_asset(shared+'/M_MH_'+name)
 if not m:raise RuntimeError('Missing shared material '+name)
 materials['mh_'+name.lower()]=m
materials['pt_cncdisplay']=unreal.load_asset(dest+'/Materials/M_PT_CNCDisplay')
report={'assets':[],'errors':[],'scope':'Body and Complete only; gantry/torch assets unchanged'}
for name in ['PlasmaTable_Body','PlasmaTable_Complete']:
 expected=next(a for a in manifest['assets'] if a['name']==name)
 options=unreal.FbxImportUI();options.import_materials=False;options.import_textures=False;options.import_as_skeletal=False
 options.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH;options.automated_import_should_detect_type=False
 data=options.static_mesh_import_data;data.combine_meshes=True;data.convert_scene=True;data.convert_scene_unit=True;data.auto_generate_collision=True;data.generate_lightmap_u_vs=True
 task=unreal.AssetImportTask();task.filename=str(src/(name+'.fbx'));task.destination_path=dest;task.destination_name='SM_'+name;task.automated=True;task.replace_existing=True;task.save=True;task.options=options;task.factory=unreal.FbxFactory()
 tools.import_asset_tasks([task]);mesh=unreal.load_asset(dest+'/SM_'+name)
 if not mesh:raise RuntimeError('Missing mesh '+name)
 for i,slot in enumerate(mesh.get_editor_property('static_materials')):
  key=next((k for k in materials if k in str(slot.material_slot_name).lower()),None)
  if not key or not materials[key]:raise RuntimeError('Unmatched material '+str(slot.material_slot_name))
  mesh.set_material(i,materials[key])
 settings=unreal.EditorStaticMeshLibrary.get_lod_build_settings(mesh,0);settings.recompute_tangents=True;settings.use_mikk_t_space=False;unreal.EditorStaticMeshLibrary.set_lod_build_settings(mesh,0,settings)
 unreal.EditorAssetLibrary.save_loaded_asset(mesh)
 size=mesh.get_bounds().box_extent*2;actual=[size.x,size.y,size.z]
 if any(abs(a-b)>.3 for a,b in zip(actual,expected['dimensions_cm'])):raise RuntimeError('Scale mismatch '+name)
 report['assets'].append({'path':mesh.get_path_name(),'dimensions_cm':actual,'all_materials_present':all(s.material_interface for s in mesh.get_editor_property('static_materials'))})
(root/'Saved/Logs/PlasmaRailSupportsImport.json').write_text(json.dumps(report,indent=2));unreal.log('PLASMA_RAIL_SUPPORTS_IMPORTED')
