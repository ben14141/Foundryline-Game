import unreal,json
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
source=root/'SourceAssets/MegapolyTrucks'
dest='/Game/Foundryline/Vehicles/MegapolyTrucks'
tools=unreal.AssetToolsHelpers.get_asset_tools()
report={'meshes':[],'errors':[]}
def task(path,name,folder,options=None,factory=None):
 existing=unreal.load_asset(dest+'/'+folder+'/'+name)
 if existing:return [existing]
 t=unreal.AssetImportTask();t.filename=str(path);t.destination_path=dest+'/'+folder;t.destination_name=name;t.automated=True;t.replace_existing=False;t.save=True
 if options:t.options=options
 if factory:t.factory=factory
 tools.import_asset_tasks([t])
 return [unreal.load_asset(x) for x in t.imported_object_paths]
tex=unreal.load_asset(dest+'/Textures/T_MegapolyGradient') or task(source/'Tex_256Gradient.png','T_MegapolyGradient','Textures')[0]
mat=unreal.load_asset(dest+'/Materials/M_MegapolyMain') or tools.create_asset('M_MegapolyMain',dest+'/Materials',unreal.Material,unreal.MaterialFactoryNew())
e=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionTextureSample,0,0);e.texture=tex
unreal.MaterialEditingLibrary.connect_material_property(e,'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
r=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionConstant,0,180);r.set_editor_property('r',0.75)
unreal.MaterialEditingLibrary.connect_material_property(r,'',unreal.MaterialProperty.MP_ROUGHNESS)
unreal.MaterialEditingLibrary.recompile_material(mat);unreal.EditorAssetLibrary.save_loaded_asset(mat)
glass=unreal.load_asset(dest+'/Materials/M_MegapolyGlass') or tools.create_asset('M_MegapolyGlass',dest+'/Materials',unreal.Material,unreal.MaterialFactoryNew())
c=unreal.MaterialEditingLibrary.create_material_expression(glass,unreal.MaterialExpressionConstant3Vector,0,0);c.constant=unreal.LinearColor(.035,.065,.085,1)
unreal.MaterialEditingLibrary.connect_material_property(c,'',unreal.MaterialProperty.MP_BASE_COLOR)
r=unreal.MaterialEditingLibrary.create_material_expression(glass,unreal.MaterialExpressionConstant,0,180);r.set_editor_property('r',.18)
unreal.MaterialEditingLibrary.connect_material_property(r,'',unreal.MaterialProperty.MP_ROUGHNESS)
unreal.MaterialEditingLibrary.recompile_material(glass);unreal.EditorAssetLibrary.save_loaded_asset(glass)
for f in sorted(source.iterdir()):
 if f.suffix.lower()!='.fbx':continue
 try:
  o=unreal.FbxImportUI();o.import_mesh=True;o.import_materials=False;o.import_textures=False;o.import_as_skeletal=False;o.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH;o.automated_import_should_detect_type=False
  d=o.static_mesh_import_data;d.combine_meshes=True;d.generate_lightmap_u_vs=True;d.auto_generate_collision=True;d.convert_scene=True;d.convert_scene_unit=True;d.import_uniform_scale=1.0
  assets=task(f,'SM_'+f.stem,'Meshes',o,unreal.FbxFactory())
  for mesh in assets:
   if not isinstance(mesh,unreal.StaticMesh):continue
   slots=mesh.get_editor_property('static_materials')
   for i,slot in enumerate(slots):mesh.set_material(i,glass if 'glass' in str(slot.material_slot_name).lower() else mat)
   unreal.EditorAssetLibrary.save_loaded_asset(mesh)
   b=mesh.get_bounds();size=b.box_extent*2
   report['meshes'].append({'asset':mesh.get_path_name(),'size_cm':[size.x,size.y,size.z],'slots':[str(s.material_slot_name) for s in slots],'lods':mesh.get_num_lods()})
 except Exception as e:report['errors'].append(str(f)+': '+str(e))
(root/'Saved/Logs/MegapolyTrucksImport.json').write_text(json.dumps(report,indent=2))
