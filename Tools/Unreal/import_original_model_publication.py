"""Import ONLY original standalone model assets into an isolated minimal UE project."""
import unreal,json,re
from pathlib import Path
ROOT=Path(r'C:/Users/curly/Documents/Foundryline Game');MAN=ROOT/'SourceAssets/PublishedOriginalModels/manifest.json';rows=json.loads(MAN.read_text())['assets'];rackroot=ROOT/'SourceAssets/Inventory/Storage/ModularPalletRacking';rack=json.loads((rackroot/'manifest.json').read_text());assert rack['original_authorship']
for a in rack['assets']:
 variant=next(v for v in rack['variants'] if v['name']==a['name']);folder,name=a['destination'].rsplit('/',1);rows.append({'id':a['name'],'destination':folder,'asset_name':name,'fbx':str((rackroot/a['file']).relative_to(ROOT)),'bounds_cm':{'size':variant['dimensions_cm']},'materials':{n:{'color':v['base_color'],'metallic':v['metallic'],'roughness':v['roughness'],'coat':0,'emission':[0,0,0],'emission_strength':0} for n,v in rack['materials'].items()},'usable_levels':variant['levels'],'usable_pallet_slots':variant['usable_pallet_slots']})
unreal.SystemLibrary.execute_console_command(unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world(),'Interchange.FeatureFlags.Import.FBX 0')
tools=unreal.AssetToolsHelpers.get_asset_tools();E=unreal.MaterialEditingLibrary;MROOT='/Game/Foundryline/Shared/Materials/OriginalModels';materials={};generated=[]
def clean(s):return re.sub('[^a-z0-9]','',s.lower())
def constant(m,value,at=(0,0)):
 n=E.create_material_expression(m,unreal.MaterialExpressionConstant,*at);n.set_editor_property('r',value);return n
def vector(m,value,at=(0,0)):
 n=E.create_material_expression(m,unreal.MaterialExpressionConstant3Vector,*at);n.constant=unreal.LinearColor(*value,1);return n
def material(name,spec):
 key=clean(name)
 if key in materials:return materials[key]
 slug=re.sub('[^A-Za-z0-9_]','_',name);path=MROOT+'/M_Original_'+slug;m=unreal.load_asset(path) or tools.create_asset('M_Original_'+slug,MROOT,unreal.Material,unreal.MaterialFactoryNew());assert m;E.delete_all_material_expressions(m)
 col=vector(m,spec['color'],(-500,0));E.connect_material_property(col,'',unreal.MaterialProperty.MP_BASE_COLOR)
 for prop,value in [(unreal.MaterialProperty.MP_METALLIC,spec['metallic']),(unreal.MaterialProperty.MP_ROUGHNESS,spec['roughness'])]:E.connect_material_property(constant(m,value),'',prop)
 if spec.get('coat',0)>0:
  m.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_CLEAR_COAT);m.set_editor_property('use_material_attributes',True);attrs=E.create_material_expression(m,unreal.MaterialExpressionMakeMaterialAttributes,200,0);E.connect_material_expressions(col,'',attrs,'BaseColor');E.connect_material_expressions(constant(m,spec['metallic']),'',attrs,'Metallic');E.connect_material_expressions(constant(m,spec['roughness']),'',attrs,'Roughness');E.connect_material_expressions(constant(m,spec['coat']),'',attrs,'ClearCoat');E.connect_material_expressions(constant(m,.16),'',attrs,'ClearCoatRoughness');E.connect_material_property(attrs,'',unreal.MaterialProperty.MP_MATERIAL_ATTRIBUTES)
 if spec.get('emission_strength',0)>0:E.connect_material_property(vector(m,[v*spec['emission_strength'] for v in spec['emission']]),'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
 if 'rawcarbonsteel' in key:
  noise=E.create_material_expression(m,unreal.MaterialExpressionNoise,-700,250);noise.set_editor_property('scale',.15);lerp=E.create_material_expression(m,unreal.MaterialExpressionLinearInterpolate,-200,0);dark=vector(m,[.028,.037,.048],(-500,-100));light=vector(m,[.050,.061,.074],(-500,100));E.connect_material_expressions(dark,'',lerp,'A');E.connect_material_expressions(light,'',lerp,'B');E.connect_material_expressions(noise,'',lerp,'Alpha');E.connect_material_property(lerp,'',unreal.MaterialProperty.MP_BASE_COLOR)
 E.set_material_usage(m,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES);E.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m);materials[key]=m;generated.append(m.get_path_name().split('.')[0]);return m
report={'assets':[],'materials':[],'source':'Original procedural standalone models; no licensed/vendor assets','sandbox_project':unreal.Paths.project_dir()}
for row in rows:
 lookup={clean(name):material(name,spec) for name,spec in row['materials'].items()};opt=unreal.FbxImportUI();opt.import_materials=False;opt.import_textures=False;opt.import_as_skeletal=False;opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH;opt.automated_import_should_detect_type=False;d=opt.static_mesh_import_data;d.combine_meshes=True;d.import_uniform_scale=1.0;d.convert_scene=True;d.convert_scene_unit=True;d.auto_generate_collision=False;d.generate_lightmap_u_vs=True;d.normal_import_method=unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
 unreal.EditorAssetLibrary.delete_asset(row['destination']+'/'+row['asset_name']) if unreal.EditorAssetLibrary.does_asset_exist(row['destination']+'/'+row['asset_name']) else None
 task=unreal.AssetImportTask();task.filename=str(ROOT/row['fbx']);task.destination_path=row['destination'];task.destination_name=row['asset_name'];task.automated=True;task.replace_existing=True;task.save=True;task.options=opt;task.factory=unreal.FbxFactory();tools.import_asset_tasks([task]);mesh=unreal.load_asset(row['destination']+'/'+row['asset_name']);assert mesh,row['id'];slots=[]
 for i,slot in enumerate(mesh.get_editor_property('static_materials')):
  key=clean(str(slot.material_slot_name));matches=[(k,m) for k,m in lookup.items() if k==key];matches=matches or [(k,m) for k,m in lookup.items() if k in key];assert len(matches)==1,(row['id'],str(slot.material_slot_name),list(lookup));mesh.set_material(i,matches[0][1]);slots.append({'slot':str(slot.material_slot_name),'material':matches[0][1].get_path_name()})
 settings=unreal.EditorStaticMeshLibrary.get_lod_build_settings(mesh,0);settings.recompute_normals=False;settings.build_scale3d=unreal.Vector(1,1,1);settings.recompute_tangents=True;unreal.EditorStaticMeshLibrary.set_lod_build_settings(mesh,0,settings);unreal.EditorAssetLibrary.save_loaded_asset(mesh);b=mesh.get_bounds();size=[b.box_extent.x*2,b.box_extent.y*2,b.box_extent.z*2];expected=row['bounds_cm']['size'];assert all(abs(a-b)<.4 for a,b in zip(size,expected)),(row['id'],size,expected)
 report['assets'].append({'id':row['id'],'path':mesh.get_path_name().split('.')[0],'dimensions_cm':size,'materials':slots,'levels':row.get('usable_levels'),'slots':row.get('usable_pallet_slots')})
report['materials']=sorted(set(generated));(ROOT/'Saved/Logs/OriginalModelNativeImport.json').write_text(json.dumps(report,indent=2));unreal.log('ORIGINAL_MODEL_NATIVE_IMPORT_COMPLETE '+str(len(report['assets'])))
