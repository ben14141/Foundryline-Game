"""Real four-player RPC, persistent node graph, late join and independent route previews."""
import unreal,time,json,traceback
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
assert (root/'Foundryline.uproject').is_file()
N=unreal.FoundrylineNetworkTestTools
out=root/'Saved/Logs/RouteNetworkMultiplayer.json'
q={'started':time.monotonic(),'stage':0,'checks':{},'errors':[],'completed':False}
slot='DrivingQA_Nodes_'+str(int(time.time()))
q['slot']=slot
def worlds():return list(unreal.EditorLevelLibrary.get_pie_worlds(False))
def player(w):return next(p for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller())
def group():
 ws=worlds();h=next(w for w in ws if unreal.GameplayStatics.get_game_mode(w));return h,sorted([w for w in ws if w!=h],key=lambda w:w.get_path_name())
def action(p,a,xy=None):N.queue_logistics_action(p,a,unreal.Vector(*(xy or (0,0)),0),xy is not None)
def digest(w):return '/'.join(player(w).get_network_summary().split('/')[:4])
def write():out.write_text(json.dumps({k:v for k,v in q.items() if k!='started'},indent=2))
def done():
 q['completed']=True;q['save_cleaned']=unreal.GameplayStatics.delete_game_in_slot('Foundryline_'+slot,0);write();unreal.unregister_slate_post_tick_callback(handle);unreal.log('FOUNDRYLINE_NODE_NETWORK_DONE');unreal.SystemLibrary.execute_console_command(unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world(),'QUIT_EDITOR')
def tick(dt):
 try:
  t=time.monotonic()-q['started'];s=q['stage']
  schedule=[4,16,19,22,25,28,31,34,37,40,43,46,50,54,58,62,66,70,74]
  if s>=len(schedule) or t<schedule[s]:return
  if s==0:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
  else:
   h,cs=group();p=player(cs[0])
   if s==1:q['checks']['four_players']=len(worlds())==4;action(p,'NetPanel');N.queue_truck_zone(p,unreal.IntVector(-400,-2400,0),unreal.IntVector(6000,5300,0),False)
   elif s==2:action(p,'NetDraw',(3000,800))
   elif s==3:action(p,'NetDraw',(3000,2600))
   elif s==4:action(p,'NetDraw',(4800,2600))
   elif s==5:
    q['checks']['client_created_three_shared_nodes']=all(player(w).get_network_summary().split('/')[1:4]==['3','2','0'] for w in worlds());q['checks']['digests_agree']=len({digest(w) for w in worlds()})==1;action(p,'NetFinish')
   elif s==6:action(p,'NetSelect',(3000,800))
   elif s==7:action(p,'NetStart')
   elif s==8:action(p,'NetCheck')
   elif s==9:
    q['route']=p.get_network_summary();q['checks']['client_checked_articulated_route']=p.get_network_summary().split('/')[4]=='1';q['checks']['other_previews_are_independent']=all(player(w).get_network_summary().split('/')[4]=='0' for w in [h,cs[1],cs[2]]);q['checks']['save_started']=N.test_driving_save(player(h),slot,False)
   elif s==10:q['checks']['save_completed']=not N.test_save_busy(player(h)) and unreal.GameplayStatics.does_save_game_exist('Foundryline_'+slot,0);action(player(cs[1]),'NetPanel');action(player(cs[1]),'NetSelect',(3000,2600))
   elif s==11:action(player(cs[1]),'NetStaging')
   elif s==12:
    q['checks']['peer_edit_invalidates_preview']=p.get_network_summary().split('/')[4]=='0';q['checks']['peer_edit_synced']=len({digest(w) for w in worlds()})==1;action(player(cs[1]),'NetDelete')
   elif s==13:q['checks']['deletion_prunes_links_everywhere']=all(player(w).get_network_summary().split('/')[1:4]==['2','0','0'] for w in worlds());q['checks']['load_started']=N.test_driving_save(player(h),slot,True)
   elif s==14:q['checks']['host_load_restores_shared_ids_and_links']=all(player(w).get_network_summary().split('/')[1:4]==['3','2','0'] for w in worlds());q['checks']['late_join_requested']=N.request_late_join()
   elif s==15:q['checks']['fifth_player_has_graph']=len(worlds())==5 and len({digest(w) for w in worlds()})==1;action(p,'TrafficPanel');action(p,'TrafficTest')
   elif s==16:q['checks']['client_direct_construction_still_blocked']=N.client_direct_execution_blocked(p);q['checks']['client_cannot_grant_traffic_priority']=N.client_traffic_blocked(p);action(player(cs[2]),'NetPanel');action(player(cs[2]),'NetDraw',(100, -3000))
   elif s==17:q['checks']['public_street_edit_rejected']=all(player(w).get_network_summary().split('/')[1:4]==['3','2','0'] for w in worlds())
   elif s==18:done();return
  q['stage']+=1;write()
 except Exception:q['errors'].append(traceback.format_exc());unreal.log_error(q['errors'][-1]);done()
handle=unreal.register_slate_post_tick_callback(tick);write()
