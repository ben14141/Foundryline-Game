"""Import only the original collector cart and supply bins; never touch an existing map."""
import unreal,json
from pathlib import Path
ROOT=Path(unreal.Paths.project_dir()).resolve()
SOURCE=ROOT/'SourceAssets/Production/AssemblySupply'
DEST='/Game/Foundryline/Production/AssemblySupply'
PALETTE={'Supply_FoundryOrange':(.8,.19,.025,.4,.28),'Supply_PartsBlue':(.025,.18,.31,.3,.32),'Supply_GalvanizedSteel':(.48,.55,.60,.85,.27),'Supply_Graphite':(.035,.045,.055,.5,.33),'Supply_CasterRubber':(.012,.018,.022,0,.7),'Supply_Label':(.91,.92,.87,0,.5)}
tools=unreal.AssetToolsHelpers.get_asset_tools();mats={}
for name,values in PALETTE.items():
 path=DEST+'/Materials/M_'+name;m=unreal.load_asset(path)
 if not m:
  m=tools.create_asset('M_'+name,DEST+'/Materials',unreal.Material,unreal.MaterialFactoryNew())
  for prop,value in [(unreal.MaterialProperty.MP_BASE_COLOR,values[:3]),(unreal.MaterialProperty.MP_METALLIC,values[3]),(unreal.MaterialProperty.MP_ROUGHNESS,values[4])]:
   node=unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionConstant3Vector if isinstance(value,tuple) else unreal.MaterialExpressionConstant)
   if isinstance(value,tuple):node.set_editor_property('constant',unreal.LinearColor(*value,1))
   else:node.set_editor_property('r',value)
   unreal.MaterialEditingLibrary.connect_material_property(node,'',prop)
  unreal.MaterialEditingLibrary.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m)
 unreal.MaterialEditingLibrary.set_material_usage(m,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES);unreal.EditorAssetLibrary.save_loaded_asset(m)
 mats[name]=m
report=[]
for name in ['CollectorCart','CollectorCart_Body','CollectorCart_Wheel','CollectorCart_Caster','AssemblySupplyBin_4','AssemblySupplyBin_6','AssemblySupplyBin_8']:
 opt=unreal.FbxImportUI();opt.import_materials=False;opt.import_textures=False;opt.import_as_skeletal=False;opt.automated_import_should_detect_type=False;opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH
 data=opt.static_mesh_import_data;data.combine_meshes=True;data.convert_scene=True;data.convert_scene_unit=True;data.auto_generate_collision=True
 task=unreal.AssetImportTask();task.filename=str(SOURCE/(name+'.fbx'));task.destination_path=DEST;task.destination_name='SM_'+name;task.automated=True;task.replace_existing=True;task.save=True;task.options=opt
 tools.import_asset_tasks([task]);mesh=unreal.load_asset(DEST+'/SM_'+name);assert mesh,name
 for i,slot in enumerate(mesh.get_editor_property('static_materials')):
  key=str(slot.material_slot_name);matches=[mat for label,mat in mats.items() if label.lower() in key.lower()];assert len(matches)==1,(name,key);mesh.set_material(i,matches[0])
 unreal.EditorAssetLibrary.save_loaded_asset(mesh);b=mesh.get_bounds();size=[b.box_extent.x*2,b.box_extent.y*2,b.box_extent.z*2];assert max(size)<250 and min(size)>.1,(name,size)
 report.append({'asset':mesh.get_path_name(),'dimensions_cm':size})
(ROOT/'Saved/Logs/AssemblySupplyImport.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
unreal.log('ASSEMBLY_SUPPLY_IMPORT_PASSED')

wide=ROOT/'SourceAssets/Inventory/Storage/WideBayPalletRacking'
if wide.exists():
 rack_palette={'FL_Rack_Orange':'Supply_FoundryOrange','FL_Rack_Graphite':'Supply_Graphite','FL_Rack_Steel':'Supply_GalvanizedSteel','FL_Rack_SafetyYellow':'Supply_FoundryOrange','FL_Rack_LabelWhite':'Supply_Label'}
 for levels in [1,2,3]:
  name='PalletRack_'+('1Level' if levels==1 else str(levels)+'Levels')
  opt=unreal.FbxImportUI();opt.import_materials=False;opt.import_textures=False;opt.import_as_skeletal=False;opt.automated_import_should_detect_type=False;opt.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH;opt.static_mesh_import_data.combine_meshes=True;opt.static_mesh_import_data.convert_scene=True;opt.static_mesh_import_data.convert_scene_unit=True;opt.static_mesh_import_data.auto_generate_collision=True
  task=unreal.AssetImportTask();task.filename=str(wide/(name+'.fbx'));task.destination_path='/Game/Foundryline/Inventory/Storage/WideBayPalletRacking';task.destination_name='SM_'+name;task.automated=True;task.replace_existing=True;task.save=True;task.options=opt;tools.import_asset_tasks([task]);mesh=unreal.load_asset(task.destination_path+'/'+task.destination_name);assert mesh
  for slot_index,slot in enumerate(mesh.get_editor_property('static_materials')):
   labels=[target for label,target in rack_palette.items() if label.lower() in str(slot.material_slot_name).lower()];assert len(labels)==1,str(slot.material_slot_name);mesh.set_material(slot_index,mats[labels[0]])
  unreal.EditorAssetLibrary.save_loaded_asset(mesh)
 unreal.log('WIDE_BAY_RACK_IMPORT_PASSED')
