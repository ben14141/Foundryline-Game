import bpy,json,math
from pathlib import Path
from mathutils import Vector
R=Path(r'C:/Users/curly/Documents/Foundryline Game');rack=R/'SourceAssets/Products/HeadacheRack';bench=R/'SourceAssets/Production/Equipment/WeldingBench'
def bb(obs):
 p=[o.matrix_world@Vector(v) for o in obs if o.type=='MESH' for v in o.bound_box];return [[min(v[i] for v in p) for i in range(3)],[max(v[i] for v in p) for i in range(3)]]
def span(obs):
 a,b=bb(obs);return [(b[i]-a[i])*100 for i in range(3)]
def near(a,b,t=.015):assert all(abs(x-y)<t for x,y in zip(a,b)),(a,b)
q={}
bpy.ops.wm.open_mainfile(filepath=str(bench/'Foundryline_RackWeldingSetup.blend'))
ris=[o for o in bpy.data.objects if o.name.startswith('RiserReplaceablePad')];clamp=[o for o in bpy.data.objects if o.name.startswith('SwivelPressurePad')];tubes=[o for o in bpy.data.objects if o.name.startswith('Upright_') and o.type=='MESH'];feet=[o for o in bpy.data.objects if o.name.startswith('BedRailTopPlate_')];top=[o for o in bpy.data.objects if o.name.startswith('FixtureTop_')]
q['support_top_cm']=bb(ris)[1][2]*100;q['rack_frame_bottom_cm']=bb(tubes)[0][2]*100;q['clamp_bottom_cm']=bb(clamp)[0][2]*100;q['rack_frame_top_cm']=bb(tubes)[1][2]*100;q['feet_clearance_cm']=(bb(feet)[0][2]-bb(top)[1][2])*100
assert abs(q['support_top_cm']-q['rack_frame_bottom_cm'])<.015,q
assert abs(q['clamp_bottom_cm']-q['rack_frame_top_cm'])<.015,q
assert abs(q['feet_clearance_cm']-1.27)<.015,q
q['rack_scale']=list(bpy.data.objects['HeadacheRack_AssemblyDatum'].scale);assert q['rack_scale']==[1,1,1]
q['finite_mesh_vertices']=all(math.isfinite(x) for o in bpy.data.objects if o.type=='MESH' for v in o.data.vertices for x in v.co);assert q['finite_mesh_vertices']
# Compare FBX to evaluated authored geometry, including sub-millimetre weld beads.
bpy.ops.wm.open_mainfile(filepath=str(rack/'Foundryline_HeadacheRack.blend'));root=bpy.data.objects['HeadacheRack_AssemblyDatum'];points=[];dg=bpy.context.evaluated_depsgraph_get()
for o in root.children_recursive:
 if o.type in {'MESH','CURVE','FONT'}:
  e=o.evaluated_get(dg);m=e.to_mesh();points.extend(e.matrix_world@v.co for v in m.vertices);e.to_mesh_clear()
source_size=[(max(v[i] for v in points)-min(v[i] for v in points))*100 for i in range(3)];q['rack_evaluated_source_bounds_cm']=source_size;near(source_size,[198.12,27.94,81.28],.2)
# Full detailed bench export includes converted cable and lettering too.
bpy.ops.wm.open_mainfile(filepath=str(bench/'Foundryline_StandardWeldingBench.blend'));root=bpy.data.objects['StandardWeldingBench_FloorDatum'];bpy.ops.object.select_all(action='DESELECT')
for o in [root]+list(root.children_recursive):o.select_set(True)
bpy.context.view_layer.objects.active=next(o for o in root.children_recursive if o.type=='MESH')
bpy.ops.export_scene.fbx(filepath=str(bench/'StandardWeldingBench_SeparateComponents.fbx'),use_selection=True,object_types={'MESH','EMPTY','OTHER'},apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',bake_anim=False,use_custom_props=True)
for label,path,expected in [('rack',rack/'HeadacheRack_AllSeparateParts.fbx',[198.12,27.94,81.28]),('bench',bench/'StandardWeldingBench_SeparateComponents.fbx',None)]:
 bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False);bpy.ops.import_scene.fbx(filepath=str(path));obs=[o for o in bpy.context.scene.objects if o.type=='MESH'];actual=span(obs if label=='rack' else [o for o in obs if o.name.startswith('FixtureTop_')]);q[label+'_fbx_bounds_cm']=actual
 near(actual,source_size if expected else [243.84,121.92,1.27])
q['result']='PASS';(bench/'export_contact_qa.json').write_text(json.dumps(q,indent=2));print('RACK_BENCH_EXPORT_CONTACT_QA',json.dumps(q))
