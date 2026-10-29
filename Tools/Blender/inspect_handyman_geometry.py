import bpy,json
from pathlib import Path
from mathutils import Vector
root=Path(r'C:/Users/curly/Documents/Foundryline Game/SourceAssets/Employees/Handyman/Imported')
rows=[]
for f in sorted(root.glob('SK_*.fbx')):
 bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
 bpy.ops.import_scene.fbx(filepath=str(f),use_anim=False)
 meshes=[o for o in bpy.context.scene.objects if o.type=='MESH'];verts=[o.matrix_world@v.co for o in meshes for v in o.data.vertices]
 lo=Vector(tuple(min(v[i] for v in verts) for i in range(3)));hi=Vector(tuple(max(v[i] for v in verts) for i in range(3)))
 row={'name':f.stem,'vertices':len(verts),'minCm':list(lo*100),'maxCm':list(hi*100),'sizeCm':list((hi-lo)*100),'objects':[o.name for o in meshes]}
 if f.stem=='SK_Handyman_modular':
  arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE');row['bones']=[{'name':b.name,'parent':b.parent.name if b.parent else None,'headCm':list((arm.matrix_world@b.head_local)*100),'tailCm':list((arm.matrix_world@b.tail_local)*100)} for b in arm.data.bones]
 rows.append(row)
(root.parent/'GeometryReport.json').write_text(json.dumps(rows,indent=2));print('HANDYMAN_GEOMETRY_REPORT',len(rows))
