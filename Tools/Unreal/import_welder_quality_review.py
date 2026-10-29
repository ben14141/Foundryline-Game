"""Reimport Codex's original MIG welder rebuild over the existing SM_Welder_* meshes (same package paths, so no code change).

Source: SourceAssets/Production/Equipment/Welder/QualityReview_2026-10-29 (compatible exports and manifest; geometry and
materials are original procedural work). Scale is 1 Blender metre = 100 Unreal cm. Each FBX material slot gets an original
material M_Original_<slot> in /Game/Foundryline/Shared/Materials/OriginalModels built from the values recorded in the blend
(WeldingMaterials below). The torch keeps its runtime sockets Grip(0,0,0), Support(3,-3,8), WeldTip(23.9,0,13.5): they are
taken from the FBX SOCKET_ empties when present and otherwise added. Welder_Complete_Review replaces SM_Welder_Complete,
which is used only for placement holograms. Writes Saved/Logs/WelderQualityImport.json.
Run: UnrealEditor-Cmd Foundryline.uproject -run=pythonscript -script=<abs path to this file>
"""
import unreal,json,re
from pathlib import Path
ROOT=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
SRC=ROOT/'SourceAssets/Production/Equipment/Welder/QualityReview_2026-10-29';DEST='/Game/Foundryline/Production/Equipment/Welder';MROOT='/Game/Foundryline/Shared/Materials/OriginalModels'
manifest=json.loads((SRC/'manifest.json').read_text())
# Principled values from Foundryline_MIGWelder_QualityReview.blend (colour, metallic, roughness, coat, emission, transmission).
WeldingMaterials={
 'WQ_Brass':([.52,.32,.085],.88,.27,0,None,0),'WQ_BrushedSteel':([.34,.40,.46],.90,.29,0,None,0),'WQ_ClearWindow':([.38,.49,.55],0,.12,0,None,.92),
 'WQ_Copper':([.50,.22,.075],.94,.23,0,None,0),'WQ_DisplayCyan':([.12,.65,.79],0,.25,0,([.08,.55,.75],.8),0),'WQ_GasBlue':([.025,.12,.24],.35,.29,.4,None,0),
 'WQ_Graphite':([.025,.035,.046],.55,.31,0,None,0),'WQ_LabelWhite':([.75,.83,.87],0,.40,0,None,0),'WQ_PowderOrange':([.75,.13,.018],.18,.27,.45,None,0),
 'WQ_Rubber':([.009,.013,.017],0,.70,0,None,0),'WQ_ScreenBlack':([.003,.012,.018],0,.22,0,None,0)}
tools=unreal.AssetToolsHelpers.get_asset_tools();E=unreal.MaterialEditingLibrary;made={}
def constant(m,v,at):n=E.create_material_expression(m,unreal.MaterialExpressionConstant,*at);n.set_editor_property('r',v);return n
def vector(m,v,at):n=E.create_material_expression(m,unreal.MaterialExpressionConstant3Vector,*at);n.constant=unreal.LinearColor(v[0],v[1],v[2],1);return n
def material(name):
 if name in made:return made[name]
 col,metal,rough,coat,emit,transmit=WeldingMaterials[name];slug='M_Original_'+name;path=MROOT+'/'+slug
 m=unreal.load_asset(path) or tools.create_asset(slug,MROOT,unreal.Material,unreal.MaterialFactoryNew());E.delete_all_material_expressions(m)
 if transmit:
  # The wire-feed window: a see-through tinted pane rather than a path-traced transmission.
  m.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT);m.set_editor_property('translucency_lighting_mode',unreal.TranslucencyLightingMode.TLM_SURFACE)
  E.connect_material_property(constant(m,1-transmit*.8,(-400,200)),'',unreal.MaterialProperty.MP_OPACITY)
 c=vector(m,col,(-500,0))
 if coat:
  # Powder-coated orange and the gas bottle: clear coat, wired through material attributes as in Codex's importer.
  m.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_CLEAR_COAT);m.set_editor_property('use_material_attributes',True);a=E.create_material_expression(m,unreal.MaterialExpressionMakeMaterialAttributes,200,0)
  for pin,node in [('BaseColor',c),('Metallic',constant(m,metal,(-400,0))),('Roughness',constant(m,rough,(-400,100))),('ClearCoat',constant(m,coat,(-400,300))),('ClearCoatRoughness',constant(m,.16,(-400,400)))]:E.connect_material_expressions(node,'',a,pin)
  E.connect_material_property(a,'',unreal.MaterialProperty.MP_MATERIAL_ATTRIBUTES)
 else:E.connect_material_property(c,'',unreal.MaterialProperty.MP_BASE_COLOR);E.connect_material_property(constant(m,metal,(-400,0)),'',unreal.MaterialProperty.MP_METALLIC);E.connect_material_property(constant(m,rough,(-400,100)),'',unreal.MaterialProperty.MP_ROUGHNESS)
 if emit:E.connect_material_property(vector(m,[x*emit[1] for x in emit[0]],(-500,500)),'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
 E.set_material_usage(m,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES);E.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m);made[name]=m;return m
def slot_material(slot):
 key=re.sub('[^a-z0-9]','',slot.lower());hits=[n for n in WeldingMaterials if re.sub('[^a-z0-9]','',n.lower()) in key]
 assert len(hits)==1,(slot,hits);return material(hits[0])
report={'source':str(SRC),'assets':[]}
for fbx,name,sockets in [('Welder_Machine.fbx','SM_Welder_Machine',False),('Welder_MIGGun.fbx','SM_Welder_MIGGun',True),('Welder_Leads.fbx','SM_Welder_Leads',False),('Welder_WorkClamp.fbx','SM_Welder_WorkClamp',False),('Welder_Complete_Review.fbx','SM_Welder_Complete',False)]:
 o=unreal.FbxImportUI();o.import_materials=False;o.import_textures=False;o.import_as_skeletal=False;o.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH;o.automated_import_should_detect_type=False
 d=o.static_mesh_import_data;d.combine_meshes=True;d.import_uniform_scale=1.0;d.convert_scene=True;d.convert_scene_unit=True;d.auto_generate_collision=False;d.generate_lightmap_u_vs=True;d.normal_import_method=unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
 # Replace, not reimport: a reimport keeps the old mesh's material slots.
 if unreal.EditorAssetLibrary.does_asset_exist(DEST+'/'+name):unreal.EditorAssetLibrary.delete_asset(DEST+'/'+name)
 t=unreal.AssetImportTask();t.filename=str(SRC/fbx);t.destination_path=DEST;t.destination_name=name;t.automated=True;t.replace_existing=True;t.save=False;t.options=o;t.factory=unreal.FbxFactory();tools.import_asset_tasks([t])
 mesh=unreal.load_asset(DEST+'/'+name);assert mesh,name;slots=[]
 for i,s in enumerate(mesh.get_editor_property('static_materials')):m=slot_material(str(s.material_slot_name));mesh.set_material(i,m);slots.append(m.get_path_name().split('.')[0])
 if name=='SM_Welder_Machine':
  # The station supplies its own collision boxes; the cart keeps one simple box for any physics query.
  unreal.EditorStaticMeshLibrary.remove_collisions(mesh);unreal.EditorStaticMeshLibrary.add_simple_collisions(mesh,unreal.ScriptingCollisionShapeType.BOX)
 found={}
 if sockets:
  for n,loc in [('Grip',(0,0,0)),('Support',(3,-3,8)),('WeldTip',(23.9,0,13.5))]:
   s=mesh.find_socket(n)
   if not s:s=unreal.StaticMeshSocket(mesh);s.set_editor_property('socket_name',n);s.set_editor_property('relative_location',unreal.Vector(*loc));mesh.add_socket(s)
   # The FBX empties arrive at 100x scale and flipped 180 degrees; attached effects need a plain socket at the same point.
   k=mesh.find_socket(n);k.set_editor_property('relative_scale',unreal.Vector(1,1,1));k.set_editor_property('relative_rotation',unreal.Rotator(0,0,0))
   l=k.relative_location;found[n]=[round(l.x,2),round(l.y,2),round(l.z,2)]
 unreal.EditorAssetLibrary.save_loaded_asset(mesh);b=mesh.get_bounding_box()
 report['assets'].append({'path':DEST+'/'+name,'min':[round(b.min.x,2),round(b.min.y,2),round(b.min.z,2)],'max':[round(b.max.x,2),round(b.max.y,2),round(b.max.z,2)],'materials':slots,'sockets':found})
(ROOT/'Saved/Logs/WelderQualityImport.json').write_text(json.dumps(report,indent=1))
