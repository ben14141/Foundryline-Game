import unreal,time,json,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
start=time.monotonic();stage=0;last=0;host=None;target=None;other=None;before={};q={'checks':{},'errors':[]};out=Path(unreal.Paths.project_saved_dir())/'Logs/BusinessFoundationNetworkQA.json'
def worlds():return unreal.EditorLevelLibrary.get_pie_worlds(False)
def players():return [p for w in worlds() for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller()]
def registries():return [p for w in worlds() for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.FoundrylineBusinessRegistry)]
def own(p):return json.loads(unreal.FoundrylineNetworkTestTools.own_business_report(p))
def act(p,a):return unreal.FoundrylineNetworkTestTools.queue_factory_action(p,a)
def finish():
 q['completed']=True;out.write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(worlds()[0],'QUIT_EDITOR')
def tick(dt):
 global stage,last,host,target,other,before
 try:
  t=time.monotonic()-start
  if t>150:raise RuntimeError('Business network timeout '+str(stage))
  if stage==0 and t>3:stage=1;last=t;unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
  elif stage==1 and t-last>14:
   host=next(p for p in players() if p.has_authority());q['checks']['four_players']=len(players())==4;setup=json.loads(unreal.FoundrylineNetworkTestTools.exercise_business_foundation(host,0));q['create_error']=setup.pop('create_error','');q['checks'].update(setup);stage=2;last=t
  elif stage==2 and t-last>4:
   q['checks']['three_companies_replicated']=len(registries())==4 and all(r.get_company_count()==3 for r in registries());q['checks']['directory_matches_on_all_peers']=len(set(r.get_business_digest() for r in registries()))==1;q['checks']['three_isolated_plants_match_on_all_peers']=len(set(unreal.FoundrylineNetworkTestTools.scoped_plant_digest(p) for p in players()))==1
   groups={}
   for p in players():groups.setdefault(own(p)['company'],[]).append(p)
   company=next(c for c,ps in groups.items() if len(ps)==2);target=groups[company][0];other=next(p for p in players() if own(p)['company'] not in (company,own(host)['company']));before={'host':own(host),'other':own(other)}
   unreal.FoundrylineNetworkTestTools.queue_factory_console(target,1);stage=3;last=t
  elif stage==3 and t-last>1:act(target,'Apply');act(target,'Run');stage=4;last=t
  elif stage==4 and t-last>3:
   q['checks']['client_operates_own_company_plant']=own(target).get('running',False);q['checks']['other_companies_inventory_unchanged']=own(host)['plant_digest']==before['host']['plant_digest'] and own(other)['plant_digest']==before['other']['plant_digest'];act(target,'Run');stage=5;last=t
  elif stage==5 and t-last>2:
   q['checks'].update(json.loads(unreal.FoundrylineNetworkTestTools.exercise_business_foundation(host,1)));q['checks']['late_join_requested']=unreal.FoundrylineNetworkTestTools.request_late_join();stage=6;last=t
  elif stage==6 and t-last>12:
   q['checks']['five_player_directory_matches']=len(players())==5 and len(registries())==5 and len(set(r.get_business_digest() for r in registries()))==1;q['checks']['late_join_scoped_inventory_matches']=len(set(unreal.FoundrylineNetworkTestTools.scoped_plant_digest(p) for p in players()))==1;q['reports']=[own(p) for p in players()];finish();return
  q['stage']=stage;out.write_text(json.dumps(q,indent=2))
 except Exception:q['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
