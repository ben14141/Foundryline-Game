"""Four co-op peers plus late join; actual stocked cycles and pause, no save writes."""
import unreal,time,json,traceback
from pathlib import Path
T=unreal.FoundrylineNetworkTestTools;unreal.EditorPythonScripting.set_keep_python_script_alive(True)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/Foundryline/Maps/L_FlatForest_Grass')
base=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()));log=base/'Logs/EquipmentQualityNetworkQA.json';start=time.monotonic();stage=0;last=0;host=None;before=[];frozen=[];cameras=[];seen={'plasma':set(),'brake':set(),'roller':set()};q={'checks':{},'errors':[]}
def players():return [p for w in unreal.EditorLevelLibrary.get_pie_worlds(False) for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller()]
def reports():return [json.loads(T.equipment_motion_report(p)) for p in players()]
def record():
 for i,r in enumerate(reports()):
  if r.get('cutting') and r.get('sparks',0)>0:seen['plasma'].add(i)
  if abs(r.get('ram',0))>5:seen['brake'].add(i)
  if r.get('forming_visible') and abs(r.get('roller',0))>5:seen['roller'].add(i)
def finish():
 q['completed']=True;q['reports']=reports();log.write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(h);unreal.EditorPythonScripting.set_keep_python_script_alive(False)
def tick(dt):
 global stage,last,host,before,frozen
 try:
  t=time.monotonic()-start
  if t>160:raise RuntimeError('Equipment network timeout '+str(stage))
  if 2<=stage<=6:record()
  if stage==0 and t>3:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1;last=t
  elif stage==1 and t-last>16:
   q['checks']['four_initial_players']=len(players())==4;host=next(p for p in players() if p.has_authority())
   for p in players():
    old=unreal.GameplayStatics.get_player_camera_manager(p.get_world(),0).get_camera_location();q.setdefault('initial_view_positions',[]).append([old.x,old.y,old.z]);camera=T.create_review_camera(p);camera.set_actor_location(unreal.Vector(-700,800,350),False,False);p.set_view_target_with_blend(camera,0);cameras.append(camera)
   q['checks']['real_three_machine_stock_cycle']=T.start_equipment_exercise(host);stage=2;last=t
  elif stage==2 and t-last>3:before=reports();stage=3;last=t
  elif stage==3 and t-last>.8:
   now=reports();q['checks']['plasma_travels_all_peers']=all(sum((a-b)**2 for a,b in zip(r['torch'],old['torch']))>.1 for r,old in zip(now,before));T.queue_factory_action(host,'Run');stage=4;last=t
  elif stage==4 and t-last>1:frozen=reports();stage=5;last=t
  elif stage==5 and t-last>1:
   now=reports();q['checks']['pause_freezes_all_mechanics_and_arc']=all(not r['running'] and not r['cutting'] and abs(r['ram']-old['ram'])<.01 and abs(r['roller']-old['roller'])<.01 and sum((a-b)**2 for a,b in zip(r['torch'],old['torch']))<.01 for r,old in zip(now,frozen));T.queue_factory_action(host,'Run');stage=6;last=t
  elif stage==6 and t-last>5 and all(all(r.get('remaining'+str(i),1)==0 for i in range(1,4)) for r in reports()):
   q['checks']['all_real_cycles_complete']=True
   for k,v in seen.items():q['checks'][k+'_motion_observed_every_peer']=len(v)==4
   T.queue_factory_action(host,'Run');q['checks']['late_join_requested']=T.request_late_join();stage=7;last=t
  elif stage==7 and t-last>14 and len(players())==5:
   rs=reports();q['checks']['five_players_present']=True;q['checks']['late_join_receives_completed_machine_state']=all(all(r.get('remaining'+str(i),1)==0 for i in range(1,4)) for r in rs);q['checks']['late_join_pose_matches']=all(abs(r['ram']-rs[0]['ram'])<.01 and abs(r['roller']-rs[0]['roller'])<.01 and sum((a-b)**2 for a,b in zip(r['torch'],rs[0]['torch']))<.01 for r in rs);finish();return
  q['stage']=stage;log.write_text(json.dumps(q,indent=2))
 except Exception:q['errors'].append(traceback.format_exc());finish()
h=unreal.register_slate_post_tick_callback(tick)
