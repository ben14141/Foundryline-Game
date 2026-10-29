"""Four-peer visual/authority acceptance plus late join, without saving the level."""
import unreal,time,json,traceback
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve();report=root/'Saved/Logs/PlasmaNetworkQA.json'
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
# The launcher selects four-player listen-server PIE in project-local saved settings.
start=time.monotonic();stage=0;stage_start=start;seen=set();first={};q={'checks':{},'errors':[],'completed':False}
def tables():return [a for w in unreal.EditorLevelLibrary.get_pie_worlds(False) for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.FoundrylinePlasmaTable)]
def cameras():
 for w in unreal.EditorLevelLibrary.get_pie_worlds(False):
  for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController):
   if p.is_local_controller():unreal.FoundrylineNetworkTestTools.setup_plasma_review(p,True)
def finish():
 q['completed']=True;report.write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle)
 unreal.SystemLibrary.execute_console_command(unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world(),'QUIT_EDITOR')
def tick(dt):
 global stage,stage_start
 try:
  now=time.monotonic();ts=tables();elapsed=now-stage_start
  if now-start>115:raise RuntimeError('Plasma network acceptance timed out at stage '+str(stage))
  if stage==0 and now-start>3:
   unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1;stage_start=now
  elif stage==1 and len(ts)==4 and elapsed>8:
   cameras();q['checks']['four_peers']=True
   for a in ts:first[a.get_path_name()]=a.get_torch_tip()
   clients=[a for a in ts if not a.has_authority()]
   for a in clients:a.set_operating(False)
   q['checks']['clients_cannot_stop_machine']=all(a.is_operating() for a in ts)
   stage=2;stage_start=now
  elif stage==2:
   states=[a.is_cutting() for a in ts]
   if states and len(set(states))==1:
    seen.add(states[0]);effects=[a.plasma_effect for a in ts]
    q['checks']['spark_budget_bounded']=q['checks'].get('spark_budget_bounded',True) and all(e.get_visible_spark_count()<=256 for e in effects)
    if states[0] and all(e.get_visible_spark_count()>0 for e in effects):q['checks']['live_plasma_on_all_peers']=True
    if not states[0] and all(e.get_visible_spark_count()==0 for e in effects):q['checks']['travel_turns_effect_off']=True
    positions=[a.get_torch_tip() for a in ts]
    spread=max((p-positions[0]).length() for p in positions)
    q['max_peer_tip_difference_cm']=max(q.get('max_peer_tip_difference_cm',0),spread)
   if elapsed>34:
    q['checks']['cutting_and_travel_observed']=seen=={False,True}
    q['checks']['torch_actually_moves']=all((a.get_torch_tip()-first[a.get_path_name()]).length()>2 for a in ts)
    q['checks']['peers_follow_same_path']=q.get('max_peer_tip_difference_cm',999)<6
    q['checks']['late_join_requested']=unreal.FoundrylineNetworkTestTools.request_late_join();stage=3;stage_start=now
  elif stage==3 and len(ts)==5 and elapsed>7:
   cameras();q['checks']['fifth_peer_received_machine']=True
   positions=[a.get_torch_tip() for a in ts]
   q['checks']['late_join_matches_position']=max((p-positions[0]).length() for p in positions)<6
   q['checks']['late_join_matches_operation']=all(a.is_operating() for a in ts)
   next(a for a in ts if a.has_authority()).set_operating(False);stage=4;stage_start=now
  elif stage==4 and elapsed>2:
   q['checks']['server_stop_replicates']=not any(a.is_operating() for a in ts)
   q['checks']['stop_clears_every_effect']=all(a.plasma_effect.get_visible_spark_count()==0 for a in ts)
   next(a for a in ts if a.has_authority()).set_operating(True);stage=5;stage_start=now
  elif stage==5 and elapsed>9:
   q['checks']['server_restart_replicates']=all(a.is_operating() and a.is_cutting() for a in ts)
   q['checks']['restart_emits_on_every_peer']=all(a.plasma_effect.get_visible_spark_count()>0 for a in ts);finish();return
  report.write_text(json.dumps(q,indent=2))
 except Exception:q['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
