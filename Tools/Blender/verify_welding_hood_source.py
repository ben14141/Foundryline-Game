"""Original-only hood provenance, geometry dimensions and FBX serialization checks."""
import bpy,json,sys
from pathlib import Path
from mathutils import Vector
R=Path(__file__).resolve().parents[2];O=R/'SourceAssets/Employees/Equipment/WeldingHood';O=O/'V2Draft' if '--draft' in sys.argv else O;bpy.ops.wm.open_mainfile(filepath=str(O/'WeldingHood_Original.blend'));S=bpy.context.scene;S.frame_set(1)
checks={'no_external_images':not any(i.source=='FILE' for i in bpy.data.images),'no_linked_libraries':not bpy.data.libraries,'no_vendor_armature':not any(o.type=='ARMATURE' for o in bpy.data.objects),'two_independent_roots':all(bpy.data.objects.get(n) and bpy.data.objects[n].type=='EMPTY' for n in ['WeldingHood_Harness','WeldingHood_Visor'])}
dg=bpy.context.evaluated_depsgraph_get();points=[]
for o in bpy.data.objects:
 if o.type!='MESH':continue
 e=o.evaluated_get(dg);mesh=e.to_mesh();points.extend(e.matrix_world@v.co for v in mesh.vertices);e.to_mesh_clear()
lo=[min(p[i] for p in points)*100 for i in range(3)];hi=[max(p[i] for p in points)*100 for i in range(3)];size=[b-a for a,b in zip(lo,hi)];v=bpy.data.objects['WeldingHood_Visor'];S.frame_set(30);checks['lift_action_105degrees']=abs(v.rotation_euler.x+1.8325957145940461)<1e-5
original_count=len([o for o in bpy.data.objects if o.type=='MESH']);triangles=0
for o in bpy.data.objects:
 if o.type=='MESH':
  e=o.evaluated_get(dg);m=e.to_mesh();m.calc_loop_triangles();triangles+=len(m.loop_triangles);e.to_mesh_clear()
bpy.ops.wm.read_factory_settings(use_empty=True)
for n in ['WeldingHood_Harness.fbx','WeldingHood_Visor.fbx']:bpy.ops.import_scene.fbx(filepath=str(O/n),use_anim=False)
pts=[o.matrix_world@v.co for o in bpy.data.objects if o.type=='MESH' for v in o.data.vertices];sizes=[(max(p[i] for p in pts)-min(p[i] for p in pts))*100 for i in range(3)];checks['fbx_roundtrip_fullscale']=all(abs(a-b)<.05 for a,b in zip(size,sizes));checks['fbx_mesh_count']=len([o for o in bpy.data.objects if o.type=='MESH'])==original_count
assert all(checks.values()),(checks,size,sizes)
report={'checks':checks,'closed_neutral_bounds_cm':{'min':lo,'max':hi,'size':size},'closed_size_inches':[x/2.54 for x in size],'original_mesh_parts':original_count,'evaluated_triangles':triangles,'passed':True,'no_engine_use':True};(O/'source_qa.json').write_text(json.dumps(report,indent=2));m=json.loads((O/'manifest.json').read_text());m['bounds_cm']=report['closed_neutral_bounds_cm'];m['dimensions_inches']=report['closed_size_inches'];(O/'manifest.json').write_text(json.dumps(m,indent=2));print('WELDING_HOOD_SOURCE_QA',len(checks),'PASS',size)
