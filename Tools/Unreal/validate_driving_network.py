"""Four-player routing metadata, local overlay and save/load; fifth late join.
Writes and removes one unique DrivingQA save, never a user save or map.
"""
import unreal,time,json,traceback,uuid
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
if not (root/'Foundryline.uproject').is_file():raise RuntimeError('Foundryline only')
report=root/'Saved/Logs/DrivingNetwork.json'
slot='DrivingQA_'+uuid.uuid4().hex[:16]
qa={'started':time.monotonic(),'stage':0,'action':0,'checks':{},'samples':[],'errors':[],'completed':False,'slot':slot}
N=unreal.FoundrylineNetworkTestTools
point=unreal.Vector(1500,1500,0)
def write():report.write_text(json.dumps({k:v for k,v in qa.items() if k!='started'},indent=2))
def worlds():return list(unreal.EditorLevelLibrary.get_pie_worlds(False))
def pc(w):return next(p for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller())
def site(w):return next(a for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.Actor) if a.get_class().get_name()=='FoundrylineBuildSite')
def setup():
 ws=worlds();host=next(w for w in ws if unreal.GameplayStatics.get_game_mode(w));return host,sorted([w for w in ws if w!=host],key=lambda w:w.get_path_name())
def actor_paths(w):return sorted(a.get_path_name() for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.Actor) if a.get_class().get_name()=='Actor')
def sample(label):
 rows=[{'world':w.get_path_name(),'property':site(w).get_property_digest(),'driving':N.driving_inventory(pc(w)),'overlay':pc(w).is_logistics_overlay_visible(),'editing':pc(w).is_editing_logistics(),'truck_point':N.truck_point_allowed(pc(w),point),'error':pc(w).get_last_construction_error()} for w in worlds()]
 qa['samples'].append({'label':label,'worlds':rows});qa['checks'][label+'_converged']=len(set(r['property'] for r in rows))==1 and len(set(r['driving'] for r in rows))==1;write();return rows
def finish():
 qa['completed']=True;qa['save_cleaned']=unreal.GameplayStatics.delete_game_in_slot('Foundryline_'+slot,0);write();unreal.unregister_slate_post_tick_callback(handle);unreal.log('FOUNDRYLINE_DRIVING_NETWORK_DONE');unreal.SystemLibrary.execute_console_command(unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world(),'QUIT_EDITOR')
def tick(dt):
 try:
  t=time.monotonic()-qa['started']
  if qa['stage']==0 and t>3:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();qa['stage']=1;return
  schedule=[15,18,21,24,27,30,34,37,40,45,48]
  i=qa['action']
  if qa['stage'] and i<len(schedule) and t>schedule[i]:
   host,cs=setup()
   if i==0:
    rows=sample('initial');qa['checks']['four_players']=len(rows)==4;qa['checks']['hidden_by_default']=not any(r['overlay'] or r['editing'] for r in rows);qa['actors_before']={w.get_path_name():actor_paths(w) for w in worlds()};unreal.SystemLibrary.set_timer_for_next_tick(pc(cs[0]),'ToggleLogisticsEdit')
   elif i==1:
    rows=sample('local_overlay');qa['checks']['one_player_editing']=sum(r['editing'] for r in rows)==1 and sum(r['overlay'] for r in rows)==1;N.queue_truck_zone(pc(cs[0]),unreal.IntVector(-4000,-2400,0),unreal.IntVector(6000,5300,0),False)
   elif i==2:
    rows=sample('paint');qa['checks']['client_paint_accepted']=pc(cs[0]).get_last_construction_error()==0 and all(r['truck_point'] for r in rows);qa['checks']['metadata_did_not_rebuild_mesh_actors']=all(actor_paths(w)==qa['actors_before'][w.get_path_name()] for w in worlds());qa['checks']['save_accepted']=N.test_driving_save(pc(host),slot,False)
   elif i==3:
    qa['checks']['save_completed']=not N.test_save_busy(pc(host)) and unreal.GameplayStatics.does_save_game_exist('Foundryline_'+slot,0);N.queue_truck_zone(pc(cs[1]),unreal.IntVector(1000,1000,0),unreal.IntVector(2000,2000,0),True)
   elif i==4:
    rows=sample('erase');qa['checks']['client_erase_accepted']=pc(cs[1]).get_last_construction_error()==0 and not any(r['truck_point'] for r in rows);qa['checks']['load_accepted']=N.test_driving_save(pc(host),slot,True)
   elif i==5:
    rows=sample('restored');qa['checks']['zones_restored']=not N.test_save_busy(pc(host)) and all(r['truck_point'] for r in rows);qa['checks']['public_street_requires_no_zone']=all(N.truck_point_allowed(pc(w),unreal.Vector(-5000,-3000,0)) for w in worlds())
   elif i==6:
    qa['before_public_edit']=site(host).get_property_revision();N.queue_truck_zone(pc(cs[2]),unreal.IntVector(-7000,-3200,0),unreal.IntVector(-5000,-2400,0),False)
   elif i==7:qa['checks']['public_street_not_modified']=site(host).get_property_revision()==qa['before_public_edit']
   elif i==8:qa['checks']['late_join_requested']=N.request_late_join()
   elif i==9:
    rows=sample('late_join');qa['checks']['fifth_player_received_zones']=len(rows)==5 and all(r['truck_point'] for r in rows);qa['checks']['late_join_overlay_local']=sum(r['overlay'] for r in rows)==1
   elif i==10:finish();return
   qa['action']+=1;write()
 except Exception:
  qa['errors'].append(traceback.format_exc());unreal.log_error(qa['errors'][-1]);finish()
handle=unreal.register_slate_post_tick_callback(tick);write()
