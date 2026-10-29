"""Four live peers exercise real-stock machine motion, pause, save/load and a fifth late join."""
import unreal,time,json,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.project_saved_dir())/'Logs/EquipmentMotionNetworkQA.json';start=time.monotonic();stage=0;last=0;host=None;saved='';motion_before=[];frozen=[];q={'checks':{},'errors':[],'completed':False}
def worlds():return unreal.EditorLevelLibrary.get_pie_worlds(False)
def plants():return [a for w in worlds() for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.FoundrylineFactoryPlant)]
def players():return [p for w in worlds() for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller()]
def act(p,a):return unreal.FoundrylineNetworkTestTools.queue_factory_action(p,a)
def report(p):return json.loads(p.get_factory_report())
def hostplant():return next(p for p in plants() if p.has_authority())
def finish():
 q['completed']=True;out.write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle)
 unreal.SystemLibrary.execute_console_command(worlds()[0],'QUIT_EDITOR')
def tick(dt):
 global stage,last,host,saved,motion_before,frozen
 try:
  t=time.monotonic()-start
  q['checks']['diagonal_route_observed']=q['checks'].get('diagonal_route_observed',False) or any(report(p).get('diagonal_segments',0)>0 for p in plants())
  q['checks']['rounded_route_observed']=q['checks'].get('rounded_route_observed',False) or any(report(p).get('curved_segments',0)>0 for p in plants())
  if t>180:raise RuntimeError('Factory network QA timed out: '+str(stage)+' '+str([report(p) for p in plants()]))
  if stage==0 and t>3:stage=1;last=t;unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
  elif stage==1 and t-last>12:
   ps=players();q['checks']['four_players']=len(ps)==4;q['checks']['four_plants']=len(plants())==4
   q['checks']['stocked_starter_shared']=len(set(p.get_ledger_digest() for p in plants()))==1 and all(report(p)['pallets']==29 and p.get_zone_count()==2 for p in plants())
   rigs=[a for w in worlds() for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.FoundrylineFreightRig)]
   q['checks']['pickup_tractor_mode_replicated']=len(rigs)==4 and all(r.get_editor_property('bTractorOnly') and abs(r.get_actor_location().y-1650)<1 for r in rigs)
   q['checks']['two_standard_welders_per_peer']=all(sum(c.get_instance_count() for c in p.get_components_by_class(unreal.InstancedStaticMeshComponent) if c.get_editor_property('static_mesh') and c.get_editor_property('static_mesh').get_name()=='SM_Welder_Machine')==2 for p in plants())
   host=next(p for p in ps if p.has_authority());q['checks']['real_stock_equipment_fixture']=unreal.FoundrylineNetworkTestTools.start_equipment_exercise(host);stage=70;last=t
  elif stage==70 and t-last>3:
   motion_before=[json.loads(unreal.FoundrylineNetworkTestTools.equipment_motion_report(p)) for p in players()];q['checks']['all_three_machines_active_on_all_peers']=all(all(r.get('remaining'+str(i),0)>0 for i in range(1,4)) for r in motion_before);stage=71;last=t
  elif stage==71 and t-last>1:
   after=[json.loads(unreal.FoundrylineNetworkTestTools.equipment_motion_report(p)) for p in players()];q['motion_before']=motion_before;q['motion_after']=after
   q['checks']['plasma_moves_on_every_peer']=all(sum((a-b)**2 for a,b in zip(r['torch'],old['torch']))>.1 for r,old in zip(after,motion_before))
   q['checks']['press_moves_on_every_peer']=all(abs(r['ram']-old['ram'])>.1 for r,old in zip(after,motion_before))
   q['checks']['roller_moves_and_forms_on_every_peer']=all(abs(r['roller']-old['roller'])>.1 and r['forming_visible'] and r['forming_segments']==32 for r,old in zip(after,motion_before))
   act(host,'Run');stage=72;last=t
  elif stage==72 and t-last>.9:frozen=[json.loads(unreal.FoundrylineNetworkTestTools.equipment_motion_report(p)) for p in players()];stage=73;last=t
  elif stage==73 and t-last>.7:
   now=[json.loads(unreal.FoundrylineNetworkTestTools.equipment_motion_report(p)) for p in players()]
   q['checks']['pause_freezes_all_machines_on_every_peer']=all(not r['running'] and not r['cutting'] and abs(r['ram']-old['ram'])<.01 and abs(r['roller']-old['roller'])<.01 and sum((a-b)**2 for a,b in zip(r['torch'],old['torch']))<.01 for r,old in zip(now,frozen));act(host,'Run');stage=74;last=t
  elif stage==74 and t-last>16:
   done=[json.loads(unreal.FoundrylineNetworkTestTools.equipment_motion_report(p)) for p in players()];q['checks']['real_cycles_finish_after_resume']=all(all(r.get('remaining'+str(i),1)==0 for i in range(1,4)) for r in done)
   act(host,'Run');stage=75;last=t
  elif stage==75 and t-last>2:
   q['checks']['equipment_final_state_matches']=len(set(p.get_ledger_digest() for p in plants()))==1
   act(host,'Save');saved=hostplant().get_ledger_digest();stage=76;last=t
  elif stage==76 and t-last>1:
   act(host,'Load');stage=77;last=t
  elif stage==77 and t-last>2:
   q['checks']['machine_results_survive_save_load']=all(p.get_ledger_digest()==saved for p in plants())
   q['checks']['late_join_requested']=unreal.FoundrylineNetworkTestTools.request_late_join();stage=78;last=t
  elif stage==78 and t-last>12:
   q['checks']['five_peers']=len(plants())==5;q['checks']['late_join_exact_equipment_state']=len(set(p.get_ledger_digest() for p in plants()))==1;finish();return
  q['stage']=stage;q['live_reports']=[report(p) for p in plants()];out.write_text(json.dumps(q,indent=2))
 except Exception:q['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
