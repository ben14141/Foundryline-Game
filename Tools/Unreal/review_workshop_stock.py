"""Verify real starter stock, capture shelf/bay placement, and leave it playable."""
import unreal,time,json,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
base=Path(unreal.Paths.project_saved_dir());out=base/'Screenshots/Reviews';out.mkdir(parents=True,exist_ok=True)
result={'checks':{},'errors':[]};started=time.monotonic();stage=0;last=0
def pc():
 return next(p for w in unreal.EditorLevelLibrary.get_pie_worlds(False) for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller())
def plant():return unreal.GameplayStatics.get_all_actors_of_class(pc().get_world(),unreal.FoundrylineFactoryPlant)[0]
def act(a):unreal.FoundrylineNetworkTestTools.queue_factory_action(pc(),a)
def shot(name):unreal.AutomationLibrary.take_high_res_screenshot(1920,1200,str(out/('Stocked_'+name+'.png')))
def tick(dt):
 global stage,last
 try:
  t=time.monotonic()-started
  if stage==0 and t>3:
   unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1;last=t
  elif stage==1 and t-last>14:
   unreal.FoundrylineNetworkTestTools.setup_workshop_review(pc(),True);act('Toggle');act('Page1');stage=2;last=t
  elif stage==2 and t-last>3:
   state=json.loads(plant().get_factory_report());result['checks']['twenty_three_real_pallets']=state['pallets']==23;result['checks']['paused_for_testing']=not state['running']
   names=['SteelSheet_60x120','BracketBlank','RailBlank','FrameRail','ShellBlank','ShellSection','TankHead_ASME_FD','RunningGearKit','FittingsKit','BentBracket'];counts={n:0 for n in names}
   for c in plant().get_components_by_class(unreal.InstancedStaticMeshComponent):
    mesh=c.get_editor_property('static_mesh')
    if mesh:
     name=mesh.get_name().removeprefix('SM_')
     if name in counts:counts[name]+=c.get_instance_count()
   result['visible_parts']=counts;result['checks']['all_ten_part_types_visible']=all(counts.values());shot('Workshop');stage=3;last=t
  elif stage==3 and t-last>3:
   camera=pc().get_view_target();location=unreal.Vector(2500,4400,2300);target=unreal.Vector(1050,2350,160)
   camera.set_actor_location(location,False,False);camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(location,target),False);act('Toggle');stage=4;last=t
  elif stage==4 and t-last>3:
   shot('Rack_Detail');stage=5;last=t
  elif stage==5 and t-last>3:
   unreal.FoundrylineNetworkTestTools.setup_workshop_review(pc(),True);stage=6;last=t
  elif stage==6 and t-last>3:
   shot('Parts_Staging');act('Save');stage=7;last=t
  elif stage==7 and t-last>3:
   result['checks']['stocked_save_no_error']=not pc().get_factory_feedback();pc().set_view_target_with_blend(pc().get_controlled_pawn(),0);stage=8;last=t
  elif stage==8 and t-last>3:
   result['checks']['playable_camera_restored']=pc().get_view_target()==pc().get_controlled_pawn();result['completed']=True;(base/'Logs/WorkshopStockVisualQA.json').write_text(json.dumps(result,indent=2));unreal.unregister_slate_post_tick_callback(handle);return
  if t>150:raise RuntimeError('Stock review timed out at '+str(stage))
  (base/'Logs/WorkshopStockVisualQA.json').write_text(json.dumps({**result,'stage':stage},indent=2))
 except Exception:
  result['errors'].append(traceback.format_exc());(base/'Logs/WorkshopStockVisualQA.json').write_text(json.dumps(result,indent=2));unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
