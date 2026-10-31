import bpy,json
from pathlib import Path
from mathutils import Vector
P=Path(r'C:/Users/curly/Documents/Foundryline Game/SourceAssets/Production/Equipment/WeldingBench')
q={}
for label,file,rootname in [('bench','Foundryline_StandardWeldingBench.blend','StandardWeldingBench_FloorDatum'),('fixtures','Foundryline_WeldingFixtureKit.blend','WeldingFixtureKit_FloorDatum')]:
 bpy.ops.wm.open_mainfile(filepath=str(P/file));root=bpy.data.objects[rootname];children=list(root.children_recursive);names=[o.name for o in children];q[label+'_object_count']=len(children);assert all(v==0 for v in root.location);assert tuple(root.scale)==(1,1,1)
 assert not any('Rack' in n or 'Upright_' in n or 'Louver_' in n for n in names),names
 if label=='bench':
  forbidden=['FixtureRiser','StoredFixtureRiser','RiserReplaceablePad','HoldDown','SwivelPressurePad','ScrewTHandle'];assert not any(any(n.startswith(f) for f in forbidden) for n in names);q['bench_no_product_or_removable_fixture']=True
  for name,metal,rough in [('Bench gloss orange',0,.20),('Bench gloss graphite',0,.20),('Bench polished steel',1,.22)]:
   bs=bpy.data.materials[name].node_tree.nodes.get('Principled BSDF');assert abs(bs.inputs['Metallic'].default_value-metal)<.0001;assert abs(bs.inputs['Roughness'].default_value-rough)<.0001;q[name]={'metallic':bs.inputs['Metallic'].default_value,'roughness':bs.inputs['Roughness'].default_value,'coat':bs.inputs['Coat Weight'].default_value}
 else:assert not any(n.startswith(('Leg_','UpperLongitudinal','FixtureTop','LowerToolShelf')) for n in names);q['fixture_kit_no_table_or_product']=True
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False);bpy.ops.import_scene.fbx(filepath=str(P/'StandardWeldingBench_SeparateComponents.fbx'));tops=[o for o in bpy.context.scene.objects if o.name.startswith('FixtureTop_')];assert len(tops)==1
points=[tops[0].matrix_world@Vector(v) for v in tops[0].bound_box];sizes=[(max(v[i] for v in points)-min(v[i] for v in points))*100 for i in range(3)];assert all(abs(a-b)<.015 for a,b in zip(sizes,[243.84,121.92,1.27]));assert abs(max(v.z for v in points)*100-91.44)<.015;q['fbx_top_cm']=sizes;q['height_cm']=max(v.z for v in points)*100
assert not any(o.name.startswith(('HoldDown','StoredFixtureRiser','Upright_','ProtectiveLouver_')) for o in bpy.context.scene.objects);q['result']='PASS';(P/'independent_assets_qa.json').write_text(json.dumps(q,indent=2));manifest=json.loads((P/'dimensions.json').read_text());manifest.update({'independent_asset':True,'contains_product':False,'contains_removable_fixture':False,'finish':'Gloss orange/graphite painted steel frame; polished bare steel tabletop','fixture_kit_source':'Foundryline_WeldingFixtureKit.blend','fixture_kit_export':'WeldingFixtureKit_SeparateComponents.fbx'});(P/'dimensions.json').write_text(json.dumps(manifest,indent=2));print('WELDING_BENCH_INDEPENDENCE_QA',json.dumps(q))
