"""Rendered client overlay review. Temporary PIE data only; no map/save writes."""
import unreal,time
started=time.monotonic();stage=0
N=unreal.FoundrylineNetworkTestTools
def tick(dt):
 global stage
 t=time.monotonic()-started
 if stage==0 and t>8:
  unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1
 if stage==1 and t>20:
  for w in unreal.EditorLevelLibrary.get_pie_worlds(False):
   if unreal.GameplayStatics.get_game_mode(w) is None:
    p=next((p for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller()),None)
    if p:
     unreal.SystemLibrary.set_timer_for_next_tick(p,'ToggleLogisticsEdit');N.queue_truck_zone(p,unreal.IntVector(-4000,-2400,0),unreal.IntVector(6000,5300,0),False);stage=2
 if stage==2 and t>26:
  for w in unreal.EditorLevelLibrary.get_pie_worlds(False):
   p=next((p for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller()),None)
   if p:unreal.log('FOUNDRYLINE_LOGISTICS_UI '+N.driving_inventory(p)+' overlay='+str(p.is_logistics_overlay_visible())+' error='+str(p.get_last_construction_error()))
  unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
