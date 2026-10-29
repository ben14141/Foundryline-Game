"""Leave the plasma prototype running for visual review without saving map changes."""
import unreal,time,json
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
start=time.monotonic();stage=0
def tick(dt):
 global stage
 t=time.monotonic()-start
 if stage==0 and t>3:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1
 elif stage==1 and t>12:
  for w in unreal.EditorLevelLibrary.get_pie_worlds(False):
   for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController):
    if p.is_local_controller():
     unreal.FoundrylineNetworkTestTools.setup_plasma_review(p,False)
   for cls in [unreal.FoundrylinePlasmaTable,unreal.FoundrylineMaterialHandlingCell,unreal.FoundrylinePressBrakeDisplay]:
    for actor in unreal.GameplayStatics.get_all_actors_of_class(w,cls):
     for area in actor.get_components_by_class(unreal.FoundrylineEquipmentFootprintComponent):area.show_placement_area(True)
  stage=2
 elif stage==2 and t>20:
  out=Path(unreal.Paths.project_saved_dir())/'Screenshots/Reviews';out.mkdir(parents=True,exist_ok=True)
  unreal.AutomationLibrary.take_high_res_screenshot(1600,1000,str(out/'PlasmaTable_GridAreas.png'))
  stage=3
 elif stage==3 and t>22:
  for w in unreal.EditorLevelLibrary.get_pie_worlds(False):
   for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController):
    if p.is_local_controller():unreal.FoundrylineNetworkTestTools.setup_plasma_review(p,True)
  stage=4
 elif stage==4 and t>24:
  out=Path(unreal.Paths.project_saved_dir())/'Screenshots/Reviews'
  unreal.AutomationLibrary.take_high_res_screenshot(1600,1000,str(out/'PlasmaTable_CuttingDetail.png'));stage=5
 elif stage==5 and t>27:
  for w in unreal.EditorLevelLibrary.get_pie_worlds(False):
   for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController):
    if p.is_local_controller():unreal.FoundrylineNetworkTestTools.setup_plasma_review(p,False)
   for cls in [unreal.FoundrylinePlasmaTable,unreal.FoundrylineMaterialHandlingCell,unreal.FoundrylinePressBrakeDisplay]:
    for actor in unreal.GameplayStatics.get_all_actors_of_class(w,cls):
     for area in actor.get_components_by_class(unreal.FoundrylineEquipmentFootprintComponent):area.show_placement_area(False)
  unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
