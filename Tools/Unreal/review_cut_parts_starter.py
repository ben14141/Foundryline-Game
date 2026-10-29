"""Native inspection of the starter level; leave a fresh paused scene open."""
import unreal,time,json,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
base=Path(unreal.Paths.project_saved_dir());out=base/'Screenshots/Reviews';start=time.monotonic();stage=0;last=0;q={'checks':{},'errors':[]}
def pc():return next(p for w in unreal.EditorLevelLibrary.get_pie_worlds(False) for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller())
def plant():return unreal.GameplayStatics.get_all_actors_of_class(pc().get_world(),unreal.FoundrylineFactoryPlant)[0]
def act(a):unreal.FoundrylineNetworkTestTools.queue_factory_action(pc(),a)
def camera(p,target):
 unreal.FoundrylineNetworkTestTools.setup_workshop_review(pc(),True);c=pc().get_view_target();c.set_actor_location(p,False,False);c.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(p,target),False)
def shot(n):unreal.SystemLibrary.execute_console_command(pc().get_world(),'Shot SHOWUI filename="'+str(out/('CutPartsStarter_'+n+'.png'))+'"')
def tick(dt):
 global stage,last
 try:
  t=time.monotonic()-start
  if stage==0 and t>3:stage=1;last=t;unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
  elif stage==1 and t-last>14:
   q['checks']['starter_profile']=plant().get_zone_count()==0 and 'remaining2' not in json.loads(unreal.FoundrylineNetworkTestTools.equipment_motion_report(pc()));q['checks']['one_plasma']=len(unreal.GameplayStatics.get_all_actors_of_class(pc().get_world(),unreal.FoundrylinePlasmaTable))==1;q['checks']['no_brake_or_roller']=not unreal.GameplayStatics.get_all_actors_of_class(pc().get_world(),unreal.FoundrylinePressBrakeDisplay) and not unreal.GameplayStatics.get_all_actors_of_class(pc().get_world(),unreal.FoundrylineMaterialHandlingCell)
   act('Toggle');camera(unreal.Vector(3900,4900,6500),unreal.Vector(-900,500,0));stage=2;last=t
  elif stage==2 and t-last>4:stage=3;last=t;shot('Overview')
  elif stage==3 and t-last>3:camera(unreal.Vector(-200,2400,2400),unreal.Vector(-1350,800,80));stage=4;last=t
  elif stage==4 and t-last>4:stage=5;last=t;shot('Shop')
  elif stage==5 and t-last>3:camera(unreal.Vector(-3700,2100,1900),unreal.Vector(-2800,900,80));stage=6;last=t
  elif stage==6 and t-last>4:stage=7;last=t;shot('Offices')
  elif stage==7 and t-last>3:camera(unreal.Vector(3900,4900,6500),unreal.Vector(-900,500,0));act('Save');stage=8;last=t
  elif stage==8 and t-last>3:
   r=json.loads(plant().get_factory_report());q['checks']['paused_raw_only']=not r['running'] and r['raw']==10 and plant().get_zone_count()==0;q['checks']['saved']=Path(base/'SaveGames/FoundrylineCutParts01.sav').exists();pawn=pc().get_controlled_pawn();pawn.focus_on_world_position(unreal.Vector(-500,650,0));pc().set_view_target_with_blend(pawn,0);q['completed']=True;(base/'Logs/CutPartsStarterVisualQA.json').write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);return
  if t>150:raise RuntimeError('Starter render timeout '+str(stage))
  (base/'Logs/CutPartsStarterVisualQA.json').write_text(json.dumps({**q,'stage':stage},indent=2))
 except Exception:q['errors'].append(traceback.format_exc());(base/'Logs/CutPartsStarterVisualQA.json').write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
