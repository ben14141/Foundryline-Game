"""Import the local-only welding bench, fixture kit and headache rack as separate static meshes.

The art stays in ignored Content/ and SourceAssets/ (see Docs/REPOSITORY_CONTENT.md); this script only
documents how a local checkout recreates it. Scale is checked against the source QA; nothing is resized.
"""
import unreal,json,os
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
# Source-only worktrees read the art from the main checkout's ignored SourceAssets.
art=Path(os.environ.get('FOUNDRYLINE_SOURCE_ASSETS',str(root/'SourceAssets')))
if not (art/'Production/Equipment/WeldingBench').exists():art=root.parent/'Foundryline Game'/'SourceAssets'
shared='/Game/Foundryline/Shared/Materials/MaterialHandling'
tools=unreal.AssetToolsHelpers.get_asset_tools()
materials={}
for name in ['FoundryOrange','Graphite','MachinedSteel','DarkSteel','Rubber','White','Red','SafetyYellow','Glass']:
 m=unreal.load_asset(shared+'/M_MH_'+name)
 if not m:raise RuntimeError('Missing shared material '+name)
 materials[name]=m
# Ordered keyword rules over FBX slot names; first match wins.
rules=[('orange','FoundryOrange'),('graphite','Graphite'),('polish','MachinedSteel'),('bare','MachinedSteel'),('mill','DarkSteel'),('carbon','DarkSteel'),('raw','DarkSteel'),('weld','DarkSteel'),
 ('rubber','Rubber'),('tire','Rubber'),('cable','Rubber'),('lens','Glass'),('glass','Glass'),('lamp','White'),('light','White'),('led','White'),('red','Red'),('yellow','SafetyYellow'),('copper','MachinedSteel'),
 ('hardware','MachinedSteel'),('bolt','MachinedSteel'),('zinc','MachinedSteel'),('steel','MachinedSteel'),('black','Graphite'),('dark','Graphite')]
assets=[('Production/Equipment/WeldingBench/StandardWeldingBench_SeparateComponents.fbx','/Game/Foundryline/Production/Equipment/WeldingBench','SM_StandardWeldingBench',[243.84,121.92,91.44]),
 # The fixture kit FBX is exported in its rack-assembly pose (Tools/Blender/export_fixture_rack_assembly.py), so it shares the bench floor datum.
 ('Production/Equipment/WeldingBench/WeldingFixtureKit_RackAssembly.fbx','/Game/Foundryline/Production/Equipment/WeldingBench','SM_WeldingFixtureKit',None),
 ('Products/HeadacheRack/HeadacheRack_AllSeparateParts.fbx','/Game/Foundryline/Products/HeadacheRack','SM_HeadacheRack',None)]
report={'source':str(art),'assets':[],'unmatched':[]}
for rel,dest,name,top in assets:
 o=unreal.FbxImportUI();o.import_materials=False;o.import_textures=False;o.import_as_skeletal=False;o.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH;o.automated_import_should_detect_type=False
 d=o.static_mesh_import_data;d.combine_meshes=True;d.convert_scene=True;d.convert_scene_unit=True;d.auto_generate_collision=False;d.generate_lightmap_u_vs=True;d.remove_degenerates=False
 t=unreal.AssetImportTask();t.filename=str(art/rel);t.destination_path=dest;t.destination_name=name;t.automated=True;t.replace_existing=True;t.save=True;t.options=o;t.factory=unreal.FbxFactory();tools.import_asset_tasks([t])
 mesh=unreal.load_asset(dest+'/'+name)
 if not mesh:raise RuntimeError('Missing mesh '+name)
 slots=[]
 for i,slot in enumerate(mesh.get_editor_property('static_materials')):
  label=str(slot.material_slot_name);low=label.lower();key=next((v for k,v in rules if k in low),None)
  if not key:report['unmatched'].append(name+':'+label);key='MachinedSteel'
  mesh.set_material(i,materials[key]);slots.append(label+' -> '+key)
 unreal.EditorAssetLibrary.save_loaded_asset(mesh)
 b=mesh.get_bounding_box();size=b.max-b.min
 entry={'path':mesh.get_path_name(),'min_cm':[b.min.x,b.min.y,b.min.z],'max_cm':[b.max.x,b.max.y,b.max.z],'size_cm':[size.x,size.y,size.z],'slots':slots}
 # The bench's 8 x 4 ft top and 36 in height must survive import exactly (holster/lead extend past the top).
 if top and (abs(b.max.z-top[2])>1.0 or size.y<top[1]-.5 or size.x<top[0]-.5):raise RuntimeError('Bench scale mismatch '+str(entry))
 report['assets'].append(entry)
(root/'Saved/Logs/WeldingStationImport.json').write_text(json.dumps(report,indent=2));unreal.log('WELDING_STATION_IMPORT_COMPLETE '+json.dumps(report))
