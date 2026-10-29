"""Import the riser and hold-down clamp split from Codex's fixture kit (Tools/Blender/export_fixture_jig_parts.py).

Each FBX material slot is bound to the matching original material in /Game/Foundryline/Shared/Materials/OriginalModels,
exactly as Codex's kit uses them. Scale is unchanged (Blender metres, Unreal centimetres). Writes a JSON report.
Run: UnrealEditor-Cmd Foundryline.uproject -run=pythonscript -script=Tools/Unreal/import_fixture_jig_parts.py
"""
import unreal,json,re
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
src=root/'SourceAssets/PublishedOriginalModels/WeldingFixtureKit';dest='/Game/Foundryline/Production/Equipment/WeldingBench'
tools=unreal.AssetToolsHelpers.get_asset_tools();report={'assets':[]}
for name in ['WeldingFixtureRiser','WeldingFixtureHoldDown']:
 o=unreal.FbxImportUI();o.import_materials=False;o.import_textures=False;o.import_as_skeletal=False;o.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH;o.automated_import_should_detect_type=False
 d=o.static_mesh_import_data;d.combine_meshes=True;d.convert_scene=True;d.convert_scene_unit=True;d.auto_generate_collision=False;d.generate_lightmap_u_vs=True
 t=unreal.AssetImportTask();t.filename=str(src/(name+'.fbx'));t.destination_path=dest;t.destination_name='SM_'+name;t.automated=True;t.replace_existing=True;t.save=False;t.options=o;t.factory=unreal.FbxFactory();tools.import_asset_tasks([t])
 m=unreal.load_asset(dest+'/SM_'+name)
 if not m:raise RuntimeError('Import failed: '+name)
 slots=m.static_materials;bound=[]
 for i,s in enumerate(slots):
  path='/Game/Foundryline/Shared/Materials/OriginalModels/M_Original_'+re.sub('[^A-Za-z0-9]','_',str(s.material_slot_name))
  mat=unreal.load_asset(path)
  if not mat:raise RuntimeError('No original material for slot '+str(s.material_slot_name))
  m.set_material(i,mat);bound.append(path)
 unreal.EditorAssetLibrary.save_loaded_asset(m)
 b=m.get_bounding_box();report['assets'].append({'path':dest+'/SM_'+name,'min':[b.min.x,b.min.y,b.min.z],'max':[b.max.x,b.max.y,b.max.z],'materials':bound})
(root/'Saved/Logs/FixtureJigImport.json').write_text(json.dumps(report,indent=1))
