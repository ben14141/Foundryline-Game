"""Read-only prefab sidewall component geometry probe, run in background Blender."""
from pathlib import Path
import bpy,bmesh,json
root=Path(__file__).resolve().parents[2]
exec((root/'Tools/Blender/convert_starter_flatbed.py').read_text().split('# Separate the loading-side')[0])
report={}
for name in ['Cube.069','Cube.070']:
 o=bpy.data.objects[name];bm=bmesh.new();bm.from_mesh(o.data);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=.00001);bm.verts.ensure_lookup_table();seen=set();rows=[]
 for v in bm.verts:
  if v.index in seen:continue
  stack=[v];vs=[]
  while stack:
   a=stack.pop()
   if a.index in seen:continue
   seen.add(a.index);vs.append(a.co.copy());stack.extend(e.other_vert(a) for e in a.link_edges)
  rows.append({'vertices':len(vs),'min_cm':[min(v[i] for v in vs)*100 for i in range(3)],'max_cm':[max(v[i] for v in vs)*100 for i in range(3)]})
 report[name]=rows;bm.free()
(root/'Saved/Logs/StarterSidewallComponents.json').write_text(json.dumps(report,indent=2))
print('STARTER_SIDEWALL_PROBE_READY')
