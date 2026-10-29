import unreal,json,time,traceback
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve();report=root/'Saved/Logs/TankerNetworkQA.json'
q={'stage':0,'checks':{},'errors':[],'completed':False};start=time.monotonic();last=0;motion='';driver=None;parked=None;tractor_before=None
N=unreal.FoundrylineNetworkTestTools
def ws():return list(unreal.EditorLevelLibrary.get_pie_worlds(False))
def pc(w):return next(p for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.FoundrylinePlayerController) if p.is_local_controller())
def rig(w):return unreal.GameplayStatics.get_all_actors_of_class(w,unreal.FoundrylineFreightRig)[0]
def write():report.write_text(json.dumps(q,indent=2))
def cmd(p,a):N.queue_freight_control(p,a,0,0)
def done():
 q['completed']=True;write();unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world(),'QUIT_EDITOR')
def tick(dt):
 global last,motion,driver,parked,tractor_before
 try:
  t=time.monotonic()-start
  if driver and t-last>.1:
   last=t;r=rig(driver.get_world());throttle=-1 if motion=='reverse' and not r.state.coupled else 1 if motion=='forward' else 0
   N.queue_freight_control(driver,0,throttle,0)
  s=q['stage'];schedule=[4,14,38,43,46,49,51,55,57,63,65]
  if s>=len(schedule) or t<schedule[s]:return
  if s==0:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
  else:
   worlds=ws();host=next(w for w in worlds if unreal.GameplayStatics.get_game_mode(w));clients=sorted([w for w in worlds if w!=host],key=lambda w:w.get_path_name())
   if s==1:
    q['checks']['four_players']=len(worlds)==4;driver=pc(clients[0]);cmd(driver,10);motion='reverse'
   elif s==2:
    q['checks']['reversing_auto_couples_on_server_and_clients']=all(rig(w).state.coupled for w in worlds)
    q['checks']['gear_retracts_everywhere']=all(rig(w).state.gear<.01 for w in worlds)
    q['checks']['wheel_travel_replicates']=all(abs(rig(w).state.tractor_travel)>500 for w in worlds)
    tractor_before=unreal.Vector(rig(host).state.tractor.x,rig(host).state.tractor.y,rig(host).state.tractor.z);cmd(pc(clients[1]),10);motion='forward'
   elif s==3:
    q['checks']['driver_lock_rejects_other_player']=all(rig(w).driver and rig(w).driver.get_player_name()==driver.player_state.get_player_name() for w in worlds)
    q['checks']['trailer_moves_when_hitched']=all(rig(w).state.trailer.x>100 for w in worlds);motion=''
   elif s==4:
    q['checks']['truck_stops_on_release']=all(abs(rig(w).state.speed)<1 for w in worlds);cmd(driver,2)
   elif s==5:
    q['checks']['gear_extends_everywhere']=all(rig(w).state.gear>.99 for w in worlds);cmd(driver,3)
   elif s==6:
    q['checks']['supported_trailer_detaches_everywhere']=all(not rig(w).state.coupled for w in worlds);parked=unreal.Vector(rig(host).state.trailer.x,rig(host).state.trailer.y,rig(host).state.trailer.z);tractor_before=unreal.Vector(rig(host).state.tractor.x,rig(host).state.tractor.y,rig(host).state.tractor.z);motion='forward'
   elif s==7:
    motion='';q['checks']['trailer_remains_parked']=all(rig(w).state.trailer.distance(parked)<1 for w in worlds)
    q['tractor_departure_distance_cm']=[((rig(w).state.tractor.x-tractor_before.x)**2+(rig(w).state.tractor.y-tractor_before.y)**2)**.5 for w in worlds]
    q['checks']['tractor_drives_away']=all(d>100 for d in q['tractor_departure_distance_cm'])
   elif s==8:
    cmd(driver,11);driver=None;cmd(pc(clients[1]),10);q['checks']['late_join_requested']=N.request_late_join()
   elif s==9:
    q['checks']['five_players_receive_detached_state']=len(worlds)==5 and all(not rig(w).state.coupled and rig(w).state.gear>.99 and abs(rig(w).state.speed)<1 for w in worlds)
    q['checks']['driver_handoff_replicates']=all(rig(w).driver and rig(w).driver.get_player_name()==pc(clients[1]).player_state.get_player_name() for w in worlds)
   elif s==10:done();return
  q['stage']+=1
  if s>0:q['last_state']=[{'x':rig(w).state.tractor.x,'travel':rig(w).state.tractor_travel,'driver':rig(w).driver.get_player_name() if rig(w).driver else '', 'feedback':rig(w).feedback} for w in ws()]
  write()
 except Exception:q['errors'].append(traceback.format_exc());done()
handle=unreal.register_slate_post_tick_callback(tick);write()
