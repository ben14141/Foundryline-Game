"""Four-player property QA: initial sync, host edits, late join, door RPC and host load.
Run with 3 initial PIE players; native editor late join adds the fourth.
Uses a unique temporary save slot, never Property01. Native timers preserve RPC callspace.
"""
import unreal,time,json,traceback,uuid
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
if not (root/'Foundryline.uproject').is_file(): raise RuntimeError('Foundryline only')
report_path=root/'Saved/Logs/SharedPropertyAcceptance.json'
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
slot='NetworkQA_'+uuid.uuid4().hex[:16]
qa={'started':time.monotonic(),'stage':0,'actions':[],'samples':[],'errors':[],'completed':False,'slot':slot,'render_mode':'NullRHI','fixture_note':'Door range/LOS setup teleports both test pawns to controlled positions; movement prediction is not part of this test.'}
def write_report(): report_path.write_text(json.dumps({k:v for k,v in qa.items() if k!='started'},indent=2))
def sample_world(world):
 actors=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.Actor)
 controllers=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.PlayerController)
 rows=[]
 for pc in controllers:
  pawn=pc.get_controlled_pawn()
  rows.append({'name':pc.get_name(),'local':pc.is_local_controller(),'pawn':pawn.get_class().get_name() if pawn else None,'pawn_location':str(pawn.get_actor_location()) if pawn else None,'view_target':pc.get_view_target().get_class().get_name() if pc.get_view_target() else None,'roofs_requested':pc.are_roofs_shown()})
 construction=[a for a in actors if a.get_class().get_name()=='Actor']
 mesh_count=0;instance_count=0;doors=[];lights=0
 for actor in construction:
  for c in actor.get_components_by_class(unreal.StaticMeshComponent):
   mesh_count+=int(c.static_mesh is not None)
   if isinstance(c,unreal.InstancedStaticMeshComponent): instance_count+=c.get_instance_count()
   if c.get_class().get_name()=='FoundrylinePersonnelDoorComponent': doors.append({'name':c.get_path_name(),'mesh':c.static_mesh.get_name() if c.static_mesh else None,'rotation':str(c.get_editor_property('relative_rotation'))})
  lights+=len(actor.get_components_by_class(unreal.RectLightComponent))
 game_state=unreal.GameplayStatics.get_game_state(world)
 return {'world':world.get_name(),'has_game_mode':unreal.GameplayStatics.get_game_mode(world) is not None,'player_count':len(game_state.get_editor_property('player_array')) if game_state else 0,'controllers':rows,'construction_actors':len(construction),'replicates_flag_count':sum(a.get_editor_property('replicates') for a in construction),'construction_mesh_components':mesh_count,'construction_instances':instance_count,'ceiling_area_lights':lights,'walk_characters':sum(a.get_class().get_name()=='FoundrylineWalkCharacter' for a in actors),'visible_roof_components':sum(c.is_visible() for a in construction for c in a.get_components_by_class(unreal.SceneComponent) if c.component_has_tag('FoundrylineRoof')),'doors':doors,'build_site_actors':sum(a.get_class().get_name()=='FoundrylineBuildSite' for a in actors)}
def inventory_world(world):
 summary=sample_world(world)
 mesh_groups={};door_records=[];fixtures=0;plant_instances=0
 for actor in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.Actor):
  if actor.get_class().get_name()!='Actor': continue
  anchor=actor.get_actor_location();rotation=actor.get_actor_rotation()
  for component in actor.get_components_by_class(unreal.StaticMeshComponent):
   mesh=component.static_mesh
   if not mesh: continue
   count=component.get_instance_count() if isinstance(component,unreal.InstancedStaticMeshComponent) else 1
   key=mesh.get_path_name()
   mesh_groups[key]=mesh_groups.get(key,0)+count
   if component.component_has_tag('FoundrylineCeilingFixture'): fixtures+=count
   if component.component_has_tag('FoundrylineParkingPlant'): plant_instances+=count
   if component.get_class().get_name()=='FoundrylinePersonnelDoorComponent':
    local=component.get_editor_property('relative_location');angles=component.get_editor_property('relative_rotation')
    door_records.append({'mesh':key,'actor_anchor_cm':[round(anchor.x,2),round(anchor.y,2),round(anchor.z,2)],'actor_yaw':round(rotation.yaw,2),'relative_location_cm':[round(local.x,2),round(local.y,2),round(local.z,2)],'relative_yaw':round(angles.yaw,2),'open':component.is_open()})
 summary.update(mesh_inventory=mesh_groups,ceiling_fixture_meshes=fixtures,parking_plant_instances=plant_instances,replicated_component_records=sorted(door_records,key=lambda v:json.dumps(v,sort_keys=True)),door_records=sorted([v for v in door_records if v['mesh'].rsplit('/',1)[-1].startswith('SM_Door_')],key=lambda v:json.dumps(v,sort_keys=True)))
 return summary


def worlds_now(): return list(unreal.EditorLevelLibrary.get_pie_worlds(False))
def site_in(world): return next((a for a in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.Actor) if a.get_class().get_name()=='FoundrylineBuildSite'),None)
def local_pc(world): return next((pc for pc in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.PlayerController) if pc.is_local_controller()),None)
def choose_worlds():
 worlds=worlds_now();server=next(w for w in worlds if unreal.GameplayStatics.get_game_mode(w) is not None)
 clients=sorted([w for w in worlds if unreal.GameplayStatics.get_game_mode(w) is None],key=lambda w:w.get_path_name())
 if 'target_client' not in qa: qa['target_client']=clients[0].get_path_name()
 return server,next(w for w in clients if w.get_path_name()==qa['target_client'])
def door_fixture(server,client,closing):
 server_pc=next(pc for pc in unreal.GameplayStatics.get_all_actors_of_class(server,unreal.PlayerController) if not pc.is_local_controller() and pc.get_controlled_pawn() and pc.get_controlled_pawn().get_class().get_name()=='FoundrylineWalkCharacter')
 door=next(c for a in unreal.GameplayStatics.get_all_actors_of_class(server,unreal.Actor) for c in a.get_components_by_class(unreal.StaticMeshComponent) if c.get_class().get_name()=='FoundrylinePersonnelDoorComponent' and c.static_mesh and c.static_mesh.get_name()=='SM_Door_A1')
 center,extent,radius=unreal.SystemLibrary.get_component_bounds(door)
 normal=unreal.Vector(1,0,0) if extent.x<extent.y else unreal.Vector(0,1,0)
 anchor=door.get_owner().get_actor_location();offset=door.get_editor_property('relative_location')
 position=unreal.Vector(center.x-normal.x*140,center.y-normal.y*140,anchor.z+offset.z+90)
 eye=unreal.Vector(position.x,position.y,position.z+160)
 facing=unreal.MathLibrary.find_look_at_rotation(eye,center)
 client_pc=local_pc(client)
 for pc in [server_pc,client_pc]:
  pc.get_controlled_pawn().set_actor_location(position,False,True);pc.set_control_rotation(facing)
 return True

def snapshot():
 result=[]
 for world in worlds_now():
  row=inventory_world(world);site=site_in(world)
  row['interaction_diagnostics']=unreal.FoundrylineNetworkTestTools.inspect_player(local_pc(world));row['world_path']=world.get_path_name();row['property_digest']=site.get_property_digest() if site else 'NO_SITE';row['property_revision']=site.get_property_revision() if site else -1
  result.append(row)
 return {'seconds':round(time.monotonic()-qa['started'],1),'worlds':result}
def finish_qa():
 qa['completed']=True
 checks={}
 for snap in qa['samples']:
  worlds=snap['worlds'];host=next((w for w in worlds if w['has_game_mode']),None)
  if not host: continue
  checks[str(snap['seconds'])]={'players':host['player_count'],'worlds':len(worlds),'records_match':all(w['property_digest']==host['property_digest'] for w in worlds),'instances_match':all(w['construction_instances']==host['construction_instances'] for w in worlds),'meshes_match':all(w['mesh_inventory']==host['mesh_inventory'] for w in worlds),'lights_match':all(w['ceiling_area_lights']==host['ceiling_area_lights'] for w in worlds),'doors_match':all(w['door_records']==host['door_records'] for w in worlds),'local_roofs_match':all((w['visible_roof_components']>0)==next(c['roofs_requested'] for c in w['controllers'] if c['local']) for w in worlds)}
 qa['checks']=checks
 qa['door_close_observed']=any(not d['open'] for s in qa['samples'] if 30<s['seconds']<50 for w in s['worlds'] for d in w['door_records'] if 'SM_Door_A1' in d['mesh'])
 qa['door_reopen_observed']=qa['door_close_observed'] and all(d['open'] for s in qa['samples'] if 60<s['seconds']<65 for w in s['worlds'] for d in w['door_records'] if 'SM_Door_A1' in d['mesh'])
 qa['save_file_cleaned']=unreal.GameplayStatics.delete_game_in_slot('Foundryline_'+slot,0)
 unreal.unregister_slate_post_tick_callback(network_qa_handle);write_report();unreal.log('FOUNDRYLINE_NETWORK_QA_DONE '+str(report_path))
def tick_qa(delta):
 try:
  elapsed=time.monotonic()-qa['started']
  if qa['stage']==0 and elapsed>3: levels.editor_request_begin_play();qa['stage']=1;return
  if qa['stage']==0: return
  schedule=[18,24,26,28,34,40,46,52,54,66,76,86,96]
  index=len(qa['actions'])
  if index<len(schedule) and elapsed>schedule[index]:
   server,client=choose_worlds();host=site_in(server);remote=site_in(client);success=True
   if index==0: unreal.SystemLibrary.set_timer_for_next_tick(local_pc(client),'ToggleWalk');label='Client enters first person'
   elif index==1: host.save_property(slot);label='Save baseline to isolated QA slot'
   elif index==2: success=door_fixture(server,client,True);label='Place test player near open shop door'
   elif index==3: unreal.SystemLibrary.set_timer_for_next_tick(local_pc(client),'UseDoor');label='Client E closes door through server RPC'
   elif index==4:
    success=host.build_rectangle('Building',unreal.IntVector(1000,4000,0),unreal.IntVector(1900,5300,0))
    success=host.begin_edit('Select',unreal.IntVector(1400,4500,0)) and success
    success=host.paint_selected(1,unreal.LinearColor(0.1,0.2,0.6,1)) and success;label='Host builds custom shop and paints interior'
   elif index==5: success=unreal.FoundrylineNetworkTestTools.request_late_join();label='Fourth player joins after edits'
   elif index==6:
    before=remote.get_property_revision();denied=not remote.build_rectangle('Building',unreal.IntVector(2200,4200,0),unreal.IntVector(3000,5000,0));success=denied and before==remote.get_property_revision();label='Direct client state mutation rejected'
   elif index==7: success=door_fixture(server,client,False);label='Place test player near closed shop door'
   elif index==8: unreal.SystemLibrary.set_timer_for_next_tick(local_pc(client),'UseDoor');label='Client E reopens door through server RPC'
   elif index==9:
    success=unreal.GameplayStatics.does_save_game_exist('Foundryline_'+slot,0);host.load_property(slot);label='Host loads baseline while three clients connected'
   elif index==10: unreal.SystemLibrary.set_timer_for_next_tick(local_pc(client),'ToggleWalk');label='Client returns to top down'
   elif index==11: unreal.SystemLibrary.set_timer_for_next_tick(local_pc(server),'ToggleWalk');label='Host walks while clients stay top down'
   elif index==12: unreal.SystemLibrary.set_timer_for_next_tick(local_pc(server),'ToggleWalk');label='Host returns to top down'
   qa['actions'].append({'seconds':round(elapsed,1),'label':label,'accepted':success});write_report()
  if elapsed>=12+len(qa['samples'])*10 and len(qa['samples'])<10:
   qa['samples'].append(snapshot());write_report()
  if elapsed>108: finish_qa()
 except Exception:
  qa['errors'].append(traceback.format_exc());unreal.log_error(qa['errors'][-1]);finish_qa()
network_qa_handle=unreal.register_slate_post_tick_callback(tick_qa)
write_report();unreal.log('FOUNDRYLINE_NETWORK_QA_ARMED')
