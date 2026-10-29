import bpy,json,math
from pathlib import Path
from mathutils import Vector
root=Path(r'C:\Users\curly\Documents\Foundryline Game');src=root/'SourceAssets/AssemblyReviews/TankerTractor'
bpy.ops.wm.open_mainfile(filepath=str(src/'Foundryline_Tanker_Tractor.blend'))
checks={}
for frame,name in [(1,'Detached / legs down'),(100,'Kingpin aligned'),(145,'Landing gear raised'),(230,'Towed forward'),(285,'Legs supporting trailer'),(300,'Tractor departs')]:bpy.context.scene.timeline_markers.new(name,frame=frame)
scene=bpy.context.scene
scene.frame_set(1);bpy.context.view_layer.update()
legs=[bpy.data.objects['Tanker_Leg_'+str(i)] for i in range(2)]
checks['feet_touch_ground_detached']=all(abs(min((o.matrix_world@v.co).z for v in o.data.vertices))<.01 for o in legs)
checks['ten_separate_road_wheels']=len([o for o in bpy.data.objects if '_Wheel_' in o.name])==10
scene.frame_set(150);bpy.context.view_layer.update()
checks['landing_feet_clear_ground_coupled']=all(min((o.matrix_world@v.co).z for v in o.data.vertices)>.7 for o in legs)
checks['hitches_align']= (bpy.data.objects['Kingpin'].matrix_world.translation-bpy.data.objects['FifthWheel'].matrix_world.translation).length<.001
wheel=bpy.data.objects['Tractor_Wheel_0'];scene.frame_set(1);a=wheel.rotation_euler.y;scene.frame_set(100);b=wheel.rotation_euler.y
checks['wheels_rotate_from_travel']=abs(b-a)>math.tau
scene.frame_set(1);bpy.ops.wm.save_as_mainfile(filepath=str(src/'Foundryline_Tanker_Tractor.blend'))
(root/'Saved/Logs/TankerBlenderQA.json').write_text(json.dumps(checks,indent=2));print(checks)
assert all(checks.values())
