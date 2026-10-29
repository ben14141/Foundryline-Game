"""Review dock-connected tanker assembly, enclosed supply bays, and staged tractor."""
import unreal,time,json,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
base=Path(unreal.Paths.project_saved_dir());out=base/'Screenshots/Reviews';out.mkdir(parents=True,exist_ok=True)
result={'checks':{},'errors':[]};start=time.monotonic();stage=0;last=0
def pc():return next(p for w in unreal.EditorLevelLibrary.get_pie_worlds(False) for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller())
def plant():return unreal.GameplayStatics.get_all_actors_of_class(pc().get_world(),unreal.FoundrylineFactoryPlant)[0]
def report():return json.loads(plant().get_factory_report())
def act(a):unreal.FoundrylineNetworkTestTools.queue_factory_action(pc(),a)
def camera():
 unreal.FoundrylineNetworkTestTools.setup_workshop_review(pc(),True)
 c=pc().get_view_target();location=unreal.Vector(2200,6500,7000);target=unreal.Vector(800,2300,100)
 c.set_actor_location(location,False,False);c.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(location,target),False)
def shot(name):unreal.AutomationLibrary.take_high_res_screenshot(1920,1200,str(out/('Dockside_'+name+'.png')))
def tick(dt):
 global stage,last
 try:
  t=time.monotonic()-start
  if stage==0 and t>3:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1;last=t
  elif stage==1 and t-last>14:
   camera();act('Toggle');act('Overlay');act('Page2');stage=2;last=t
  elif stage==2 and t-last>3:
   result['checks']['starter_stock_present']=report()['pallets']==23
   rigs=unreal.GameplayStatics.get_all_actors_of_class(pc().get_world(),unreal.FoundrylineFreightRig)
   result['checks']['tractor_only_at_loading_door']=len(rigs)==1 and rigs[0].get_editor_property('bTractorOnly') and abs(rigs[0].get_actor_location().x-2950)<1 and abs(rigs[0].get_actor_location().y-1650)<1
   shot('Zone_Hologram');stage=3;last=t
  elif stage==3 and t-last>3:act('Page0');act('Order');act('Speed');stage=4;last=t
  elif stage==4 and t-last>2:act('Speed');stage=5;last=t
  elif stage==5 and t-last>.15:
   if report()['mask']==63:act('Run');stage=6;last=t
   else:unreal.FoundrylineNetworkTestTools.advance_factory_test(pc(),200);last=t
  elif stage==6 and t-last>3:result['checks']['dockside_production_completes']=report()['mask']==63;shot('Completed_Tanker');stage=7;last=t
  elif stage==7 and t-last>3:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_end_play();stage=8;last=t
  elif stage==8 and t-last>3:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=9;last=t
  elif stage==9 and t-last>14:act('Toggle');act('Page2');act('Overlay');act('Save');stage=10;last=t
  elif stage==10 and t-last>3:
   result['checks']['fresh_stocked_starter_saved']=report()['mask']==0 and report()['pallets']==23 and not report()['running'] and not pc().get_factory_feedback()
   result['completed']=True;(base/'Logs/DocksideZoneVisualQA.json').write_text(json.dumps(result,indent=2));unreal.unregister_slate_post_tick_callback(handle);return
  if t>240:raise RuntimeError('Dockside review timeout at '+str(stage))
  (base/'Logs/DocksideZoneVisualQA.json').write_text(json.dumps({**result,'stage':stage},indent=2))
 except Exception:
  result['errors'].append(traceback.format_exc());(base/'Logs/DocksideZoneVisualQA.json').write_text(json.dumps(result,indent=2));unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
