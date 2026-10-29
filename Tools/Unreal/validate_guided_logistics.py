"""Four-player guided trip isolation, stale-route invalidation and shared zones."""
import unreal,time,json,traceback
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
if not (root/'Foundryline.uproject').is_file():raise RuntimeError('Foundryline only')
N=unreal.FoundrylineNetworkTestTools
report=root/'Saved/Logs/GuidedLogisticsNetwork.json'
qa={'started':time.monotonic(),'stage':0,'action':0,'checks':{},'errors':[],'completed':False}
def write():report.write_text(json.dumps({k:v for k,v in qa.items() if k!='started'},indent=2))
def worlds():return list(unreal.EditorLevelLibrary.get_pie_worlds(False))
def pc(w):return next(p for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller())
def setup():
 ws=worlds();h=next(w for w in ws if unreal.GameplayStatics.get_game_mode(w));return h,sorted([w for w in ws if w!=h],key=lambda w:w.get_path_name())
def finish():
 qa['completed']=True;write();unreal.unregister_slate_post_tick_callback(handle);unreal.log('FOUNDRYLINE_GUIDED_NETWORK_DONE');unreal.SystemLibrary.execute_console_command(unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world(),'QUIT_EDITOR')
def tick(dt):
 try:
  t=time.monotonic()-qa['started']
  if qa['stage']==0 and t>3:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();qa['stage']=1;return
  schedule=[15,18,21,24,40,43,46,49,53,58]
  i=qa['action']
  if qa['stage'] and i<len(schedule) and t>schedule[i]:
   h,cs=setup()
   if i==0:
    qa['checks']['four_players']=len(worlds())==4;qa['checks']['hidden_defaults']=all(not pc(w).is_logistics_overlay_visible() for w in worlds());unreal.SystemLibrary.set_timer_for_next_tick(pc(cs[0]),'ToggleLogisticsEdit')
   elif i==1:N.queue_truck_zone(pc(cs[0]),unreal.IntVector(-4000,-2400,0),unreal.IntVector(6000,5300,0),False)
   elif i==2:
    qa['checks']['client_zones_synced']=len({N.driving_inventory(pc(w)) for w in worlds()})==1
    p=pc(cs[0]);p.logistics_action('TripEntrance');p.select_logistics_point(unreal.Vector(2000,-2400,0));p.logistics_action('TripDestination');p.select_logistics_point(unreal.Vector(-400,2575,0));p.logistics_action('TripExit');p.select_logistics_point(unreal.Vector(2000,-2400,0))
    qa['checks']['client_completed_steps']=p.get_logistics_summary().startswith('TRUCK/1/1/1/')
    qa['checks']['other_drafts_untouched']=all(pc(w).get_logistics_summary().startswith('TRUCK/0/0/0/') for w in [h,cs[1],cs[2]])
   elif i==3:pc(cs[0]).logistics_action('TripTest')
   elif i==4:
    state=pc(cs[0]).get_logistics_summary();qa['trip_result']=state;qa['checks']['client_full_trip_preview']=state.startswith('TRUCK/1/1/1/0/1/') and int(state.split('/')[-1])>0
    qa['checks']['preview_stays_local']=all(pc(w).get_logistics_summary().endswith('/0/0/0') for w in [h,cs[1],cs[2]])
    pc(cs[0]).logistics_action('TripPlay');N.queue_truck_zone(pc(cs[1]),unreal.IntVector(1000,1000,0),unreal.IntVector(2000,2000,0),True)
   elif i==5:
    qa['checks']['peer_edit_invalidates_checked_trip']=pc(cs[0]).get_logistics_summary().endswith('/0/0/0');qa['checks']['zone_erase_synced']=len({N.driving_inventory(pc(w)) for w in worlds()})==1
    pc(cs[2]).logistics_action('TripCar');pc(cs[2]).select_logistics_point(unreal.Vector(-2400,-2400,0))
   elif i==6:
    qa['checks']['car_draft_independent']=pc(cs[2]).get_logistics_summary().startswith('CAR/1/0/0/') and pc(cs[0]).get_logistics_summary().startswith('TRUCK/1/1/1/')
    qa['checks']['late_join_requested']=N.request_late_join()
   elif i==7:
    qa['checks']['fifth_joined']=len(worlds())==5;qa['checks']['late_join_shared_zones']=len({N.driving_inventory(pc(w)) for w in worlds()})==1
   elif i==8:
    qa['checks']['fifth_has_no_other_player_draft']=sum(pc(w).get_logistics_summary().startswith('TRUCK/0/0/0/') for w in worlds())==3
   elif i==9:finish();return
   qa['action']+=1;write()
 except Exception:
  qa['errors'].append(traceback.format_exc());unreal.log_error(qa['errors'][-1]);finish()
handle=unreal.register_slate_post_tick_callback(tick);write()
