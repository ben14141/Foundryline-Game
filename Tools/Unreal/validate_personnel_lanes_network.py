"""Four live peers, owned commands, in-flight save/load and a fifth late join."""
import unreal,time,json,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.project_saved_dir())/'Logs/PersonnelLanesNetworkQA.json';start=time.monotonic();stage=0;last=0;host=None;saved='';q={'checks':{},'errors':[],'completed':False}
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
 global stage,last,host,saved
 try:
  t=time.monotonic()-start
  if t>180:raise RuntimeError('Factory network QA timed out: '+str(stage)+' '+str([report(p) for p in plants()]))
  if stage==0 and t>3:stage=1;last=t;unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
  elif stage==1 and t-last>12:
   ps=players();q['checks']['four_players']=len(ps)==4;q['checks']['four_plants']=len(plants())==4
   q['checks']['stocked_starter_shared']=len(set(p.get_ledger_digest() for p in plants()))==1 and all(report(p)['pallets']==29 and p.get_zone_count()==2 for p in plants())
   rigs=[a for w in worlds() for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.FoundrylineFreightRig)]
   q['checks']['pickup_tractor_mode_replicated']=len(rigs)==4 and all(r.get_editor_property('bTractorOnly') and abs(r.get_actor_location().y-1650)<1 for r in rigs)
   q['checks']['two_standard_welders_per_peer']=all(sum(c.get_instance_count() for c in p.get_components_by_class(unreal.InstancedStaticMeshComponent) if c.get_editor_property('static_mesh') and c.get_editor_property('static_mesh').get_name()=='SM_Welder_Machine')==2 for p in plants())
   host=next(p for p in ps if p.has_authority());client=next(p for p in ps if not p.has_authority())
   brush=json.loads(unreal.FoundrylineNetworkTestTools.preview_factory_brush(client,'Forklift',unreal.Vector(-450.37,1800.19,20),unreal.Vector(-145.57,1800.19,20),True,True));q['checks']['client_paint_preview_valid_and_sent']=brush['valid'] and brush['sent'];stage=90;last=t
  elif stage==90 and t-last>2:
   q['checks']['client_painted_lane_replicates_exactly']=len(set(p.get_ledger_digest() for p in plants()))==1
   client=next(p for p in players() if not p.has_authority());unreal.FoundrylineNetworkTestTools.queue_factory_preset(client,unreal.Vector(1855.6,948.96,20),2,0,1);stage=91;last=t
  elif stage==91 and t-last>2:
   client=next(p for p in players() if not p.has_authority());q['checks']['client_zone_over_lane_rejected']=all(p.get_zone_count()==2 for p in plants()) and 'lane' in client.get_factory_feedback().lower()
   unreal.FoundrylineNetworkTestTools.prepare_empty_factory_zones(host);stage=100;last=t
  elif stage==100 and t-last>2:
   client=next(p for p in players() if not p.has_authority());unreal.FoundrylineNetworkTestTools.queue_factory_preset(client,unreal.Vector(1855.6,1863.36,20),2,0,1);stage=101;last=t
  elif stage==101 and t-last>2:
   q['checks']['client_preset_and_supply_link_replicated']=all(unreal.FoundrylineNetworkTestTools.inspect_factory_zones(p)=='1/2/0/1/1' for p in players())
   client=next(p for p in players() if not p.has_authority());unreal.FoundrylineNetworkTestTools.queue_factory_preset(client,unreal.Vector(1855.6,1863.36,20),2,0,1);stage=102;last=t
  elif stage==102 and t-last>2:
   q['checks']['overlapping_client_preset_rejected']=all(unreal.FoundrylineNetworkTestTools.inspect_factory_zones(p)=='1/2/0/1/1' for p in players())
   ps=players();client=next(p for p in ps if not p.has_authority());act(host,'Welder');act(client,'Order');act(host,'Speed');act(ps[-1],'Walk');act(ps[-2],'Walk');stage=2;last=t
  elif stage==2 and t-last>3:
   q['checks']['client_order_replicated']=all(report(p)['running'] for p in plants());act(host,'Speed');stage=3;last=t
  elif stage==3 and t-last>.15:
   r=report(hostplant())
   if r['carried']>0:act(host,'Run');stage=4;last=t
   else:unreal.FoundrylineNetworkTestTools.advance_factory_test(host,1);last=t
  elif stage==4 and t-last>2:
   q['checks']['paused_same_state']=len(set(p.get_ledger_digest() for p in plants()))==1;q['checks']['save_has_cargo']=report(hostplant())['carried']>0
   act(host,'Save');saved=hostplant().get_ledger_digest();stage=5;last=t
  elif stage==5 and t-last>1:
   act(host,'Run');stage=50;last=t
  elif stage==50 and t-last>1:
   unreal.FoundrylineNetworkTestTools.advance_factory_test(host,150);act(host,'Run');stage=6;last=t
  elif stage==6 and t-last>2:
   q['mutated_digest']=hostplant().get_ledger_digest();q['checks']['simulation_changed_after_save']=q['mutated_digest']!=saved
   act(next(p for p in players() if not p.has_authority()),'Load');stage=7;last=t
  elif stage==7 and t-last>2:
   q['checks']['client_load_rejected']=hostplant().get_ledger_digest()==q['mutated_digest'];act(host,'Load');stage=8;last=t
  elif stage==8 and t-last>3:
   q['checks']['host_load_restores_exact_inflight_state']=all(p.get_ledger_digest()==saved for p in plants());act(host,'Run');stage=9;last=t
  elif stage==9 and t-last>.15:
   if report(hostplant())['mask']==63:act(host,'Run');stage=10;last=t
   else:unreal.FoundrylineNetworkTestTools.advance_factory_test(host,200);last=t
  elif stage==10 and t-last>3:
   q['checks']['all_peers_finished_tanker']=all(report(p)['mask']==63 for p in plants());q['checks']['completed_same_state']=len(set(p.get_ledger_digest() for p in plants()))==1
   ps=players();walks=sum('WalkCharacter' in p.get_controlled_pawn().get_class().get_name() for p in ps);q['checks']['two_walk_and_two_top_down']=walks==2 and len(ps)-walks==2
   act(next(p for p in ps if not p.has_authority()),'Collect');stage=11;last=t
  elif stage==11 and t-last>3:
   q['checks']['client_collect_replicated']=all(report(p)['finished']==1 for p in plants());q['checks']['late_join_requested']=unreal.FoundrylineNetworkTestTools.request_late_join();stage=12;last=t
  elif stage==12 and t-last>12:
   q['checks']['five_peers']=len(plants())==5;q['checks']['late_join_exact_state']=len(set(p.get_ledger_digest() for p in plants()))==1;q['reports']=[report(p) for p in plants()];finish();return
  q['stage']=stage;q['live_reports']=[report(p) for p in plants()];out.write_text(json.dumps(q,indent=2))
 except Exception:q['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
