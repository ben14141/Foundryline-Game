import unreal,json
from pathlib import Path
root=Path(unreal.Paths.project_dir());src=root/'SourceAssets/StarterHauler';folder='/Game/Foundryline/Logistics/StarterHauler';tools=unreal.AssetToolsHelpers.get_asset_tools();e=unreal.MaterialEditingLibrary
texture=unreal.load_asset(folder+'/T_TruckPalette')
if not texture:
 tex=unreal.AssetImportTask();tex.filename=str(root/'SourceAssets/MegapolySmallTrucks/Textures/Tex_256Gradient.png');tex.destination_path=folder;tex.destination_name='T_TruckPalette';tex.automated=True;tex.save=True;tools.import_asset_tasks([tex]);texture=unreal.load_asset(folder+'/T_TruckPalette')
if not texture:raise RuntimeError('Missing truck palette texture')
mat=unreal.load_asset(folder+'/M_TruckPalette')
if not mat:
 mat=tools.create_asset('M_TruckPalette',folder,unreal.Material,unreal.MaterialFactoryNew());n=e.create_material_expression(mat,unreal.MaterialExpressionTextureSample);n.texture=texture;e.connect_material_property(n,'RGB',unreal.MaterialProperty.MP_BASE_COLOR);r=e.create_material_expression(mat,unreal.MaterialExpressionConstant);r.r=.48;e.connect_material_property(r,'',unreal.MaterialProperty.MP_ROUGHNESS);e.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat)
def solid(name,color,roughness):
 m=unreal.load_asset(folder+'/'+name)
 if not m:
  m=tools.create_asset(name,folder,unreal.Material,unreal.MaterialFactoryNew());n=e.create_material_expression(m,unreal.MaterialExpressionConstant3Vector);n.constant=unreal.LinearColor(*color,1);e.connect_material_property(n,'',unreal.MaterialProperty.MP_BASE_COLOR);r=e.create_material_expression(m,unreal.MaterialExpressionConstant);r.r=roughness;e.connect_material_property(r,'',unreal.MaterialProperty.MP_ROUGHNESS);e.recompile_material(m)
 unreal.EditorAssetLibrary.save_loaded_asset(m);return m
paint=solid('M_TruckCabPaint',(.82,.84,.86),.38);glass=solid('M_TruckGlass',(.025,.055,.08),.15)
meshes=[]
for name in ['StarterFlatbed']+['Wheel'+str(i) for i in range(6)]+['LoadingSideGate']:
 o=unreal.FbxImportUI();o.import_materials=False;o.import_textures=False;o.import_as_skeletal=False;o.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH;o.automated_import_should_detect_type=False;d=o.static_mesh_import_data;d.combine_meshes=True;d.convert_scene=True;d.convert_scene_unit=True;d.auto_generate_collision=False
 t=unreal.AssetImportTask();t.filename=str(src/(name+'.fbx'));t.destination_path=folder;t.destination_name='SM_'+name;t.automated=True;t.replace_existing=True;t.save=True;t.options=o;tools.import_asset_tasks([t]);m=unreal.load_asset(folder+'/SM_'+name)
 if not m:raise RuntimeError(name)
 for i,slot in enumerate(m.get_editor_property('static_materials')):
  name=str(slot.material_slot_name).lower();m.set_material(i,glass if 'glass' in name else paint if 'cabpaint' in name else mat)
 unreal.EditorAssetLibrary.save_loaded_asset(m);meshes.append(m)
path='/Game/Foundryline/Logistics';lib=unreal.load_asset(path+'/DA_FreightVehicles') or tools.create_asset('DA_FreightVehicles',path,unreal.FoundrylineFreightVehicleLibrary,unreal.DataAssetFactory())
lib.set_editor_property('bodies',[None,meshes[0],None,None]);lib.set_editor_property('body_offsets',[unreal.Vector(),unreal.Vector(0,134.4,0),unreal.Vector(),unreal.Vector()]);lib.set_editor_property('yaw_offsets',[0,90,0,0]);lib.set_editor_property('starter_wheels',meshes[1:7]);j=json.loads((src/'manifest.json').read_text());lib.set_editor_property('starter_wheel_positions',[unreal.Vector(-r['centre_cm'][1],r['centre_cm'][0]+134.4,r['centre_cm'][2]) for r in j['wheels']]);unreal.EditorAssetLibrary.save_loaded_asset(lib)
(root/'Saved/Logs/StarterHaulerImported.json').write_text(json.dumps({'meshes':[m.get_path_name() for m in meshes],'library':lib.get_path_name(),'source_dimensions_cm':j['size_cm']},indent=2))

chairs=[]
for name in ['SM_Chairs_01','SM_Chairs_02']:
 m=unreal.load_asset('/Game/Factory/Meshes/Office/'+name)
 if m:
  b=m.get_bounding_box();chairs.append({'name':name,'min':[b.min.x,b.min.y,b.min.z],'max':[b.max.x,b.max.y,b.max.z]})
(root/'Saved/Logs/OfficeChairBounds.json').write_text(json.dumps(chairs,indent=2))
