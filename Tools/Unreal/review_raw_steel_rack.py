"""Front view of the actual receiving rack and its shared sheet inventory."""
import unreal,time
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
start=time.monotonic();stage=0
def tick(dt):
 global stage
 t=time.monotonic()-start
 if stage==0 and t>3:stage=1;unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
 elif stage==1 and t>12:
  stage=2
  for w in unreal.EditorLevelLibrary.get_pie_worlds(False):
   for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController):
    if p.is_local_controller():
     unreal.FoundrylineNetworkTestTools.setup_workshop_review(p,True);camera=p.get_view_target();a=unreal.Vector(1150,1500,460);b=unreal.Vector(650,1500,200);camera.set_actor_location_and_rotation(a,unreal.MathLibrary.find_look_at_rotation(a,b),False,True);camera.get_components_by_class(unreal.CameraComponent)[0].set_field_of_view(70)
   unreal.SystemLibrary.execute_console_command(w,'showhud')
 elif stage==2 and t>19:
  stage=3;out=Path(unreal.Paths.project_saved_dir())/'Screenshots/Reviews/ProductionWorkshop_RawSteelRack.png';unreal.AutomationLibrary.take_high_res_screenshot(1600,1000,str(out))
 elif stage==3 and t>23:
  stage=4
  for w in unreal.EditorLevelLibrary.get_pie_worlds(False):
   unreal.SystemLibrary.execute_console_command(w,'showhud')
   for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController):
    if p.is_local_controller():unreal.FoundrylineNetworkTestTools.setup_workshop_review(p,True)
  unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
