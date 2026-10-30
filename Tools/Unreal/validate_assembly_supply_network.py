"""Four-player shared collector race, foreign-target rejection, and fifth-player late join. No saves."""
import unreal,time,json,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
T=unreal.FoundrylineNetworkTestTools
root=Path(unreal.Paths.project_saved_dir()).resolve();start=time.monotonic();stage=0;last=0;employee=0;station=0
result={'checks':{},'errors':[]}
def worlds():return unreal.EditorLevelLibrary.get_pie_worlds(False)
def players():return [a for w in worlds() for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if a.is_local_controller()]
def host():return next(p for p in players() if p.has_authority())
def guests():return [p for p in players() if not p.has_authority()]
def reports():return [json.loads(T.employee_report(p)) for p in players()]
def guid(text):
 g=unreal.Guid()
 for n,i in zip('abcd',range(0,32,8)):g.set_editor_property(n,int(text.replace('-','')[i:i+8],16))
 return g
def finish():
 result['completed']=True;(root/'Logs/AssemblySupplyNetworkQA.json').write_text(json.dumps(result,indent=2));unreal.unregister_slate_post_tick_callback(handle)
 if worlds():unreal.SystemLibrary.execute_console_command(worlds()[0],'QUIT_EDITOR')
def tick(dt):
 global stage,last,employee,station
 try:
  now=time.monotonic()-start
  if now>240:raise RuntimeError('Network timeout stage '+str(stage))
  if stage==0 and now>3:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1;last=now
  elif stage==1 and now-last>15:
   result['checks']['four_players']=len(players())==4;assert len(players())==4,str(len(players()))
   p=host();T.queue_property_view(p);T.queue_factory_lane(p,0,unreal.Vector(-2217.6,1275,20),unreal.Vector(-2217.6,2170,20),304.8);T.queue_factory_lane(p,1,unreal.Vector(-1455,1214.04,20),unreal.Vector(-810,1214.04,20),121.92);T.queue_factory_lane(p,2,unreal.Vector(-1370,1450,20),unreal.Vector(-1370,1450,20),200);stage=2;last=now
  elif stage==2 and now-last>3:T.queue_welding_station(host(),unreal.Vector(-1768.02,1823.12,20),0,False);stage=3;last=now
  elif stage==3 and now-last>3:
   rows=[json.loads(T.welding_station_report(p))['stations'] for p in players()];result['checks']['station_on_all_peers']=all(len(r)==1 for r in rows);assert rows[0],rows
   station=rows[0][0]['id'];T.setup_welding_supply_review(host(),station);T.queue_factory_action(host(),'StationSupply');T.queue_employee_hire(guests()[0],3,guid('573fdc658d274a728301c237d5ca7a11'),guid(reports()[0]['plant']));stage=4;last=now
  elif stage==4 and now-last>3:
   employee=next(e['id'] for e in reports()[0]['employees'] if e['role']==3);plant=guid(reports()[0]['plant']);T.queue_employee_assignment(guests()[0],employee,4,station,plant);T.queue_employee_assignment(guests()[1],employee,4,station,plant);stage=5;last=now
  elif stage==5 and now-last>4:
   data=reports();result['checks']['one_collector_after_race']=all(len([e for e in r['employees'] if e['role']==4])==1 and r['count']==3 for r in data)
   T.queue_employee_assignment(guests()[0],employee,4,station,guid('ffffffffffffffffffffffffffffffff'));result['checks']['late_join_requested']=T.request_late_join();stage=6;last=now
  elif stage==6 and now-last>15:
   result['checks']['five_players']=len(players())==5;data=reports();result['checks']['late_join_sees_collector']=all(any(e['id']==employee and e['role']==4 and e['assignment']==station for e in r['employees']) for r in data)
   result['checks']['foreign_target_did_not_reassign']=result['checks']['late_join_sees_collector'];T.queue_factory_action(host(),'Run');stage=7;last=now
  elif stage==7 and now-last>4:
   hashes=[];bins=[]
   for world in worlds():
    plant=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.FoundrylineFactoryPlant));hashes.append(plant.get_ledger_digest());bins.append(sum('AssemblySupplyBin' in str(c.static_mesh.get_path_name()) for c in plant.get_components_by_class(unreal.StaticMeshComponent) if c.static_mesh))
   result['checks']['paused_full_state_equal']=len(set(hashes))==1;result['checks']['bin_visual_on_all_peers']=all(n>=1 for n in bins);result['checks']['host_state_valid']=json.loads(T.employee_report(host())).get('valid',False);result['hashes']=hashes;result['bins']=bins;finish();return
  result['stage']=stage;(root/'Logs/AssemblySupplyNetworkQA.json').write_text(json.dumps(result,indent=2))
 except Exception:result['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
