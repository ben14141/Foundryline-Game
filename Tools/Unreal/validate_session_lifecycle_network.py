import unreal,time,json,traceback
from pathlib import Path
# Four-player listen-server PIE: a driving client and a walking client leave, the waiting player takes the truck, then a fifth player joins.
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
N=unreal.FoundrylineNetworkTestTools;start=time.monotonic();stage=0;last=0;host=a=b=c=None;ids={};q={'checks':{},'errors':[],'reports':{}};out=Path(unreal.Paths.project_saved_dir())/'Logs/SessionLifecycleNetworkQA.json'
def worlds():return list(unreal.EditorLevelLibrary.get_pie_worlds(False))
def players():return [p for w in worlds() for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.FoundrylinePlayerController) if p.is_local_controller()]
def rep(p):return json.loads(N.session_report(p))
def alive(p):
 try:return p.is_local_controller() and p.player_state is not None
 except Exception:return False
def snap(name):q['reports'][name]={k:rep(p) for k,p in (('host',host),('a',a),('b',b),('c',c)) if p and alive(p)}
def finish():
 q['completed']=True;q['passed']=bool(q['checks']) and all(q['checks'].values()) and not q['errors'];out.write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(worlds()[0] if worlds() else None,'QUIT_EDITOR')
def tick(dt):
 global stage,last,host,a,b,c
 try:
  t=time.monotonic()-start
  if t>180:raise RuntimeError('Session lifecycle timeout at stage '+str(stage))
  if stage==0 and t>3:stage=1;last=t;unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
  elif stage==1 and t-last>20:
   ps=players();host=next(p for p in ps if p.has_authority());a,b,c=sorted([p for p in ps if p!=host],key=lambda p:p.get_world().get_path_name())
   for k,p in (('host',host),('a',a),('b',b),('c',c)):ids[k]=rep(p)['player_id']
   snap('start');h=rep(host);q['checks']['four_players']=len(ps)==4;q['checks']['four_ledger_members']=h.get('ledger_members')==4;q['checks']['directory_lists_four']=all(rep(p).get('active_members')==4 for p in ps)
   q['checks']['no_leaked_pawns_at_start']=h['unpossessed_pawns']==h['walking'];q['checks']['rig_spawned']=N.spawn_freight_rig_for_test(host,unreal.Vector(2000,-1800,20));N.queue_property_view(a);N.queue_property_view(b);stage=2;last=t
  elif stage==2 and t-last>4:
   snap('property_view');q['checks']['drivers_left_walk_mode']=not rep(a)['walking'] and not rep(b)['walking'];N.queue_freight_control(a,10,0,0);stage=3;last=t
  elif stage==3 and t-last>2:
   q['checks']['a_holds_truck_everywhere']=all(rep(p).get('rig_driver')==ids['a'] for p in (host,a,b,c));N.queue_freight_drive_toggle(b);stage=4;last=t
  elif stage==4 and t-last>2:
   snap('before_driver_leaves');q['checks']['b_cannot_take_occupied_truck']=all(rep(p).get('rig_driver')==ids['a'] for p in (host,a,b,c));q['checks']['kick_driver']=N.kick_client_for_test(a);stage=5;last=t
  elif stage==5 and t-last>4:
   snap('after_driver_left');h=rep(host);q['checks']['driver_membership_removed']=h.get('ledger_members')==3;q['checks']['directory_drops_driver']=all(rep(p).get('active_members')==3 for p in (host,b,c))
   q['checks']['waiting_player_takes_truck']=all(rep(p).get('rig_driver')==ids['b'] for p in (host,b,c));q['checks']['no_leaked_pawn_after_driver_left']=h['unpossessed_pawns']==h['walking'];q['checks']['c_is_walking']=rep(c)['walking'];q['checks']['kick_walker']=N.kick_client_for_test(c);stage=6;last=t
  elif stage==6 and t-last>4:
   snap('after_walker_left');h=rep(host);q['checks']['walker_membership_removed']=h.get('ledger_members')==2;q['checks']['directory_drops_walker']=all(rep(p).get('active_members')==2 for p in (host,b))
   q['checks']['no_leaked_camera_after_walker_left']=h['unpossessed_pawns']==h['walking'];q['checks']['host_still_owns_coop']=rep(host)['has_company'];q['checks']['late_join_requested']=N.request_late_join();stage=7;last=t
  elif stage==7 and t-last>15:
   new=[p for p in players() if alive(p) and rep(p)['player_id'] not in ids.values()];h=rep(host);q['reports']['late_join']=[rep(p) for p in new]+[h]
   q['checks']['late_joiner_gets_company']=len(new)==1 and rep(new[0])['has_company'];q['checks']['ledger_after_late_join']=h.get('ledger_members')==3;q['checks']['directory_after_late_join']=all(rep(p).get('active_members')==3 for p in [host,b]+new);q['checks']['no_leaked_pawns_at_end']=h['unpossessed_pawns']==h['walking'];finish();return
  q['stage']=stage;out.write_text(json.dumps(q,indent=2))
 except Exception:q['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
