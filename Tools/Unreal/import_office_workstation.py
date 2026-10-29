"""Import only the authored triple-monitor workstation; tablet and fabrication assets untouched."""
import unreal,json
from pathlib import Path
root=Path(unreal.Paths.project_dir());tools=unreal.AssetToolsHelpers.get_asset_tools();e=unreal.MaterialEditingLibrary;folder='/Game/Foundryline/UI/Operations'
def plain(name,color,rough,metal):
 m=unreal.load_asset(folder+'/'+name)
 if not m:m=tools.create_asset(name,folder,unreal.Material,unreal.MaterialFactoryNew())
 e.delete_all_material_expressions(m);v=e.create_material_expression(m,unreal.MaterialExpressionConstant3Vector);v.constant=unreal.LinearColor(*color,1);e.connect_material_property(v,'',unreal.MaterialProperty.MP_BASE_COLOR)
 for value,prop in [(rough,unreal.MaterialProperty.MP_ROUGHNESS),(metal,unreal.MaterialProperty.MP_METALLIC)]:n=e.create_material_expression(m,unreal.MaterialExpressionConstant);n.r=value;e.connect_material_property(n,'',prop)
 e.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m);return m
specs={'case':((.045,.055,.065),.44,.16),'desk':((.72,.73,.70),.64,.04),'keycap':((.035,.042,.05),.65,0),'legend':((.72,.76,.77),.65,0),'bezel':((.012,.017,.023),.38,.1),'orange':((.75,.17,.025),.4,.15),'port':((.004,.007,.01),.7,.05),'indicator':((.1,.55,.34),.35,0)}
mapping={'ops_'+key:plain('M_Office_'+key.capitalize(),*values) for key,values in specs.items()}
for n in ['Graphite','White','FoundryOrange','MachinedSteel','CarbonSteel','Rubber']:mapping['ops_'+n.lower()]=unreal.load_asset('/Game/Foundryline/Shared/Materials/MaterialHandling/M_MH_'+n)
screen=unreal.load_asset(folder+'/M_OfficeDesktopScreen')
if not screen:screen=tools.create_asset('M_OfficeDesktopScreen',folder,unreal.Material,unreal.MaterialFactoryNew())
exec((root/'Tools/Unreal/repair_office_desktop_material.py').read_text())

o=unreal.FbxImportUI();o.import_materials=False;o.import_textures=False;o.import_as_skeletal=False;o.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH;o.automated_import_should_detect_type=False;d=o.static_mesh_import_data;d.combine_meshes=True;d.convert_scene=True;d.convert_scene_unit=True;d.generate_lightmap_u_vs=True;d.auto_generate_collision=True;d.normal_import_method=unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
task=unreal.AssetImportTask();task.filename=str(root/'SourceAssets/Operations/OfficeWorkstation.fbx');task.destination_path=folder;task.destination_name='SM_OfficeWorkstation';task.automated=True;task.replace_existing=True;task.save=True;task.options=o;tools.import_asset_tasks([task]);mesh=unreal.load_asset(folder+'/SM_OfficeWorkstation')
if not mesh:raise RuntimeError('Office workstation import failed')
slots=[]
for i,s in enumerate(mesh.get_editor_property('static_materials')):
 name=str(s.material_slot_name);material=screen if 'OfficeDesktopScreen' in name else mapping.get(name.lower(),mapping['ops_case']);mesh.set_material(i,material);slots.append({'slot':name,'material':material.get_path_name() if material else None})
unreal.EditorAssetLibrary.save_loaded_asset(mesh);bounds=mesh.get_bounding_box();report={'asset':mesh.get_path_name(),'slots':slots,'min_cm':[bounds.min.x,bounds.min.y,bounds.min.z],'max_cm':[bounds.max.x,bounds.max.y,bounds.max.z]}
for label in ['Left','Center','Right']:
 if not any(s['slot']=='OfficeDesktopScreen_'+label for s in slots):raise RuntimeError('Missing distinct screen slot '+label)
(root/'Saved/Logs/OfficeWorkstationImport.json').write_text(json.dumps(report,indent=2));unreal.log('OFFICE_WORKSTATION_IMPORT_OK')
