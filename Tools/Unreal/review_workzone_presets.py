"""Disposable PIE placement review. Never edits the saved map or plant save slot."""
import unreal,time,json,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
base=Path(unreal.Paths.project_saved_dir());out=base/'Logs/WorkZonePresetVisualQA.json'
start=time.monotonic();stage=0;last=0;lastshot=0;report={'checks':{},'errors':[]}
def players():return [p for w in unreal.EditorLevelLibrary.get_pie_worlds(False) for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller()]
def act(p,a):unreal.FoundrylineNetworkTestTools.queue_factory_action(p,a)
def tick(dt):
 global stage,last,lastshot
 try:
  t=time.monotonic()-start
  if stage==0 and t>3:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1;last=t
  elif stage==1 and t-last>14:
   p=players()[0];unreal.FoundrylineNetworkTestTools.prepare_empty_factory_zones(p);unreal.FoundrylineNetworkTestTools.setup_workshop_review(p,True);act(p,'Toggle');act(p,'Page2');act(p,'ToolZone');stage=2;last=t
  elif stage==2 and t-last>2:
   p=players()[0];report['zones']=unreal.FoundrylineNetworkTestTools.inspect_factory_zones(p);report['feedback']=p.get_factory_feedback()
   if t-lastshot>10:
    unreal.AutomationLibrary.take_high_res_screenshot(1920,1200,str(base/'Screenshots/Reviews/Factory_WorkZone_Presets.png'));lastshot=t
   if report['zones'].startswith('1/'):report['checks']['placement_created_zone']=True
   if t>180:
    report['completed']=True;unreal.unregister_slate_post_tick_callback(handle)
  out.write_text(json.dumps({**report,'stage':stage},indent=2))
 except Exception:
  report['errors'].append(traceback.format_exc());out.write_text(json.dumps(report,indent=2));unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
