"""Import authored fabrication parts/stages and prepare actual construction-progress shaders."""
import unreal,json
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve();tools=unreal.AssetToolsHelpers.get_asset_tools();edit=unreal.MaterialEditingLibrary
materials={}
for n in ['CarbonSteel','MachinedSteel','DarkSteel','Graphite','FoundryOrange','Rubber','White','Red']:
 materials['mh_'+n.lower()]=unreal.load_asset('/Game/Foundryline/Shared/Materials/MaterialHandling/M_MH_'+n)
palette=unreal.load_asset('/Game/Foundryline/Vehicles/MegapolyTrucks/Materials/M_MegapolyMain')
details={n.lower():unreal.load_asset('/Game/Foundryline/Vehicles/Shared/Materials/M_'+n) for n in ['DetailRubber','DetailMetal','DetailRed','DetailWhite']}
report=[]
for folder,name in [('PlasmaTable','T_PlasmaTable_CNC'),('PressBrake','T_PressBrake_CNC'),('SheetRoller','T_SheetRoller_CNC')]:
 t=unreal.AssetImportTask();t.filename=str(root/'SourceAssets/Production/Equipment'/folder/(name+'.png'));t.destination_path='/Game/Foundryline/Production/Equipment/'+folder+'/Materials';t.destination_name=name;t.automated=True;t.replace_existing=True;t.save=True;tools.import_asset_tasks([t])
for folder,destination in [('Products/Components/Fabrication','Products/Components/Fabrication'),('Products/Components/DishedHeads','Products/Components/DishedHeads'),('Products/Tankers/StraightBarrel/ProductionStages','Products/Tankers/StraightBarrel/ProductionStages')]:
 src=root/'SourceAssets'/folder;data=json.loads((src/'manifest.json').read_text())
 for a in data['assets']:
  name=a['name'];o=unreal.FbxImportUI();o.import_materials=False;o.import_textures=False;o.import_as_skeletal=False;o.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH;o.automated_import_should_detect_type=False
  d=o.static_mesh_import_data;d.combine_meshes=True;d.auto_generate_collision=False;d.convert_scene=True;d.convert_scene_unit=True;d.generate_lightmap_u_vs=True
  target='Products/Tankers/StraightBarrel' if name=='Tanker_Body' else destination
  t=unreal.AssetImportTask();t.filename=str(src/(name+'.fbx'));t.destination_path='/Game/Foundryline/'+target;t.destination_name='SM_'+name;t.automated=True;t.replace_existing=True;t.save=True;t.options=o;t.factory=unreal.FbxFactory();tools.import_asset_tasks([t]);m=unreal.load_asset(t.destination_path+'/'+t.destination_name);assert m,name
  for i,s in enumerate(m.get_editor_property('static_materials')):
   key=str(s.material_slot_name).lower();mat=next((v for k,v in {**materials,**details}.items() if k in key),palette);assert mat,key;m.set_material(i,mat)
  b=unreal.EditorStaticMeshLibrary.get_lod_build_settings(m,0);b.recompute_tangents=True;b.use_mikk_t_space=False;unreal.EditorStaticMeshLibrary.set_lod_build_settings(m,0,b);unreal.EditorAssetLibrary.save_loaded_asset(m)
  if 'dimensions_cm' in a:
   size=m.get_bounds().box_extent*2;assert all(abs(x-y)<.3 for x,y in zip(size.to_tuple(),a['dimensions_cm'])),name
  report.append(m.get_path_name())
dest='/Game/Foundryline/Production/Materials'
def node(m,cls):return edit.create_material_expression(m,cls)
def scalar(m,name,value):
 n=node(m,unreal.MaterialExpressionScalarParameter);n.set_editor_property('parameter_name',name);n.set_editor_property('default_value',value);return n
def vector(m,name,value):
 n=node(m,unreal.MaterialExpressionVectorParameter);n.set_editor_property('parameter_name',name);n.set_editor_property('default_value',unreal.LinearColor(*value));return n
def const(m,x):n=node(m,unreal.MaterialExpressionConstant);n.set_editor_property('r',x);return n
def binary(m,cls,a,b):
 n=node(m,cls);assert edit.connect_material_expressions(a,'',n,'A');assert edit.connect_material_expressions(b,'',n,'B');return n
def material(name):
 m=unreal.load_asset(dest+'/'+name) or tools.create_asset(name,dest,unreal.Material,unreal.MaterialFactoryNew());edit.delete_all_material_expressions(m);return m
def finish(m):
 edit.set_material_usage(m,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES);edit.recompile_material(m);assert unreal.EditorAssetLibrary.save_loaded_asset(m)
def reveal(m):
 w=node(m,unreal.MaterialExpressionWorldPosition);origin=vector(m,'BuildOrigin',(0,0,0,1));axis=vector(m,'BuildAxis',(0,0,1,1));local=binary(m,unreal.MaterialExpressionSubtract,w,origin);dot=binary(m,unreal.MaterialExpressionDotProduct,local,axis)
 low=scalar(m,'BuildLow',0);high=scalar(m,'BuildHigh',400);q=binary(m,unreal.MaterialExpressionDivide,binary(m,unreal.MaterialExpressionSubtract,dot,low),binary(m,unreal.MaterialExpressionSubtract,high,low));fraction=scalar(m,'BuildFraction',0)
 # A binary reveal without If-node pin-name differences between engine versions.
 delta=binary(m,unreal.MaterialExpressionSubtract,fraction,q);ceil=node(m,unreal.MaterialExpressionCeil);assert edit.connect_material_expressions(delta,'',ceil,'')
 mask=node(m,unreal.MaterialExpressionSaturate);assert edit.connect_material_expressions(ceil,'',mask,'')
 return mask
texture=unreal.load_asset('/Game/Foundryline/Vehicles/MegapolyTrucks/Textures/T_MegapolyGradient');assert texture
floor=material('M_FactoryFloorPaint');floor.set_editor_property('blend_mode',unreal.BlendMode.BLEND_OPAQUE);edit.connect_material_property(vector(floor,'PaintColor',(1,.6,.04,1)),'',unreal.MaterialProperty.MP_BASE_COLOR);edit.connect_material_property(const(floor,.85),'',unreal.MaterialProperty.MP_ROUGHNESS);finish(floor)
for suffix,color in [('Palette',None),('Metal',(.34,.39,.44,1)),('Rubber',(.025,.03,.035,1)),('Red',(.65,.02,.012,1)),('White',(.85,.88,.84,1))]:
 m=material('M_FactoryBuild'+suffix);m.set_editor_property('blend_mode',unreal.BlendMode.BLEND_MASKED)
 if color is None:
  n=node(m,unreal.MaterialExpressionTextureSample);n.texture=texture;edit.connect_material_property(n,'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
 else:edit.connect_material_property(vector(m,'BaseTint',color),'',unreal.MaterialProperty.MP_BASE_COLOR)
 edit.connect_material_property(reveal(m),'',unreal.MaterialProperty.MP_OPACITY_MASK);edit.connect_material_property(const(m,.55),'',unreal.MaterialProperty.MP_ROUGHNESS);finish(m)
for name,clipped in [('M_FactoryHologram',True),('M_FactoryPreview',False)]:
 m=material(name);m.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT);m.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT);m.set_editor_property('two_sided',True)
 color=vector(m,'GlowColor',(.025,.48,1,1));power=scalar(m,'Strength',7000);edit.connect_material_property(binary(m,unreal.MaterialExpressionMultiply,color,power),'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
 opacity=const(m,.22)
 if clipped:
  inv=binary(m,unreal.MaterialExpressionSubtract,const(m,1),reveal(m));opacity=binary(m,unreal.MaterialExpressionMultiply,inv,opacity)
 edit.connect_material_property(opacity,'',unreal.MaterialProperty.MP_OPACITY);finish(m)
# Existing flexible machine cables need the spline usage permutation too.
rubber=materials['mh_rubber'];rubber.set_editor_property('used_with_spline_meshes',True);edit.recompile_material(rubber);unreal.EditorAssetLibrary.save_loaded_asset(rubber)
(root/'Saved/Logs/FactoryAssets.json').write_text(json.dumps({'assets':report,'materials':dest},indent=2));unreal.log('FACTORY_LOOP_ASSETS_READY')
