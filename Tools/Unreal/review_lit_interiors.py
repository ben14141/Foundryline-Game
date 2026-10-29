"""Temporary native Unreal interior review cameras; saves no map changes."""
import unreal,time,traceback
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
if not (root/'Foundryline.uproject').is_file(): raise RuntimeError('Foundryline only')
out=root/'Saved/Screenshots/Sharing';out.mkdir(parents=True,exist_ok=True)
showcase='FoundrylineShowcase' in unreal.SystemLibrary.get_command_line()
views=[('Foundryline_Tier2_Lit_Interior',unreal.Vector(1500,900,180),unreal.Rotator(pitch=12,yaw=45,roll=0))] if showcase else [('Foundryline_Tier1_Lit_Interior',unreal.Vector(-2500,2200,180),unreal.Rotator(pitch=8,yaw=15,roll=0)),('Foundryline_Office_Lit_Interior',unreal.Vector(-2500,4400,180),unreal.Rotator(pitch=0,yaw=315,roll=0)),('Foundryline_Office_Workroom_Lit_Interior',unreal.Vector(-1700,4200,180),unreal.Rotator(pitch=-5,yaw=285,roll=0))]
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
state={'stage':0,'at':time.monotonic(),'index':0,'camera':None,'task':None}
def review_tick(delta):
 try:
  now=time.monotonic()
  if now-state['at']<8: return
  if state['stage']==0:
   levels.editor_request_begin_play();state.update(stage=1,at=now);return
  world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
  if not world: return
  pc=unreal.GameplayStatics.get_player_controller(world,0)
  if not pc: return
  if state['stage']==1:
   for actor in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.Actor):
    for component in actor.get_components_by_class(unreal.SceneComponent):
     if component.component_has_tag('FoundrylineRoof'): component.set_visibility(True)
   name,position,rotation=views[state['index']]
   transform=unreal.Transform(location=position,rotation=rotation)
   pawn=pc.get_controlled_pawn()
   pawn.set_actor_tick_enabled(False)
   camera=pawn.get_component_by_class(unreal.CameraComponent)
   camera.set_absolute(True,True,False)
   camera.set_world_transform(transform,False,True)
   camera.set_field_of_view(85)
   for hud in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.HUD): hud.set_editor_property('show_hud',False)
   state.update(stage=2,at=now,camera=camera);return
  if state['stage']==2:
   name=views[state['index']][0]
   state.update(stage=3,at=now)
   state['task']=unreal.AutomationLibrary.take_high_res_screenshot(1920,1080,str(out/(name+'.png')))
   unreal.log('FOUNDRYLINE_INTERIOR_REQUESTED '+name)
   state.update(stage=3,at=now);return
  if state['stage']==3:
   state['index']+=1
   if state['index']>=len(views):
    unreal.unregister_slate_post_tick_callback(review_handle);unreal.log('FOUNDRYLINE_INTERIOR_REVIEW_DONE');return
   state.update(stage=1,at=now)
 except Exception:
  unreal.log_error(traceback.format_exc());unreal.unregister_slate_post_tick_callback(review_handle)
review_handle=unreal.register_slate_post_tick_callback(review_tick)
unreal.log('FOUNDRYLINE_INTERIOR_REVIEW_ARMED')
