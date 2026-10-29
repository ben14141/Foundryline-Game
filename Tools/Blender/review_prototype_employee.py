import bpy,json
from pathlib import Path
r=Path(r'C:/Users/curly/Documents/Foundryline Game/SourceAssets/Employees/Prototype')
bpy.ops.wm.open_mainfile(filepath=str(r/'Foundryline_PrototypeEmployee.blend'))
old=bpy.data.objects['LeftThigh'].rotation_euler.y
exec(bpy.data.texts['ActivatePose.py'].as_string());bpy.context.scene.frame_set(16)
new=bpy.data.objects['LeftThigh'].rotation_euler.y
q={'saved_action_switch_works':abs(new-old)>.1,'idle_thigh_rad':old,'walk_frame16_thigh_rad':new}
(r/'saved_rig_qa.json').write_text(json.dumps(q,indent=2));assert q['saved_action_switch_works'],q
