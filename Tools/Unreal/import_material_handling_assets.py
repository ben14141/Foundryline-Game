import unreal,json
from pathlib import Path
R=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve();src=R/'SourceAssets/MaterialHandling';data=json.loads((src/'manifest.json').read_text());tools=unreal.AssetToolsHelpers.get_asset_tools();dest='/Game/Foundryline/Shared/Materials/MaterialHandling';textures={};report={'assets':[],'materials':[],'errors':[]}
roller_only='-RollerOnly' in unreal.SystemLibrary.get_command_line()
for file in ([] if roller_only else sorted((src/'Textures').glob('*.png'))):
 t=unreal.AssetImportTask();t.filename=str(file);t.destination_path=dest+'/Textures';t.destination_name=file.stem;t.automated=True;t.replace_existing=True;t.save=True;tools.import_asset_tasks([t]);tex=unreal.load_asset(t.destination_path+'/'+file.stem)
 if file.stem.endswith('_Normal'):tex.set_editor_property('srgb',False);tex.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_NORMALMAP)
 elif file.stem.endswith('_Roughness'):tex.set_editor_property('srgb',False)
 unreal.EditorAssetLibrary.save_loaded_asset(tex);textures[file.stem]=tex
specs=[('MH_FoundryOrange',(.78,.24,.055),.45,.2,'PowderOrange'),('MH_Graphite',(.055,.069,.082),.57,.3,None),('MH_MachinedSteel',(.43,.48,.52),.26,.8,None),('MH_DarkSteel',(.095,.12,.145),.4,.65,None),('MH_Rubber',(.016,.021,.025),.88,0,None),('MH_White',(.86,.88,.82),.38,0,None),('MH_Red',(.6,.018,.01),.35,0,None),('MH_SafetyYellow',(.88,.58,.035),.5,0,None),('MH_Glass',(.025,.06,.075),.12,.2,None),('MH_PalletWood',(.55,.35,.15),.8,0,'PalletWood'),('MH_CarbonSteel',(.4,.42,.44),.43,.8,'SteelSheet')]
materials={}
for name,color,rough,metal,prefix in specs:
 if roller_only:
  materials[name.lower()]=unreal.load_asset(dest+'/M_'+name);continue
 m=unreal.load_asset(dest+'/M_'+name) or tools.create_asset('M_'+name,dest,unreal.Material,unreal.MaterialFactoryNew());unreal.MaterialEditingLibrary.delete_all_material_expressions(m)
 c=unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionConstant3Vector,0,0);c.constant=unreal.LinearColor(*color,1);unreal.MaterialEditingLibrary.connect_material_property(c,'',unreal.MaterialProperty.MP_BASE_COLOR)
 for prop,value in [(unreal.MaterialProperty.MP_ROUGHNESS,rough),(unreal.MaterialProperty.MP_METALLIC,metal)]:
  n=unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionConstant,0,180);n.set_editor_property('r',value);unreal.MaterialEditingLibrary.connect_material_property(n,'',prop)
 if prefix:
  for suffix,prop in [('BaseColor',unreal.MaterialProperty.MP_BASE_COLOR),('Roughness',unreal.MaterialProperty.MP_ROUGHNESS),('Normal',unreal.MaterialProperty.MP_NORMAL)]:
   n=unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionTextureSample,-300,0);n.texture=textures['T_'+prefix+'_'+suffix]
   if suffix=='Normal':n.sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL
   elif suffix=='Roughness':n.sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR
   unreal.MaterialEditingLibrary.connect_material_property(n,'RGB' if suffix!='Roughness' else 'R',prop)
 unreal.MaterialEditingLibrary.set_material_usage(m,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
 unreal.MaterialEditingLibrary.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m);materials[name.lower()]=m;report['materials'].append(m.get_path_name())
for a in data['assets']:
 if roller_only and not a['kind'].startswith('roller'):continue
 o=unreal.FbxImportUI();o.import_materials=False;o.import_textures=False;o.import_as_skeletal=False;o.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH;o.automated_import_should_detect_type=False
 d=o.static_mesh_import_data;d.combine_meshes=True;d.auto_generate_collision=False;d.convert_scene=True;d.convert_scene_unit=True;d.generate_lightmap_u_vs=True
 t=unreal.AssetImportTask();t.filename=str(R/'SourceAssets'/a['source_file']);t.destination_path=a['asset_path'].rsplit('/',1)[0];t.destination_name='SM_'+a['name'];t.automated=True;t.replace_existing=True;t.save=True;t.options=o;t.factory=unreal.FbxFactory();tools.import_asset_tasks([t]);mesh=unreal.load_asset(a['asset_path'])
 if not mesh:raise RuntimeError('Missing imported '+a['name'])
 slots=[]
 for i,slot in enumerate(mesh.get_editor_property('static_materials')):
  key=next((k for k in materials if k in str(slot.material_slot_name).lower()),None)
  if key is None:raise RuntimeError('Unmatched material slot '+str(slot.material_slot_name)+' on '+a['name'])
  mesh.set_material(i,materials[key]);slots.append(key)
 build=unreal.EditorStaticMeshLibrary.get_lod_build_settings(mesh,0);build.recompute_tangents=True;build.use_mikk_t_space=False;unreal.EditorStaticMeshLibrary.set_lod_build_settings(mesh,0,build);unreal.EditorAssetLibrary.save_loaded_asset(mesh)
 b=mesh.get_bounds();size=b.box_extent*2;report['assets'].append({'asset':mesh.get_path_name(),'size_cm':[size.x,size.y,size.z],'materials':slots})
(R/('Saved/Logs/MaterialHandlingLongRollerImport.json' if roller_only else 'Saved/Logs/MaterialHandlingImport.json')).write_text(json.dumps(report,indent=2))
