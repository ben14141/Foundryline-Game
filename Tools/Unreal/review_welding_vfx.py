"""Open the local shop welding demonstration without saving map/property changes."""
import unreal,time
start=time.monotonic();stage=0
def tick(dt):
 global stage
 if stage==0 and time.monotonic()-start>3:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1
 elif stage==1 and time.monotonic()-start>10:
  for w in unreal.EditorLevelLibrary.get_pie_worlds(False):
   pawn=unreal.GameplayStatics.get_player_pawn(w,0)
   if pawn:pawn.focus_on_world_position(unreal.Vector(-775,3200,0))
   for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController):
    if p.is_local_controller():unreal.FoundrylineNetworkTestTools.setup_welding_review(p)
   unreal.SystemLibrary.execute_console_command(w,'viewmode lit')
  exec((__import__('pathlib').Path(unreal.Paths.project_dir())/'Tools/Unreal/inspect_welding_vfx.py').read_text())
  unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
