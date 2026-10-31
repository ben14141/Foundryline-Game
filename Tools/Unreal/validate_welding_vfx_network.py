import unreal,time,json,traceback
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve();report=root/'Saved/Logs/WeldingVFXNetworkQA.json'
start=time.monotonic();stage=0;seen=set();q={'checks':{},'errors':[],'completed':False}
def worlds():return list(unreal.EditorLevelLibrary.get_pie_worlds(False))
def effects():return [a.welding_effect for w in worlds() for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.FoundrylineWeldingStation)]
def done():
 q['completed']=True;report.write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world(),'QUIT_EDITOR')
def tick(dt):
 global stage
 try:
  t=time.monotonic()-start
  if stage==0 and t>4:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1
  elif stage==1 and t>14:
   q['checks']['four_peer_effects']=len(effects())==4
   for w in worlds():
    for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController):
     if p.is_local_controller():unreal.FoundrylineNetworkTestTools.setup_welding_review(p)
   stage=2
  elif stage==2:
   es=effects();ws=worlds()
   if es:
    phase=unreal.GameplayStatics.get_time_seconds(ws[0])%3.5
    if .4<phase<1.9 or 2.6<phase<3.2:
     states=[e.is_welding() for e in es];seen.update(states if len(set(states))==1 else [])
     q['checks']['settled_states_agree']=q['checks'].get('settled_states_agree',True) and len(set(states))==1
     q['checks']['spark_budget_bounded']=all(e.get_visible_spark_count()<=640 for e in es)
     if all(states):q['checks']['nearby_live_ticks_emit_sparks']=q['checks'].get('nearby_live_ticks_emit_sparks',False) or all(e.get_visible_spark_count()>0 for e in es)
     elif not any(states):q['checks']['off_state_clears_sparks']=all(e.get_visible_spark_count()==0 for e in es)
   if t>23:q['checks']['on_and_off_observed']=seen=={True,False};q['checks']['late_join_requested']=unreal.FoundrylineNetworkTestTools.request_late_join();stage=3
  elif stage==3 and t>32:
   q['checks']['fifth_player_has_effect']=len(effects())==5;stage=4
  elif stage==4:
   ws=worlds();phase=unreal.GameplayStatics.get_time_seconds(ws[0])%3.5
   if .4<phase<1.9 or 2.6<phase<3.2:q['checks']['late_join_activity_matches']=len(set(e.is_welding() for e in effects()))==1;done()
  report.write_text(json.dumps(q,indent=2))
 except Exception:q['errors'].append(traceback.format_exc());done()
handle=unreal.register_slate_post_tick_callback(tick)
