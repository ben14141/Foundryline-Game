"""Read-only property replication acceptance probe, run in two-player PIE with NullRHI."""
import unreal,time,json,traceback
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
if not (root/'Foundryline.uproject').is_file(): raise RuntimeError('Foundryline only')
report_path=root/'Saved/Logs/SharedPropertyProbe.json'
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
probe={'started':time.monotonic(),'stage':0,'samples':[],'errors':[],'completed':False,'read_only':True,'render_mode':'NullRHI'}
def write_report(): report_path.write_text(json.dumps({k:v for k,v in probe.items() if k!='started'},indent=2))
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
    door_records.append({'mesh':key,'actor_anchor_cm':[round(anchor.x,2),round(anchor.y,2),round(anchor.z,2)],'actor_yaw':round(rotation.yaw,2),'relative_location_cm':[round(local.x,2),round(local.y,2),round(local.z,2)],'relative_yaw':round(angles.yaw,2)})
 summary.update(mesh_inventory=mesh_groups,ceiling_fixture_meshes=fixtures,parking_plant_instances=plant_instances,replicated_component_records=sorted(door_records,key=lambda v:json.dumps(v,sort_keys=True)),door_records=sorted([v for v in door_records if v['mesh'].rsplit('/',1)[-1].startswith('SM_Door_')],key=lambda v:json.dumps(v,sort_keys=True)))
 return summary

def finish_shared():
 probe['completed']=True
 if probe['samples']:
  worlds=probe['samples'][-1]['worlds']
  host=next((w for w in worlds if w['has_game_mode']),None)
  client=next((w for w in worlds if not w['has_game_mode']),None)
  if host and client:
   probe['checks']={'both_players_connected':host['player_count']==2 and client['player_count']==2,'generated_instances_match':host['construction_instances']>0 and host['construction_instances']==client['construction_instances'],'ceiling_lights_match':host['ceiling_area_lights']>0 and host['ceiling_area_lights']==client['ceiling_area_lights'],'mesh_inventory_matches':host['mesh_inventory']==client['mesh_inventory'],'door_mesh_and_transforms_match':host['door_records']==client['door_records'],'client_has_construction_service_actor':client['build_site_actors']>0}
   probe['missing_mesh_instances']={name:count-client['mesh_inventory'].get(name,0) for name,count in host['mesh_inventory'].items() if count>client['mesh_inventory'].get(name,0)}
 unreal.unregister_slate_post_tick_callback(shared_probe_handle)
 write_report();unreal.log('FOUNDRYLINE_SHARED_PROPERTY_DONE '+str(report_path))
def tick_shared(delta):
 try:
  elapsed=time.monotonic()-probe['started']
  if probe['stage']==0 and elapsed>3:
   levels.editor_request_begin_play();probe['stage']=1;return
  if probe['stage']==1 and elapsed>=12+len(probe['samples'])*10 and len(probe['samples'])<3:
   worlds=unreal.EditorLevelLibrary.get_pie_worlds(False)
   probe['samples'].append({'seconds':round(elapsed,1),'worlds':[inventory_world(w) for w in worlds]});write_report()
  if elapsed>35: finish_shared()
 except Exception:
  probe['errors'].append(traceback.format_exc());unreal.log_error(probe['errors'][-1]);finish_shared()
shared_probe_handle=unreal.register_slate_post_tick_callback(tick_shared)
write_report();unreal.log('FOUNDRYLINE_SHARED_PROPERTY_ARMED')
