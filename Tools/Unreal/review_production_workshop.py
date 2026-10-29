"""Capture the new workshop, office, parts and incoming-steel areas; leave PIE open."""
import unreal,time,json
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
start=time.monotonic();stage=0
out=Path(unreal.Paths.project_saved_dir())/'Screenshots/Reviews';out.mkdir(parents=True,exist_ok=True)
def views(interior):
 for w in unreal.EditorLevelLibrary.get_pie_worlds(False):
  for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController):
   if p.is_local_controller():unreal.FoundrylineNetworkTestTools.setup_workshop_review(p,interior)
def focus(location,target):
 for w in unreal.EditorLevelLibrary.get_pie_worlds(False):
  for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController):
   if p.is_local_controller():
    camera=p.get_view_target();camera.set_actor_location_and_rotation(location,unreal.MathLibrary.find_look_at_rotation(location,target),False,True)
def tick(dt):
 global stage
 t=time.monotonic()-start
 if stage==0 and t>3:stage=1;unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
 elif stage==1 and t>12:
  stage=2;views(False)
  rows=[]
  for w in unreal.EditorLevelLibrary.get_pie_worlds(False):
   # Roofs already default hidden for the top-down local player.
   unreal.SystemLibrary.execute_console_command(w,'showhud')
   for c in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.FoundrylineMaterialHandlingCell):
    rows.append({'stock':[(str(s.place),s.count) for s in c.stock],'components':[(m.get_name(),list(m.get_world_location().to_tuple())) for m in c.get_components_by_class(unreal.SceneComponent) if m.get_name().startswith(('Stock','Pallet'))]})
  (Path(unreal.Paths.project_saved_dir())/'Logs/WorkshopStockVisuals.json').write_text(json.dumps(rows,indent=2))
 elif stage==2 and t>20:stage=3;unreal.AutomationLibrary.take_high_res_screenshot(1920,1200,str(out/'ProductionWorkshop_Overview.png'))
 elif stage==3 and t>23:stage=4;views(True)
 elif stage==4 and t>29:stage=5;unreal.AutomationLibrary.take_high_res_screenshot(1920,1200,str(out/'ProductionWorkshop_Interior.png'))
 elif stage==5 and t>32:stage=6;focus(unreal.Vector(-5500,1800,2300),unreal.Vector(-2900,3200,100))
 elif stage==6 and t>37:stage=7;unreal.AutomationLibrary.take_high_res_screenshot(1920,1200,str(out/'ProductionWorkshop_Office.png'))
 elif stage==7 and t>40:stage=8;focus(unreal.Vector(1050,1900,700),unreal.Vector(300,1500,140))
 elif stage==8 and t>45:stage=9;unreal.AutomationLibrary.take_high_res_screenshot(1920,1200,str(out/'ProductionWorkshop_Receiving.png'))
 elif stage==9 and t>48:
  stage=10;views(True)
  for w in unreal.EditorLevelLibrary.get_pie_worlds(False):unreal.SystemLibrary.execute_console_command(w,'showhud')
  unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
