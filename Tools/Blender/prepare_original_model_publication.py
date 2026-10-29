import bpy,json,hashlib,importlib.util
from pathlib import Path
from mathutils import Vector
ROOT=Path(r'C:/Users/curly/Documents/Foundryline Game');OUT=ROOT/'SourceAssets/PublishedOriginalModels';spec=importlib.util.spec_from_file_location('h',ROOT/'Tools/Blender/build_rack_welding_bench.py');h=importlib.util.module_from_spec(spec);spec.loader.exec_module(h)
A=[('HeadacheRack','SourceAssets/ArtPolish/2026-10-29/HeadacheRack/HeadacheRack_ArtPolish.blend','HeadacheRack_AssemblyDatum','Products/HeadacheRack','SM_HeadacheRack'),('StandardWeldingBench','SourceAssets/ArtPolish/2026-10-29/WeldingBench/WeldingBench_ArtPolish.blend','StandardWeldingBench_FloorDatum','Production/Equipment/WeldingBench','SM_StandardWeldingBench'),('CurvedSheetCradle','SourceAssets/ArtPolish/2026-10-29/CurvedSheetCradle/CurvedSheetCradle_ArtPolish.blend','CurvedSheetCradle_FloorCentre','Inventory/Storage/CurvedSheetCradle','SM_CurvedSheetCradle'),('WeldingFixtureKit','SourceAssets/Production/Equipment/WeldingBench/Foundryline_WeldingFixtureKit.blend','WeldingFixtureKit_FloorDatum','Production/Equipment/WeldingBench','SM_WeldingFixtureKit')]
rows=[]
for name,source,rootname,dest,asset in A:
 bpy.ops.wm.open_mainfile(filepath=str(ROOT/source));root=bpy.data.objects[rootname];folder=OUT/name;folder.mkdir(exist_ok=True)
 if name=='StandardWeldingBench':
  lead=bpy.data.objects['GroundLead'];points=[(-1.1176,-.48,.80),(-1.16,-.45,.65),(-1.18,-.30,.35),(-1.16,.20,.35),(-1.12,.25,.60)]
  for p,co in zip(lead.data.splines[0].bezier_points,points):p.co=co;p.handle_left_type='AUTO';p.handle_right_type='AUTO'
  bpy.data.objects['GroundQuickConnector'].location=(-1.12,.25,.60)
 assert not bpy.data.libraries
 assert not [i.filepath for i in bpy.data.images if i.source=='FILE'],[(i.name,i.filepath) for i in bpy.data.images if i.source=='FILE']
 keep={root,*root.children_recursive}
 for o in list(bpy.data.objects):
  if o not in keep:bpy.data.objects.remove(o,do_unlink=True)
 bpy.data.orphans_purge(do_local_ids=True,do_linked_ids=True,do_recursive=True);bpy.context.view_layer.update();materials={};points=[];dg=bpy.context.evaluated_depsgraph_get();names=[]
 for o in root.children_recursive:
  if o.type in {'MESH','CURVE','FONT'}:
   names.append(o.name);e=o.evaluated_get(dg);m=e.to_mesh();points.extend(e.matrix_world@v.co for v in m.vertices);e.to_mesh_clear()
   for mat in o.data.materials:
    if not mat:continue
    assert not any(n.type=='TEX_IMAGE' for n in mat.node_tree.nodes),mat.name
    bs=mat.node_tree.nodes.get('Principled BSDF');materials[mat.name]={'color':list(bs.inputs['Base Color'].default_value[:3]),'metallic':bs.inputs['Metallic'].default_value,'roughness':bs.inputs['Roughness'].default_value,'coat':bs.inputs['Coat Weight'].default_value,'emission':list(bs.inputs['Emission Color'].default_value[:3]),'emission_strength':bs.inputs['Emission Strength'].default_value}
 lo=[min(v[i] for v in points)*100 for i in range(3)];hi=[max(v[i] for v in points)*100 for i in range(3)];size=[b-a for a,b in zip(lo,hi)]
 if name=='StandardWeldingBench':assert abs(size[0]-243.84)<.03 and abs(size[1]-121.92)<.03,(lo,hi,size)
 if name=='WeldingFixtureKit':assert not any(n.startswith(('Leg_','FixtureTop_','Upright_')) for n in names)
 else:assert not any(n.startswith('REVIEW_') for n in names)
 bpy.context.scene.unit_settings.scale_length=1.0;fbx=folder/(name+'.fbx');blend=folder/(name+'.blend');h.export(fbx,h.descendants(root));bpy.ops.wm.save_as_mainfile(filepath=str(blend));rows.append({'id':name,'destination':'/Game/Foundryline/'+dest,'asset_name':asset,'fbx':str(fbx.relative_to(ROOT)),'blend':str(blend.relative_to(ROOT)),'root_name':rootname,'bounds_cm':{'min':lo,'max':hi,'size':size},'materials':materials,'mesh_objects':names,'provenance':'Original procedural geometry/materials; no external images, linked libraries or vendor geometry','no_external_images':True,'bench_cable_stowed_within_table_footprint':name=='StandardWeldingBench'})
(OUT/'manifest.json').write_text(json.dumps({'assets':rows,'source_scope':'Native publication of original standalone models only; no licensed tanker content'},indent=2));(ROOT/'Saved/Logs/OriginalModelSourceAudit.json').write_text(json.dumps(rows,indent=2));print('ORIGINAL_MODEL_PUBLICATION_SOURCES_READY',json.dumps([{'id':r['id'],'bounds':r['bounds_cm']} for r in rows]))
