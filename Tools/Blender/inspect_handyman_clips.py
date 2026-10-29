import bpy,json
from pathlib import Path
root=Path(r'C:/Users/curly/Documents/Foundryline Game/SourceAssets/Employees/Handyman/Imported');rows={}
for name in ['Walk','WalkInPlace','Idle']:
 bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False);bpy.ops.import_scene.fbx(filepath=str(root/(name+'.fbx')),use_anim=True)
 arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE');act=arm.animation_data.action;fr=act.frame_range;frames=[]
 for f in range(int(fr[0]),int(fr[1])+1):
  bpy.context.scene.frame_set(f);row={'frame':f}
  for b in ['pelvis','foot_l','foot_r','hand_l','hand_r']:
   row[b]=list((arm.matrix_world@arm.pose.bones[b].matrix.translation)*100)
  frames.append(row)
 rows[name]={'range':list(fr),'fps':bpy.context.scene.render.fps,'samples':frames}
(root.parent/'ClipGeometryReport.json').write_text(json.dumps(rows,indent=2))
