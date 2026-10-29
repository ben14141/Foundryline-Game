"""Two-player local PIE probe. Reports real network worlds; launch with temporary PIE settings and restore them externally."""
import unreal,time,json,traceback
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
if not (root/'Foundryline.uproject').is_file(): raise RuntimeError('Foundryline only')
report_path=root/'Saved/Logs/CoopProbe.json'
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
probe={'started':time.monotonic(),'stage':0,'samples':[],'restored_settings':False,'errors':[],'actions':[]}
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
def finish_probe():
 probe['completed']=True
 unreal.unregister_slate_post_tick_callback(coop_probe_handle)
 write_report();unreal.log('FOUNDRYLINE_COOP_PROBE_FINISHED '+str(report_path))
def tick_coop(delta):
 try:
  elapsed=time.monotonic()-probe['started']
  if probe['stage']==0 and elapsed>3:
   levels.editor_request_begin_play();probe['stage']=1;unreal.log('FOUNDRYLINE_COOP_PROBE_STARTED');return
  if probe['stage']>=1 and elapsed>=12+len(probe['samples'])*10 and len(probe['samples'])<12:
   worlds=unreal.EditorLevelLibrary.get_pie_worlds(False)
   probe['samples'].append({'seconds':round(elapsed,1),'worlds':[sample_world(w) for w in worlds]});write_report();unreal.log('FOUNDRYLINE_COOP_SAMPLE '+str(len(worlds)))
  times=[20,35,50,65]
  if len(probe['actions'])<len(times) and elapsed>times[len(probe['actions'])]:
   worlds=unreal.EditorLevelLibrary.get_pie_worlds(False)
   server=next((w for w in worlds if unreal.GameplayStatics.get_game_mode(w) is not None),None)
   client=next((w for w in worlds if unreal.GameplayStatics.get_game_mode(w) is None),None)
   index=len(probe['actions']);world=client if index in [0,3] else server
   if world:
    pc=next((pc for pc in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.PlayerController) if pc.is_local_controller()),None)
    if pc:
     unreal.SystemLibrary.set_timer_for_next_tick(pc,'ToggleWalk');probe['actions'].append({'seconds':round(elapsed,1),'action':['Client first person','Host first person','Host top down','Client top down'][index]});write_report()
  if elapsed>80: finish_probe()
 except Exception:
  probe['errors'].append(traceback.format_exc());unreal.log_error(probe['errors'][-1]);finish_probe()
coop_probe_handle=unreal.register_slate_post_tick_callback(tick_coop)
write_report();unreal.log('FOUNDRYLINE_COOP_PROBE_ARMED')
