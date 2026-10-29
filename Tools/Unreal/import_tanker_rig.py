import unreal,json
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve();src=root/'SourceAssets/AssemblyReviews/TankerTractor';data=json.loads((src/'parts.json').read_text());tools=unreal.AssetToolsHelpers.get_asset_tools()
detail_materials={}
for name,color,rough,metal in [('DetailRubber',(.025,.03,.035),.9,0),('DetailMetal',(.34,.39,.44),.3,.7),('DetailRed',(.65,.02,.012),.35,0),('DetailWhite',(.85,.88,.84),.4,0)]:
 dest='/Game/Foundryline/Vehicles/Shared/Materials';m=unreal.load_asset(dest+'/M_'+name) or tools.create_asset('M_'+name,dest,unreal.Material,unreal.MaterialFactoryNew())
 unreal.MaterialEditingLibrary.delete_all_material_expressions(m)
 c=unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionConstant3Vector,0,0);c.constant=unreal.LinearColor(*color,1);unreal.MaterialEditingLibrary.connect_material_property(c,'',unreal.MaterialProperty.MP_BASE_COLOR)
 for prop,value in [(unreal.MaterialProperty.MP_ROUGHNESS,rough),(unreal.MaterialProperty.MP_METALLIC,metal)]:
  n=unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionConstant,0,180);n.set_editor_property('r',value);unreal.MaterialEditingLibrary.connect_material_property(n,'',prop)
 unreal.MaterialEditingLibrary.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m);detail_materials[name.lower()]=m
material=unreal.load_asset('/Game/Foundryline/Vehicles/MegapolyTrucks/Materials/M_MegapolyMain');report=[]
for p in data['parts']:
 o=unreal.FbxImportUI();o.import_materials=False;o.import_textures=False;o.import_as_skeletal=False;o.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH;o.automated_import_should_detect_type=False
 d=o.static_mesh_import_data;d.combine_meshes=True;d.convert_scene=True;d.convert_scene_unit=True;d.auto_generate_collision=False;d.generate_lightmap_u_vs=True
 t=unreal.AssetImportTask();t.filename=str(root/'SourceAssets'/p['source_file']);t.destination_path=p['asset_path'].rsplit('/',1)[0];t.destination_name='SM_'+p['name'];t.automated=True;t.replace_existing=True;t.save=True;t.options=o;t.factory=unreal.FbxFactory();tools.import_asset_tasks([t])
 mesh=unreal.load_asset(t.destination_path+'/'+t.destination_name)
 if not mesh:raise RuntimeError('Missing rig part '+p['name'])
 for i,slot in enumerate(mesh.get_editor_property('static_materials')):mesh.set_material(i,next((m for n,m in detail_materials.items() if n in str(slot.material_slot_name).lower()),material))
 build=unreal.EditorStaticMeshLibrary.get_lod_build_settings(mesh,0);build.recompute_tangents=True;build.use_mikk_t_space=False;unreal.EditorStaticMeshLibrary.set_lod_build_settings(mesh,0,build);unreal.EditorAssetLibrary.save_loaded_asset(mesh)
 bounds=mesh.get_bounds();size=bounds.box_extent*2;report.append({'name':p['name'],'size_cm':[size.x,size.y,size.z],'center_cm':[bounds.origin.x,bounds.origin.y,bounds.origin.z]})
(root/'Saved/Logs/TankerRigImport.json').write_text(json.dumps(report,indent=2))
