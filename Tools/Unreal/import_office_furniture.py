"""Import original decor and the private, licensed office-clothing palette. No maps changed."""
import unreal,json
from pathlib import Path
R=Path(unreal.Paths.project_dir()).resolve();T=unreal.AssetToolsHelpers.get_asset_tools();E=unreal.MaterialEditingLibrary;folder='/Game/Foundryline/Office/Decor';src=R/'SourceAssets/Office/Decor';report={'meshes':[]}
specs={'Walnut':((.19,.085,.035),0,.38),'Oak':((.45,.25,.105),0,.48),'Graphite':((.025,.032,.045),.35,.32),'Paint':((.78,.79,.77),0,.6),'Brass':((.56,.36,.10),.8,.25),'Steel':((.43,.47,.52),.85,.25),'Blueprint':((.025,.13,.27),0,.65),'PlanInk':((.83,.92,.96),0,.7),'Leaf':((.05,.20,.09),0,.65),'Paper':((.86,.85,.78),0,.85),'Upholstery':((.045,.065,.10),0,.85),'Orange':((.84,.22,.045),.15,.4),'Glass':((.70,.83,.88),0,.12),'FrostedGlass':((.77,.85,.88),0,.65)}
materials={}
# Separate front/back lettering must cull its reverse face through clear glass.
letter_path=folder+'/Materials/M_OfficeEngraving'
letter=unreal.load_asset(letter_path) if unreal.EditorAssetLibrary.does_asset_exist(letter_path) else unreal.EditorAssetLibrary.duplicate_asset('/Engine/EngineMaterials/DefaultTextMaterialOpaque',letter_path)
letter.set_editor_property('two_sided',False);E.recompile_material(letter);unreal.EditorAssetLibrary.save_loaded_asset(letter)
for name,(c,metal,rough) in specs.items():
 path=folder+'/Materials/M_Office_'+name;m=unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else T.create_asset('M_Office_'+name,folder+'/Materials',unreal.Material,unreal.MaterialFactoryNew());E.delete_all_material_expressions(m);m.set_editor_property('two_sided',True)
 n=E.create_material_expression(m,unreal.MaterialExpressionConstant3Vector);n.constant=unreal.LinearColor(*c,1);E.connect_material_property(n,'',unreal.MaterialProperty.MP_BASE_COLOR)
 for val,prop in [(metal,unreal.MaterialProperty.MP_METALLIC),(rough,unreal.MaterialProperty.MP_ROUGHNESS)]:n=E.create_material_expression(m,unreal.MaterialExpressionConstant);n.r=val;E.connect_material_property(n,'',prop)
 if 'Glass' in name:
  m.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT);n=E.create_material_expression(m,unreal.MaterialExpressionConstant);n.r=.22 if name=='FrostedGlass' else .10;E.connect_material_property(n,'',unreal.MaterialProperty.MP_OPACITY)
 E.set_material_usage(m,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES);E.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m);materials['Office_'+name]=m
for row in json.loads((src/'manifest.json').read_text())['assets']:
 name=row['name'];o=unreal.FbxImportUI();o.import_materials=False;o.import_textures=False;o.import_as_skeletal=False;o.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH;o.automated_import_should_detect_type=False;d=o.static_mesh_import_data;d.combine_meshes=True;d.convert_scene=True;d.convert_scene_unit=True;d.generate_lightmap_u_vs=True;d.auto_generate_collision=name=='SM_AssignedOfficeGlassDoor'
 if name in ['SM_ExecutiveCredenza','SM_EngineeringCabinet','SM_PresidentArtwork','SM_BlueprintBoard','SM_OfficeBookshelf','SM_WallClock','SM_FramedCertificate','SM_IndustrialWallArt','SM_EngineeringWhiteboard','SM_OfficeNoticeboard']:d.import_rotation=unreal.Rotator(yaw=180)
 task=unreal.AssetImportTask();task.filename=str(src/(name+'.fbx'));task.destination_path=folder;task.destination_name=name;task.automated=True;task.replace_existing=True;task.save=True;task.options=o;T.import_asset_tasks([task]);m=unreal.load_asset(folder+'/'+name)
 if not m:raise RuntimeError(name+' did not import')
 for i,slot in enumerate(m.get_editor_property('static_materials')):
  key=str(slot.material_slot_name)
  if key not in materials:raise RuntimeError('Unknown original material '+key)
  m.set_material(i,materials[key])
 if name!='SM_AssignedOfficeGlassDoor':unreal.EditorStaticMeshLibrary.remove_collisions(m)
 unreal.EditorAssetLibrary.save_loaded_asset(m);b=m.get_bounding_box();size=b.max-b.min;report['meshes'].append({'name':name,'dimensions_cm':[size.x,size.y,size.z]})
 if name=='SM_AssignedOfficeGlassDoor' and abs(size.z-213.36)>.5:raise RuntimeError('Glass door lost its real scale')
# The character palette is derived from licensed Handyman; keep this part private.
private='/Game/Foundryline/Employees/Office';task=unreal.AssetImportTask();task.filename=str(R/'SourceAssets/Employees/Office/T_OfficeEngineerPalette.png');task.destination_path=private;task.destination_name='T_OfficeEngineerPalette';task.automated=True;task.replace_existing=True;task.save=True;T.import_asset_tasks([task]);tex=unreal.load_asset(private+'/T_OfficeEngineerPalette');tex.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS);tex.set_editor_property('filter',unreal.TextureFilter.TF_NEAREST);unreal.EditorAssetLibrary.save_loaded_asset(tex)
path=private+'/M_OfficeEngineer';m=unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else T.create_asset('M_OfficeEngineer',private,unreal.Material,unreal.MaterialFactoryNew());E.delete_all_material_expressions(m);n=E.create_material_expression(m,unreal.MaterialExpressionTextureSampleParameter2D);n.set_editor_property('parameter_name','Palette');n.set_editor_property('texture',tex);E.connect_material_property(n,'RGB',unreal.MaterialProperty.MP_BASE_COLOR);rough=E.create_material_expression(m,unreal.MaterialExpressionConstant);rough.r=.67;E.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS);E.set_material_usage(m,unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH);E.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m)
o=unreal.FbxImportUI();o.import_materials=False;o.import_textures=False;o.import_as_skeletal=False;o.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH;o.automated_import_should_detect_type=False;o.static_mesh_import_data.combine_meshes=True;o.static_mesh_import_data.convert_scene=True;o.static_mesh_import_data.convert_scene_unit=True;o.static_mesh_import_data.auto_generate_collision=False;task=unreal.AssetImportTask();task.filename=str(R/'SourceAssets/Employees/Office/OfficeEngineerTie.fbx');task.destination_path=private;task.destination_name='SM_OfficeEngineerTie';task.options=o;task.automated=True;task.replace_existing=True;task.save=True;T.import_asset_tasks([task]);tie=unreal.load_asset(private+'/SM_OfficeEngineerTie');tie.set_material(0,m);unreal.EditorAssetLibrary.save_loaded_asset(tie);bounds=tie.get_bounding_box();report['tie_bounds_cm']=[[bounds.min.x,bounds.min.y,bounds.min.z],[bounds.max.x,bounds.max.y,bounds.max.z]]
if not (105<bounds.min.z<130 and 135<bounds.max.z<155):raise RuntimeError('Tie reference-pose scale is incorrect')
report['passed']=True;(R/'Saved/Logs/OfficeFurnitureImportQA.json').write_text(json.dumps(report,indent=2));unreal.log('OFFICE_FURNITURE_IMPORTED');unreal.SystemLibrary.quit_editor()
