import bpy,bmesh,json
from pathlib import Path
r=Path(r'C:/Users/curly/Documents/Foundryline Game');bpy.ops.wm.open_mainfile(filepath=str(r/'SourceAssets/MaterialHandling/Authored/Foundryline_MaterialHandling.blend'));o=bpy.data.objects['Forklift_Body'];bm=bmesh.new();bm.from_mesh(o.data);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=.0001);bm.verts.ensure_lookup_table();seen=set();out=[]
for v in bm.verts:
 if v.index in seen:continue
 stack=[v];vs=[]
 while stack:
  p=stack.pop()
  if p.index in seen:continue
  seen.add(p.index);vs.append(p.co.copy());stack.extend(e.other_vert(p) for e in p.link_edges)
 lo=[min(p[i] for p in vs)*100 for i in range(3)];hi=[max(p[i] for p in vs)*100 for i in range(3)]
 if hi[2]>105 and lo[2]<200 and lo[0]<0 and hi[0]>-200 and lo[1]<15 and hi[1]>-15:out.append({'lo':lo,'hi':hi,'n':len(vs)})
(r/'Saved/Logs/ForkliftSeatConnectedParts.json').write_text(json.dumps(out,indent=2))
