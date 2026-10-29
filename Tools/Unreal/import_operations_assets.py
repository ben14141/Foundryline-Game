import unreal,json
from pathlib import Path
root=Path(unreal.Paths.project_dir());src=root/'SourceAssets/Operations';data=json.loads((src/'manifest.json').read_text());tools=unreal.AssetToolsHelpers.get_asset_tools();report=[]
shared='/Game/Foundryline/Shared/Materials/MaterialHandling/M_MH_';mapping={('ops_'+n.lower()):unreal.load_asset(shared+n) for n in ['Graphite','White','FoundryOrange','MachinedSteel','CarbonSteel','Rubber']}
folder='/Game/Foundryline/UI/Operations';screen=unreal.load_asset(folder+'/M_OperationsScreen')
if not screen:
 screen=tools.create_asset('M_OperationsScreen',folder,unreal.Material,unreal.MaterialFactoryNew());e=unreal.MaterialEditingLibrary;node=e.create_material_expression(screen,unreal.MaterialExpressionTextureSample);node.texture=unreal.load_asset('/Game/Foundryline/UI/Brand/T_FoundrylineLogo');e.connect_material_property(node,'RGB',unreal.MaterialProperty.MP_BASE_COLOR);e.connect_material_property(node,'RGB',unreal.MaterialProperty.MP_EMISSIVE_COLOR);e.recompile_material(screen);unreal.EditorAssetLibrary.save_loaded_asset(screen)
logo=unreal.load_asset('/Game/Foundryline/UI/Brand/T_FoundrylineLogo');logo.set_editor_property('address_x',unreal.TextureAddress.TA_CLAMP);logo.set_editor_property('address_y',unreal.TextureAddress.TA_CLAMP);unreal.EditorAssetLibrary.save_loaded_asset(logo)
mapping['ops_screen']=screen
for a in data['assets']:
 if a['name']=='OfficeWorkstation':continue # Dedicated importer preserves three display slots and desktop material.
 dest='/Game/Foundryline/'+a['domain'];o=unreal.FbxImportUI();o.import_materials=False;o.import_textures=False;o.import_as_skeletal=False;o.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH;o.automated_import_should_detect_type=False;d=o.static_mesh_import_data;d.combine_meshes=True;d.convert_scene=True;d.convert_scene_unit=True;d.generate_lightmap_u_vs=True;d.auto_generate_collision=True
 t=unreal.AssetImportTask();t.filename=str(src/(a['name']+'.fbx'));t.destination_path=dest;t.destination_name='SM_'+a['name'];t.automated=True;t.replace_existing=True;t.save=True;t.options=o;tools.import_asset_tasks([t]);m=unreal.load_asset(dest+'/SM_'+a['name'])
 if not m:raise RuntimeError('Failed import '+a['name'])
 for i,s in enumerate(m.get_editor_property('static_materials')):
  mat=mapping.get(str(s.material_slot_name).lower());
  if mat:m.set_material(i,mat)
 unreal.EditorAssetLibrary.save_loaded_asset(m);b=m.get_bounding_box();report.append({'asset':m.get_path_name(),'min':[b.min.x,b.min.y,b.min.z],'max':[b.max.x,b.max.y,b.max.z]})
(root/'Saved/Logs/OperationsAssetsImport.json').write_text(json.dumps(report,indent=2))

# Keep the full operations workflow consistent with the dedicated workstation workflow.
exec((root/'Tools/Unreal/import_office_workstation.py').read_text())
