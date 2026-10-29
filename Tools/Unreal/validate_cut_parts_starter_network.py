import unreal,time,json,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
start=time.monotonic();stage=0;last=0;host=None;saved='';seen=False;q={'checks':{},'errors':[]};out=Path(unreal.Paths.project_saved_dir())/'Logs/CutPartsStarterNetworkQA.json'
def worlds():return unreal.EditorLevelLibrary.get_pie_worlds(False)
def plants():return [a for w in worlds() for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.FoundrylineFactoryPlant)]
def players():return [a for w in worlds() for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if a.is_local_controller()]
def hp():return next(p for p in plants() if p.has_authority())
def act(p,a):unreal.FoundrylineNetworkTestTools.queue_factory_action(p,a)
def finish():
 q['completed']=True;out.write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(worlds()[0],'QUIT_EDITOR')
def tick(dt):
 global stage,last,host,saved,seen
 try:
  t=time.monotonic()-start
  if t>150:raise RuntimeError('Starter network timeout '+str(stage))
  if stage==0 and t>3:stage=1;last=t;unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
  elif stage==1 and t-last>12:
   host=next(p for p in players() if p.has_authority());q['checks']['four_players']=len(players())==4;q['checks']['four_starter_plants']=len(plants())==4 and all(p.get_zone_count()==0 and json.loads(p.get_factory_report())['raw']==10 for p in plants());q['checks']['initial_inventory_matches']=len(set(p.get_ledger_digest() for p in plants()))==1
   c=next(p for p in players() if not p.has_authority());act(c,'Speed');act(c,'Run');stage=2;last=t
  elif stage==2 and t-last>.5:
   q['checks']['client_can_run_shared_cut_shop']=all(json.loads(p.get_factory_report())['running'] for p in plants());unreal.FoundrylineNetworkTestTools.advance_factory_test(host,200)
   r=json.loads(unreal.FoundrylineNetworkTestTools.equipment_motion_report(host));seen=seen or r.get('remaining1',0)>0
   if seen and r.get('remaining1',1)==0:act(host,'Run');stage=3;last=t
   else:last=t
  elif stage==3 and t-last>2:
   q['checks']['cut_batch_completed']=seen;q['checks']['completed_shared_inventory']=len(set(p.get_ledger_digest() for p in plants()))==1;act(host,'Save');saved=hp().get_ledger_digest();stage=4;last=t
  elif stage==4 and t-last>1:act(host,'Load');stage=5;last=t
  elif stage==5 and t-last>2:
   q['checks']['separate_starter_save_load']=all(p.get_ledger_digest()==saved for p in plants());q['checks']['late_join_requested']=unreal.FoundrylineNetworkTestTools.request_late_join();stage=6;last=t
  elif stage==6 and t-last>12:
   q['checks']['five_peers_share_starter']=len(plants())==5 and all(p.get_zone_count()==0 for p in plants());q['checks']['late_join_inventory_matches']=len(set(p.get_ledger_digest() for p in plants()))==1;finish();return
  q['stage']=stage;out.write_text(json.dumps(q,indent=2))
 except Exception:q['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
