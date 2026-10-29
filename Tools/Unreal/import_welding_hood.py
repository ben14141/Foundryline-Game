"""Import Codex's approved V2 welding hood (SourceAssets/Employees/Equipment/WeldingHood) as two static meshes.

Harness and Visor keep their FBX origins on the shared ear hinge (no bounds centring), scale 1, no collision; the parts of
each FBX are combined into one mesh. The six original materials are built from manifest.json and assigned by slot name.
Writes Saved/Logs/WeldingHoodImport.json with each mesh's bounds and the combined closed bounds for comparison with the
manifest. Run: UnrealEditor-Cmd Foundryline.uproject -run=pythonscript -script=<abs path to this file>
"""
import unreal,json,re
from pathlib import Path
ROOT=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
SRC=ROOT/'SourceAssets/Employees/Equipment/WeldingHood';DEST='/Game/Foundryline/Employees/Equipment/WeldingHood';MROOT='/Game/Foundryline/Shared/Materials/OriginalModels'
manifest=json.loads((SRC/'manifest.json').read_text());tools=unreal.AssetToolsHelpers.get_asset_tools();E=unreal.MaterialEditingLibrary;made={}
def material(name):
 if name in made:return made[name]
 spec=manifest['materials'][name];slug='M_Original_'+name;m=unreal.load_asset(MROOT+'/'+slug) or tools.create_asset(slug,MROOT,unreal.Material,unreal.MaterialFactoryNew());E.delete_all_material_expressions(m)
 c=E.create_material_expression(m,unreal.MaterialExpressionConstant3Vector,-500,0);c.constant=unreal.LinearColor(*spec['base_color'],1);E.connect_material_property(c,'',unreal.MaterialProperty.MP_BASE_COLOR)
 for prop,value,y in [(unreal.MaterialProperty.MP_METALLIC,spec['metallic'],100),(unreal.MaterialProperty.MP_ROUGHNESS,spec['roughness'],200)]:
  n=E.create_material_expression(m,unreal.MaterialExpressionConstant,-400,y);n.set_editor_property('r',value);E.connect_material_property(n,'',prop)
 E.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m);made[name]=m;return m
def slot_material(slot):
 key=re.sub('[^a-z0-9]','',slot.lower());hits=[n for n in manifest['materials'] if re.sub('[^a-z0-9]','',n.lower()) in key];assert len(hits)==1,(slot,hits);return material(hits[0])
report={'assets':[]};box=None
for fbx,name in [('WeldingHood_Harness.fbx','SM_WeldingHood_Harness'),('WeldingHood_Visor.fbx','SM_WeldingHood_Visor')]:
 o=unreal.FbxImportUI();o.import_materials=False;o.import_textures=False;o.import_as_skeletal=False;o.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH;o.automated_import_should_detect_type=False
 d=o.static_mesh_import_data;d.combine_meshes=True;d.import_uniform_scale=1.0;d.convert_scene=True;d.convert_scene_unit=True;d.auto_generate_collision=False;d.generate_lightmap_u_vs=True;d.normal_import_method=unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
 path=DEST+'/'+name
 if unreal.EditorAssetLibrary.does_asset_exist(path):unreal.EditorAssetLibrary.delete_asset(path)
 t=unreal.AssetImportTask();t.filename=str(SRC/fbx);t.destination_path=DEST;t.destination_name=name;t.automated=True;t.replace_existing=True;t.save=False;t.options=o;t.factory=unreal.FbxFactory();tools.import_asset_tasks([t])
 mesh=unreal.load_asset(path);assert mesh,name;slots=[]
 for i,s in enumerate(mesh.get_editor_property('static_materials')):m=slot_material(str(s.material_slot_name));mesh.set_material(i,m);slots.append(m.get_name())
 unreal.EditorStaticMeshLibrary.remove_collisions(mesh);unreal.EditorAssetLibrary.save_loaded_asset(mesh);b=mesh.get_bounding_box()
 lo=[b.min.x,b.min.y,b.min.z];hi=[b.max.x,b.max.y,b.max.z];box=(lo,hi) if not box else ([min(a,c) for a,c in zip(box[0],lo)],[max(a,c) for a,c in zip(box[1],hi)])
 report['assets'].append({'path':path,'min':[round(v,2) for v in lo],'max':[round(v,2) for v in hi],'materials':slots,'triangles':unreal.EditorStaticMeshLibrary.get_number_verts(mesh,0)})
report['combined_closed_size_cm']=[round(h-l,2) for l,h in zip(*box)];report['manifest_size_cm']=[round(v,2) for v in manifest['bounds_cm']['size']]
(ROOT/'Saved/Logs/WeldingHoodImport.json').write_text(json.dumps(report,indent=1))
