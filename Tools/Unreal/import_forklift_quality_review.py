"""LOCAL ONLY: import the licensed-derived forklift quality review over the existing SM_Forklift_* components.

Inputs are generated on this PC by Codex's upgrade-kit tools (Tools/Blender/build_forklift_amber_beacon.py,
refine_forklift_quality_20261029.py, fix_forklift_review_labels.py, verify_forklift_quality_20261029.py) into the ignored
SourceAssets/Vehicles/Forklift/QualityReview_2026-10-29. The meshes keep their names, so the vehicle code, wheel spin and
mast lift animation are unchanged. The old embedded beacon is already removed from the body and Codex's amber beacon
housing sits on the roof at (-115,0,253); the game adds the point light at the bulb (-115,0,263.1). The
beacon lens and bulb get deep-blue variants (Ben, 29 Oct).

Nothing this script writes may be committed: the forklift meshes are Factory-derived. Original FQ_/Beacon_ materials are
created next to them (Content/Foundryline/Vehicles/Forklift/FL01/Materials); MH_ slots reuse the project's M_MH_ materials.
Writes Saved/Logs/ForkliftQualityImport.json.
Run: UnrealEditor-Cmd Foundryline.uproject -run=pythonscript -script=<abs path to this file>
"""
import unreal,json,re
from pathlib import Path
ROOT=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
SRC=ROOT/'SourceAssets/Vehicles/Forklift/QualityReview_2026-10-29';DEST='/Game/Foundryline/Vehicles/Forklift/FL01';MROOT=DEST+'/Materials';MH='/Game/Foundryline/Shared/Materials/MaterialHandling/M_MH_'
# Values from Forklift_QualityReview.blend: colour, metallic, roughness, coat, (emission colour, strength), opacity.
Original={
 'FQ_GlossOrange':([.79,.19,.027],.15,.25,.5,None,1),'FQ_GraphiteSteel':([.036,.048,.058],.65,.30,0,None,1),'FQ_MachinedSteel':([.36,.42,.47],.92,.24,0,None,1),
 'FQ_Rubber':([.013,.017,.019],0,.68,0,None,1),'FQ_SurfacePrintedWhite':([.80,.83,.85],0,.45,0,None,1),'FQ_WorkLampGlass':([.73,.83,.88],0,.20,0,([.64,.78,1],.4),1),
 'FQ_SeatVinyl':([.027,.034,.04],0,.47,0,None,1),
 # The beacon is deep blue in game (Ben, 29 Oct): blue lens and bulb variants; Codex's amber values are not edited.
 'Beacon_BlueLens':([.02,.09,1],0,.16,0,([.01,.08,1],.6),.45),'Beacon_BlueBulb':([.55,.7,1],0,.22,0,([.01,.08,1],3),1),'Beacon_CeramicSocket':([.65,.66,.62],0,.46,0,None,1),
 'Beacon_GraphiteHousing':([.025,.033,.043],.6,.26,0,None,1),'Beacon_RubberGasket':([.012,.016,.018],0,.70,0,None,1),'Beacon_StainlessHardware':([.42,.47,.51],.95,.24,0,None,1)}
manifest=json.loads((SRC/'manifest.json').read_text())
tools=unreal.AssetToolsHelpers.get_asset_tools();E=unreal.MaterialEditingLibrary;made={}
def constant(m,v,at):n=E.create_material_expression(m,unreal.MaterialExpressionConstant,*at);n.set_editor_property('r',v);return n
def vector(m,v,at):n=E.create_material_expression(m,unreal.MaterialExpressionConstant3Vector,*at);n.constant=unreal.LinearColor(v[0],v[1],v[2],1);return n
def material(name):
 if name in made:return made[name]
 col,metal,rough,coat,emit,opacity=Original[name];slug='M_'+name;m=unreal.load_asset(MROOT+'/'+slug) or tools.create_asset(slug,MROOT,unreal.Material,unreal.MaterialFactoryNew());E.delete_all_material_expressions(m)
 if opacity<1:m.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT);m.set_editor_property('translucency_lighting_mode',unreal.TranslucencyLightingMode.TLM_SURFACE);E.connect_material_property(constant(m,opacity,(-400,200)),'',unreal.MaterialProperty.MP_OPACITY)
 c=vector(m,col,(-500,0))
 if coat:
  m.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_CLEAR_COAT);m.set_editor_property('use_material_attributes',True);a=E.create_material_expression(m,unreal.MaterialExpressionMakeMaterialAttributes,200,0)
  for pin,node in [('BaseColor',c),('Metallic',constant(m,metal,(-400,0))),('Roughness',constant(m,rough,(-400,100))),('ClearCoat',constant(m,coat,(-400,300))),('ClearCoatRoughness',constant(m,.16,(-400,400)))]:E.connect_material_expressions(node,'',a,pin)
  E.connect_material_property(a,'',unreal.MaterialProperty.MP_MATERIAL_ATTRIBUTES)
 else:E.connect_material_property(c,'',unreal.MaterialProperty.MP_BASE_COLOR);E.connect_material_property(constant(m,metal,(-400,0)),'',unreal.MaterialProperty.MP_METALLIC);E.connect_material_property(constant(m,rough,(-400,100)),'',unreal.MaterialProperty.MP_ROUGHNESS)
 if emit:E.connect_material_property(vector(m,[x*emit[1] for x in emit[0]],(-500,500)),'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
 E.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m);made[name]=m;return m
def slot_material(slot):
 key=re.sub('[^a-z0-9]','',slot.lower())
 if 'beaconamberlens' in key:return material('Beacon_BlueLens')
 if 'beaconbulbemitter' in key:return material('Beacon_BlueBulb')
 for n in Original:
  if re.sub('[^a-z0-9]','',n.lower()) in key:return material(n)
 mh=re.search(r'mh_?([a-z]+)',slot.lower())
 if mh:
  for cand in ['Red','SafetyYellow','White','FoundryOrange','Graphite','Rubber','DarkSteel','MachinedSteel','Glass']:
   if cand.lower()==mh.group(1):return unreal.load_asset(MH+cand)
 raise RuntimeError('No material for slot '+slot)
report={'assets':[]}
for name in ['Forklift_Body','Forklift_MastOuter','Forklift_MastInner','Forklift_Carriage','Forklift_Wheel_FL','Forklift_Wheel_FR','Forklift_Wheel_RL','Forklift_Wheel_RR']:
 o=unreal.FbxImportUI();o.import_materials=False;o.import_textures=False;o.import_as_skeletal=False;o.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH;o.automated_import_should_detect_type=False
 d=o.static_mesh_import_data;d.combine_meshes=True;d.import_uniform_scale=1.0;d.convert_scene=True;d.convert_scene_unit=True;d.auto_generate_collision=False;d.generate_lightmap_u_vs=True;d.normal_import_method=unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
 # The review FBXs are exported in vehicle space; the game animates each component about its own pivot (wheel spin, mast
 # and carriage lift), so each is moved back by its rest position (manifest, Blender cm; the import mirrors Y).
 px,py,pz=manifest['component_rest_transforms'][name]['position_cm'];d.import_translation=unreal.Vector(-px,py,-pz)
 path=DEST+'/SM_'+name
 if unreal.EditorAssetLibrary.does_asset_exist(path):unreal.EditorAssetLibrary.delete_asset(path)
 t=unreal.AssetImportTask();t.filename=str(SRC/(name+'.fbx'));t.destination_path=DEST;t.destination_name='SM_'+name;t.automated=True;t.replace_existing=True;t.save=False;t.options=o;t.factory=unreal.FbxFactory();tools.import_asset_tasks([t])
 mesh=unreal.load_asset(path);assert mesh,name;slots=[]
 for i,s in enumerate(mesh.get_editor_property('static_materials')):m=slot_material(str(s.material_slot_name));mesh.set_material(i,m);slots.append(m.get_name())
 unreal.EditorAssetLibrary.save_loaded_asset(mesh);b=mesh.get_bounding_box()
 report['assets'].append({'path':path,'min':[round(b.min.x,2),round(b.min.y,2),round(b.min.z,2)],'max':[round(b.max.x,2),round(b.max.y,2),round(b.max.z,2)],'materials':slots})
(ROOT/'Saved/Logs/ForkliftQualityImport.json').write_text(json.dumps(report,indent=1))
