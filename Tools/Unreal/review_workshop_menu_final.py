"""Final single-player screenshots after label cleanup; no gameplay or authored-map changes."""
import unreal,time,json,traceback
from pathlib import Path
r=Path(unreal.Paths.project_dir()).resolve();out=r/'Saved/Screenshots/Reviews';N=unreal.FoundrylineNetworkTestTools;start=time.monotonic();last=0;stage=0
unreal.EditorPythonScripting.set_keep_python_script_alive(True);unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/Foundryline/Maps/L_BasicShopStarter')
def pc():return next(p for w in unreal.EditorLevelLibrary.get_pie_worlds(False) for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller())
def shot(n):unreal.SystemLibrary.execute_console_command(pc(),'Shot filename="'+str(out/(n+'.png'))+'"')
def finish():unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.quit_editor()
def tick(dt):
 global stage,last
 try:
  now=time.monotonic()
  if now-start>90:raise RuntimeError('Final menu capture timeout')
  if stage==0 and now-start>3:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1;last=now
  elif stage==1 and now-last>12:N.setup_operations_review(pc(),1);stage=2;last=now
  elif stage==2 and now-last>2:shot('Workshop_Computer_Desk_Final');stage=3;last=now
  elif stage==3 and now-last>2:N.queue_commerce_action(pc(),'Office');stage=4;last=now
  elif stage==4 and now-last>2:N.queue_construction_tool(pc(),'WheelClick',unreal.IntVector(10,10,0),unreal.IntVector(),0,unreal.LinearColor(1,1,1,1));stage=4.5;last=now
  elif stage==4.5 and now-last>1:shot('Workshop_Computer_Home_Final');stage=5;last=now
  elif stage==5 and now-last>2:(r/'Saved/Logs/WorkshopComputerFinalUI.json').write_text(N.operations_ui_report(pc()));finish()
 except Exception:(r/'Saved/Logs/WorkshopComputerFinalError.txt').write_text(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
