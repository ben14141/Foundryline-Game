"""Four-player listen-server welding-station placement check, then a fifth late joiner. No save slots are written."""
import unreal,time,json,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
T=unreal.FoundrylineNetworkTestTools
start=time.monotonic();stage=0;last=0;q={'checks':{},'notes':{},'errors':[]};out=Path(unreal.Paths.project_saved_dir())/'Logs/WeldingStationNetworkQA.json'
CENTER=unreal.Vector(-1806.12,1823.12,20)
def worlds():return unreal.EditorLevelLibrary.get_pie_worlds(False)
def players():return [a for w in worlds() for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if a.is_local_controller()]
def reports():return [json.loads(T.welding_station_report(p)) for p in players()]
def finish():
 q['completed']=True;out.write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(worlds()[0],'QUIT_EDITOR')
def tick(dt):
 global stage,last
 try:
  t=time.monotonic()-start
  if t>220:raise RuntimeError('Welding network timeout at stage '+str(stage))
  if stage==0 and t>3:stage=1;last=t;unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
  elif stage==1 and t-last>12:
   ps=players();q['checks']['four_players']=len(ps)==4;c=next(p for p in ps if not p.has_authority())
   r=reports();q['notes']['initial']=r[0];q['checks']['no_stations_initially']=all(len(x['stations'])==0 for x in r)
   # A client sends the real station command before the lanes exist: the server refuses it.
   T.queue_welding_station(c,CENTER,0,False);stage=2;last=t
  elif stage==2 and t-last>2:
   c=next(p for p in players() if not p.has_authority())
   q['checks']['unconnected_station_refused']=all(len(x['stations'])==0 for x in reports());q['notes']['refusal']=json.loads(T.welding_station_report(c))['status']
   # Same three paint strokes as the StarterShop automation test, sent by a client.
   T.queue_factory_lane(c,0,unreal.Vector(-2217.6,1275,20),unreal.Vector(-2217.6,2170,20),304.8)
   T.queue_factory_lane(c,1,unreal.Vector(-1455,1214.04,20),unreal.Vector(-810,1214.04,20),121.92)
   T.queue_factory_lane(c,2,unreal.Vector(-1394,1450,20),unreal.Vector(-1394,1450,20),200);stage=3;last=t
  elif stage==3 and t-last>2:
   r=reports();q['notes']['lanes']=[x['lanes'] for x in r];q['checks']['client_paint_replicates']=len(set(x['lanes'] for x in r))==1 and r[0]['lanes']==q['notes']['initial']['lanes']+3
   c=next(p for p in players() if not p.has_authority());T.queue_welding_station(c,CENTER,0,False,True);stage=4;last=t
  elif stage==4 and t-last>2:
   q['checks']['foreign_plant_station_rejected']=all(len(x['stations'])==0 for x in reports())
   c=[p for p in players() if not p.has_authority()]
   # Two clients race for the same spot; exactly one station exists afterwards.
   T.queue_welding_station(c[0],CENTER,0,False);T.queue_welding_station(c[1],CENTER,0,False);stage=5;last=t
  elif stage==5 and t-last>2:
   r=reports();q['notes']['placed']=r[0]['stations'];ids=set(json.dumps(x['stations'],sort_keys=True) for x in r)
   q['checks']['race_one_station_on_all_peers']=len(ids)==1 and len(r[0]['stations'])==1
   q['checks']['station_pose_exact']=len(r[0]['stations'])==1 and abs(r[0]['stations'][0]['x']-CENTER.x)<.1 and abs(r[0]['stations'][0]['y']-CENTER.y)<.1
   q['checks']['late_join_requested']=T.request_late_join();stage=6;last=t
  elif stage==6 and t-last>12:
   r=reports();q['checks']['five_peers']=len(r)==5;q['checks']['late_joiner_sees_station']=len(set(json.dumps(x['stations'],sort_keys=True) for x in r))==1 and len(r[-1]['stations'])==1
   c=next(p for p in players() if not p.has_authority());sid=r[0]['stations'][0]['id'] if r[0]['stations'] else 0
   T.queue_welding_station(c,CENTER,0,False,True,True,sid);stage=7;last=t
  elif stage==7 and t-last>2:
   q['checks']['foreign_remove_rejected']=all(len(x['stations'])==1 for x in reports())
   c=next(p for p in players() if not p.has_authority());r=reports();sid=r[0]['stations'][0]['id'] if r[0]['stations'] else 0
   # Phase 2: a client picks headache racks x10 from the station panel and buys frame sections through the paid supplier flow.
   T.select_welding_station(c,sid);T.queue_factory_action(c,'StationSupply');stage=8;last=t
  elif stage==8 and t-last>2:
   c=next(p for p in players() if not p.has_authority());T.queue_factory_action(c,'StationBuy14');stage=9;last=t
  elif stage==9 and t-last>3:
   r=reports();s=[x['stations'][0] for x in r if x['stations']];q['notes']['supply']=s[0] if s else None
   q['checks']['client_recipe_replicates']=len(s)==len(r) and all(x['recipe']==1 and x['batch']==10 for x in s)
   q['checks']['host_books_client_frames']=bool(s) and s[0]['bins']['14'][1]==12
   q['checks']['member_client_sees_booked_frames']=len(s)>1 and all(x['bins']['14'][1]==12 for x in s[1:])
   c=next(p for p in players() if not p.has_authority());T.queue_welding_station(c,CENTER,0,False,False,True,s[0]['id'] if s else 0);stage=10;last=t
  elif stage==10 and t-last>2:
   q['checks']['station_with_booked_parts_not_removed']=all(len(x['stations'])==1 for x in reports());finish();return

  q['stage']=stage;out.write_text(json.dumps(q,indent=2))
 except Exception:q['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
