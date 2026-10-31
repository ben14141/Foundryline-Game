"""Import original hinged personnel leaves and fixed jamb frames, without touching maps/vendor assets.
Leaves have simple dynamic collision. Fixed frame meshes intentionally have no hull across the doorway;
the construction renderer supplies jamb/header collision separately.
"""
import unreal,json
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
src=root/'SourceAssets/Buildings/Doors/Personnel';manifest=json.loads((src/'manifest.json').read_text())
folder='/Game/Foundryline/Buildings/Doors/Personnel';matfolder=folder+'/Materials'
tools=unreal.AssetToolsHelpers.get_asset_tools();edit=unreal.MaterialEditingLibrary
specs={'Graphite':([.072,.088,.10],.27,.35),'LightGrey':([.61,.65,.64],.27,.3),'Stainless':([.48,.53,.55],.23,.85),'Rubber':([.012,.017,.019],.75,0),'FoundryOrange':([.85,.28,.055],.3,.25),'Glass':([.70,.85,.87],.08,0)}
materials={}
for name,(color,roughness,metallic) in specs.items():
 m=unreal.load_asset(matfolder+'/M_Personnel_'+name) or tools.create_asset('M_Personnel_'+name,matfolder,unreal.Material,unreal.MaterialFactoryNew())
 edit.delete_all_material_expressions(m)
 c=edit.create_material_expression(m,unreal.MaterialExpressionConstant3Vector);c.set_editor_property('constant',unreal.LinearColor(*color,1));edit.connect_material_property(c,'',unreal.MaterialProperty.MP_BASE_COLOR)
 for value,prop in [(roughness,unreal.MaterialProperty.MP_ROUGHNESS),(metallic,unreal.MaterialProperty.MP_METALLIC)]:
  e=edit.create_material_expression(m,unreal.MaterialExpressionConstant);e.set_editor_property('r',value);edit.connect_material_property(e,'',prop)
 if name=='Glass':
  m.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT);m.set_editor_property('two_sided',True);m.set_editor_property('translucency_lighting_mode',unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
  e=edit.create_material_expression(m,unreal.MaterialExpressionConstant);e.set_editor_property('r',.18);edit.connect_material_property(e,'',unreal.MaterialProperty.MP_OPACITY)
 edit.set_material_usage(m,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES);edit.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m);materials[('fd_'+name).lower()]=m
report={'assets':[],'errors':[],'leaf_panel_cm':manifest['leaf_panel_cm'],'rough_opening_cm':manifest['rough_opening_cm'],'hinge_offset_cm':manifest['unreal_leaf_hinge_offset_cm']}
for a in manifest['assets']:
 leaf=a['name'].endswith('Leaf')
 options=unreal.FbxImportUI();options.import_materials=False;options.import_textures=False;options.import_as_skeletal=False;options.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH;options.automated_import_should_detect_type=False
 data=options.static_mesh_import_data;data.combine_meshes=True;data.convert_scene=True;data.convert_scene_unit=True;data.auto_generate_collision=leaf;data.generate_lightmap_u_vs=True
 task=unreal.AssetImportTask();task.filename=str(src/(a['name']+'.fbx'));task.destination_path=folder;task.destination_name='SM_'+a['name'];task.automated=True;task.replace_existing=True;task.save=True;task.options=options;task.factory=unreal.FbxFactory()
 tools.import_asset_tasks([task]);mesh=unreal.load_asset(folder+'/SM_'+a['name'])
 if not mesh:raise RuntimeError('Missing mesh '+a['name'])
 for i,slot in enumerate(mesh.get_editor_property('static_materials')):
  key=next((key for key in materials if key in str(slot.material_slot_name).lower()),None)
  if key is None:raise RuntimeError('Unmatched material '+str(slot.material_slot_name))
  mesh.set_material(i,materials[key])
 if not leaf:unreal.EditorStaticMeshLibrary.remove_collisions(mesh)
 settings=unreal.EditorStaticMeshLibrary.get_lod_build_settings(mesh,0);settings.recompute_tangents=True;settings.use_mikk_t_space=False;unreal.EditorStaticMeshLibrary.set_lod_build_settings(mesh,0,settings);unreal.EditorAssetLibrary.save_loaded_asset(mesh)
 size=mesh.get_bounds().box_extent*2;actual=[size.x,size.y,size.z]
 if any(abs(x-y)>.3 for x,y in zip(actual,a['dimensions_cm'])):raise RuntimeError('Scale mismatch '+a['name'])
 report['assets'].append({'path':mesh.get_path_name(),'dimensions_cm':actual,'has_materials':all(m.material_interface for m in mesh.get_editor_property('static_materials')),'simple_collision':leaf})
(root/'Saved/Logs/PersonnelDoorImport.json').write_text(json.dumps(report,indent=2));unreal.log('PERSONNEL_DOORS_IMPORTED')
