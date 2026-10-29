"""Temporary stem-only collision overlay for native visual alignment review."""
import unreal,time,json,traceback
from pathlib import Path
T=unreal.FoundrylineNetworkTestTools
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
out=root/'Saved/Logs/TreeCollisionVisualQA.json';q={'errors':[],'completed':False};stage=0;last=0;start=time.monotonic();camera=None;old=None
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
def pc():return next(p for w in unreal.EditorLevelLibrary.get_pie_worlds(False) for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller())
def finish():
 if old:pc().set_view_target_with_blend(old,0)
 if camera:camera.destroy_actor()
 q['completed']=True;out.write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle)
def tick(dt):
 global stage,last,camera,old
 try:
  t=time.monotonic()-start
  if t>30:raise RuntimeError('Tree review timeout')
  if stage==0:
   p=pc();caps=[c for a in unreal.GameplayStatics.get_all_actors_of_class(p.get_world(),unreal.Actor) for c in a.get_components_by_class(unreal.CapsuleComponent) if 'FoundrylineTreeTrunk' in [str(x) for x in c.component_tags]];caps.sort(key=lambda c:(c.get_world_location()-p.get_controlled_pawn().get_actor_location()).length());c=caps[0];center=c.get_world_location();half=c.get_scaled_capsule_half_height();radius=c.get_scaled_capsule_radius();base=center-unreal.Vector(0,0,half);q.update({'count':len(caps),'center':[center.x,center.y,center.z],'radius':radius,'half_height':half});old=p.get_view_target();camera=T.create_review_camera(p);eye=base+unreal.Vector(160,-190,150);camera.set_actor_location(eye,False,False);camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(eye,base+unreal.Vector(0,0,80)),False);camera.get_components_by_class(unreal.CameraComponent)[0].set_field_of_view(65);p.set_view_target_with_blend(camera,0);unreal.SystemLibrary.draw_debug_capsule(p.get_world(),center,half,radius,unreal.Rotator(0,0,0),unreal.LinearColor(0,.8,1,1),8,1.2);stage=1;last=t
  elif stage==1 and t-last>2:unreal.SystemLibrary.execute_console_command(pc().get_world(),'Shot filename="'+str(root/'Saved/Screenshots/Reviews/TreeTrunk_CollisionAlignment.png')+'"');stage=2;last=t
  elif stage==2 and t-last>2:finish()
 except Exception:q['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
