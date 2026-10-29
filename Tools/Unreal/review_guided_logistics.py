"""Guided logistics visual review; temporary PIE data, no saves or map writes."""
import unreal,time
started=time.monotonic();stage=0
N=unreal.FoundrylineNetworkTestTools
def player():
 for w in unreal.EditorLevelLibrary.get_pie_worlds(False):
  if unreal.GameplayStatics.get_game_mode(w) is None:
   p=next((p for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller()),None)
   if p:return p
def tick(dt):
 global stage
 t=time.monotonic()-started
 if stage==0 and t>8:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1
 if stage==1 and t>22:
  p=player()
  if p:
   p.toggle_logistics_edit();N.queue_truck_zone(p,unreal.IntVector(-400,-2400,0),unreal.IntVector(6000,5300,0),False);stage=2
 if stage==2 and t>29:
  p=player();p.logistics_action('TripEntrance');p.select_logistics_point(unreal.Vector(2000,-2400,0));p.logistics_action('TripDestination');p.select_logistics_point(unreal.Vector(-400,2575,0));p.logistics_action('TripExit');p.select_logistics_point(unreal.Vector(2000,-2400,0));p.logistics_action('TripTest');stage=3
 if stage==3 and t>48:
  p=player();unreal.log('GUIDED_UI '+p.get_logistics_summary());unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
