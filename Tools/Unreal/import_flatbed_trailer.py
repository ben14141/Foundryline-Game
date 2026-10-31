"""Import the approved 53 ft aluminium flatbed trailer (SourceAssets/Vehicles/FlatbedTrailer) as static meshes.

The body, four dual-wheel sets and two landing-gear legs keep their FBX origins: the body on the ground point under the
kingpin, each wheel set on its axle centre, each leg on its foot top (raised). manifest.json gives every offset in
Unreal cm. Slot 0 uses the starter hauler's palette material (the Megapoly 256-gradient texture the UVs point into);
slot 1 is an emissive version of it for the marker lamps. LODs 1-2 are generated for the body and wheels.
The trailer is not wired into gameplay. Writes Saved/Logs/FlatbedTrailerImport.json.
Run (LODs need the full editor): UnrealEditor Foundryline.uproject -RenderOffscreen -ExecutePythonScript=<abs path to this file>
"""
import unreal,json
from pathlib import Path
ROOT=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
SRC=ROOT/'SourceAssets/Vehicles/FlatbedTrailer';DEST='/Game/Foundryline/Vehicles/FlatbedTrailer';HAULER='/Game/Foundryline/Logistics/StarterHauler'
tools=unreal.AssetToolsHelpers.get_asset_tools();E=unreal.MaterialEditingLibrary;A=unreal.EditorAssetLibrary
manifest=json.loads((SRC/'manifest.json').read_text())
palette=unreal.load_asset(HAULER+'/M_TruckPalette');texture=unreal.load_asset(HAULER+'/T_TruckPalette')
assert palette and texture,'Run import_starter_hauler.py first: the trailer uses its palette material'
lamp=unreal.load_asset(DEST+'/M_TrailerMarkerLight') or tools.create_asset('M_TrailerMarkerLight',DEST,unreal.Material,unreal.MaterialFactoryNew())
E.delete_all_material_expressions(lamp)
t=E.create_material_expression(lamp,unreal.MaterialExpressionTextureSample,-600,0);t.texture=texture
E.connect_material_property(t,'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
k=E.create_material_expression(lamp,unreal.MaterialExpressionConstant,-600,200);k.r=3.0
mul=E.create_material_expression(lamp,unreal.MaterialExpressionMultiply,-300,150);E.connect_material_expressions(t,'RGB',mul,'A');E.connect_material_expressions(k,'',mul,'B')
E.connect_material_property(mul,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
r=E.create_material_expression(lamp,unreal.MaterialExpressionConstant,-300,300);r.r=0.3;E.connect_material_property(r,'',unreal.MaterialProperty.MP_ROUGHNESS)
E.recompile_material(lamp);A.save_loaded_asset(lamp)
# The subsystem (and LOD reduction) needs the full editor; the commandlet falls back to the deprecated library.
SM=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.EditorStaticMeshLibrary
lods=unreal.StaticMeshReductionOptions()
lods.reduction_settings=[unreal.StaticMeshReductionSettings(1.0,1.0),unreal.StaticMeshReductionSettings(0.35,0.3),unreal.StaticMeshReductionSettings(0.1,0.12)]
report={'assets':[]}
for name in ['Trailer_Body']+['Trailer_Wheel_%d'%i for i in range(4)]+['Trailer_Leg_%d'%i for i in range(2)]:
 o=unreal.FbxImportUI();o.import_materials=False;o.import_textures=False;o.import_as_skeletal=False;o.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH;o.automated_import_should_detect_type=False
 d=o.static_mesh_import_data;d.combine_meshes=True;d.import_uniform_scale=1.0;d.convert_scene=True;d.convert_scene_unit=True;d.auto_generate_collision=False;d.generate_lightmap_u_vs=True;d.normal_import_method=unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
 path=DEST+'/SM_'+name
 if A.does_asset_exist(path):A.delete_asset(path)
 task=unreal.AssetImportTask();task.filename=str(SRC/(name+'.fbx'));task.destination_path=DEST;task.destination_name='SM_'+name;task.automated=True;task.replace_existing=True;task.save=False;task.options=o;task.factory=unreal.FbxFactory();tools.import_asset_tasks([task])
 mesh=unreal.load_asset(path);assert mesh,name;slots=[]
 for i,s in enumerate(mesh.get_editor_property('static_materials')):
  m=lamp if 'markerlight' in str(s.material_slot_name).lower() else palette;mesh.set_material(i,m);slots.append([str(s.material_slot_name),m.get_name()])
 SM.remove_collisions(mesh)
 if not name.startswith('Trailer_Leg'):SM.set_lods(mesh,lods)
 A.save_loaded_asset(mesh);b=mesh.get_bounding_box()
 report['assets'].append({'path':path,'min':[round(v,2) for v in (b.min.x,b.min.y,b.min.z)],'max':[round(v,2) for v in (b.max.x,b.max.y,b.max.z)],
  'materials':slots,'lods':SM.get_lod_count(mesh),'lod0_triangles':[mesh.get_num_triangles(i) for i in range(SM.get_lod_count(mesh))],
  'blender_triangles':manifest['parts'][name]['triangles']})
(ROOT/'Saved/Logs/FlatbedTrailerImport.json').write_text(json.dumps(report,indent=1))
if 'run=pythonscript' not in unreal.SystemLibrary.get_command_line().lower():unreal.SystemLibrary.quit_editor()
