"""Four-client grid-sensitive edits and a fifth late join. No saves."""
import json,time,traceback
from pathlib import Path
import unreal
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
assert (root/'Foundryline.uproject').is_file()
N=unreal.FoundrylineNetworkTestTools
report=root/'Saved/Logs/PavementNetworkQA.json'
q={'stage':0,'checks':{},'errors':[],'completed':False}
started=time.monotonic()
def worlds():return list(unreal.EditorLevelLibrary.get_pie_worlds(False))
def pc(w):return next(p for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller())
def site(w):return next(s for s in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.FoundrylineBuildSite))
def queue(w,t,a,b):return N.queue_construction_tool(pc(w),t,unreal.IntVector(*a,0),unreal.IntVector(*b,0),0,unreal.LinearColor(1,1,1,1))
def desc(w,p):return N.describe_construction(pc(w),unreal.IntVector(*p,0))
def agree():return len({site(w).get_property_digest() for w in worlds()})==1 and not any(pc(w).is_construction_pending() for w in worlds())
def write():report.write_text(json.dumps(q,indent=2))
def done():
 q['completed']=True;write();unreal.unregister_slate_post_tick_callback(handle)
 unreal.SystemLibrary.execute_console_command(unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world(),'QUIT_EDITOR')
def tick(dt):
 try:
  s=q['stage'];schedule=[4,14,19,24,29,34,39,44,49,54,59,64]
  if s>=len(schedule) or time.monotonic()-started<schedule[s]:return
  if s==0:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
  else:
   ws=worlds();h=next(w for w in ws if unreal.GameplayStatics.get_game_mode(w));cs=sorted([w for w in ws if w!=h],key=lambda w:w.get_path_name())
   if s==1:
    q['checks']['four_players']=len(ws)==4
    queue(cs[0],'GridTruck13',(4800,4000),(4800,5300))
   elif s==2:
    q['checks']['custom_road_reconstructed_on_server_and_peers']=agree() and all('TruckAccess_1300_1300 X=4150 Y=4000' in desc(w,(4800,4500)) for w in ws)
    N.queue_logistics_action(pc(cs[1]),'NetPanel',unreal.Vector(),False)
    N.queue_logistics_action(pc(cs[1]),'NetDraw',unreal.Vector(4800,4500,0),True)
   elif s==3:
    q['checks']['world_grid_node_on_half_metre_road_origin_accepted']=pc(cs[1]).get_last_construction_error()==0 and agree() and all(pc(w).get_network_summary().split('/')[1]=='1' for w in ws)
    queue(cs[2],'Parking',(1000,4000),(2200,5200))
   elif s==4:
    q['checks']['parking_created']=agree() and all('Parking_1200_1200' in desc(w,(1500,4500)) for w in ws)
    queue(cs[0],'Resize',(2100,5100),(2200,4600))
   elif s==5:
    q['checks']['parking_rotation_resize_shared']=agree() and all('Parking_600_1200 X=2200 Y=4000' in desc(w,(1500,4300)) and desc(w,(1500,4900))=='EMPTY' for w in ws)
    queue(cs[1],'Resize',(3900,3500),(5200,3600))
   elif s==6:
    q['checks']['starter_yard_can_expand_in_multiplayer']=agree() and all('FreightYard_5200_3600' in desc(w,(4500,2500)) for w in ws)

   elif s==7:
    q['checks']['expanded_yard_converged']=agree()
    N.queue_truck_zone(pc(cs[2]),unreal.IntVector(0,0,0),unreal.IntVector(5200,3600,0),False)
   elif s==8:
    q['checks']['driving_paint_shared']=agree() and all(N.truck_point_allowed(pc(w),unreal.Vector(5100,1000,0)) for w in ws)
    queue(cs[0],'Resize',(5100,3500),(5000,3400))
   elif s==9:
    q['checks']['painted_resize_accepted_everywhere']=agree() and all('FreightYard_5000_3400' in desc(w,(4500,2500)) for w in ws)
    q['checks']['permissions_clipped_everywhere']=all(N.truck_point_allowed(pc(w),unreal.Vector(4900,1000,0)) and not N.truck_point_allowed(pc(w),unreal.Vector(5100,1000,0)) for w in ws)
    q['checks']['late_join_requested']=N.request_late_join()
   elif s==10:
    q['checks']['fifth_player_receives_grid_edits']=len(ws)==5 and agree() and all('Parking_600_1200' in desc(w,(1500,4300)) and pc(w).get_network_summary().split('/')[1]=='1' for w in ws)
    q['checks']['client_direct_mutation_remains_denied']=N.client_direct_execution_blocked(pc(cs[0]))
   elif s==11:done();return
  q['stage']+=1;write()
 except Exception:q['errors'].append(traceback.format_exc());done()
handle=unreal.register_slate_post_tick_callback(tick);write()
