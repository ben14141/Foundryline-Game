"""Restart the route example, exercise real held-key input, leave Play running."""
import unreal,time,json,runpy,traceback
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
assert (root/'Foundryline.uproject').is_file()
already_playing=bool(unreal.EditorLevelLibrary.get_pie_worlds(False))
if not already_playing:runpy.run_path(str(root/'Tools/Unreal/review_route_network.py'))
delay=2 if already_playing else 38
started=time.monotonic();step=0;origin=None;before=None
report={'checks':{},'deltas':{},'errors':[],'completed':False}
output=root/'Saved/Logs/WASDCamera.json'
keys=[('W',1,1),('S',-1,-1),('A',1,-1),('D',-1,1)]
def player():
 for w in unreal.EditorLevelLibrary.get_pie_worlds(False):
  for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController):
   if p.is_local_controller():return p
def write():output.write_text(json.dumps(report,indent=2))
def tick(dt):
 global step,origin,before
 try:
  if time.monotonic()-started<delay+step*1.5:return
  p=player()
  if not p:return
  pawn=p.get_controlled_pawn()
  if origin is None:origin=pawn.get_actor_location()
  if step<8:
   key,sx,sy=keys[step//2]
   if step%2==0:
    before=pawn.get_actor_location();unreal.FoundrylineNetworkTestTools.hold_camera_key(p,key,0.75)
   else:
    delta=pawn.get_actor_location()-before;report['deltas'][key]=[delta.x,delta.y,delta.z];report['checks'][key+'_pans_in_camera_direction']=delta.x*sx>10 and delta.y*sy>10 and abs(delta.z)<0.1
  elif step==8:before=pawn.get_actor_location()
  else:
   delta=pawn.get_actor_location()-before;report['checks']['release_stops_panning']=delta.x*delta.x+delta.y*delta.y+delta.z*delta.z<0.01;report['checks']['logistics_editor_still_active']=p.is_editing_logistics();pawn.set_actor_location(origin,False,False);report['completed']=True;write();unreal.log('FOUNDRYLINE_WASD_DONE '+json.dumps(report));unreal.unregister_slate_post_tick_callback(handle);return
  step+=1;write()
 except Exception:
  report['errors'].append(traceback.format_exc());write();unreal.log_error(report['errors'][-1]);unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick);write()
