"""Review both tanker zones, their standard welders, and the four loading-wall doors."""
import unreal,time,json,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
base=Path(unreal.Paths.project_saved_dir());out=base/'Screenshots/Reviews';out.mkdir(parents=True,exist_ok=True)
q={'checks':{},'errors':[]};start=time.monotonic();stage=0;last=0;active_shot=False
def pc():return next(p for w in unreal.EditorLevelLibrary.get_pie_worlds(False) for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller())
def plant():return unreal.GameplayStatics.get_all_actors_of_class(pc().get_world(),unreal.FoundrylineFactoryPlant)[0]
def report():return json.loads(plant().get_factory_report())
def act(a):unreal.FoundrylineNetworkTestTools.queue_factory_action(pc(),a)
def camera(location=None,target=None):
 unreal.FoundrylineNetworkTestTools.setup_workshop_review(pc(),True);c=pc().get_view_target()
 location=location or unreal.Vector(2200,6500,7000);target=target or unreal.Vector(800,2600,100)
 c.set_actor_location(location,False,False);c.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(location,target),False)
def shot(name):unreal.AutomationLibrary.take_high_res_screenshot(1920,1200,str(out/('TwinZones_'+name+'.png')))
def tick(dt):
 global stage,last,active_shot
 try:
  t=time.monotonic()-start
  if stage==0 and t>3:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1;last=t
  elif stage==1 and t-last>14:camera();act('Toggle');act('Overlay');act('Page2');stage=2;last=t
  elif stage==2 and t-last>3:
   q['checks']['two_stocked_matching_bays']=plant().get_zone_count()==2 and report()['pallets']==29
   q['checks']['two_visible_welding_machines']=sum(c.get_instance_count() for c in plant().get_components_by_class(unreal.InstancedStaticMeshComponent) if c.get_editor_property('static_mesh') and c.get_editor_property('static_mesh').get_name()=='SM_Welder_Machine')==2
   q['checks']['two_gun_tip_effects']=len(plant().get_components_by_class(unreal.FoundrylineWeldingEffectComponent))==2
   shot('Stocked_Bays');stage=3;last=t
  elif stage==3 and t-last>3:act('Toggle');act('Overlay');camera(unreal.Vector(1600,1100,380),unreal.Vector(1205,1610,100));stage=4;last=t
  elif stage==4 and t-last>3:shot('Standard_MIG_Welder');stage=5;last=t
  elif stage==5 and t-last>3:camera(unreal.Vector(1550,700,700),unreal.Vector(1500,1600,110));act('Toggle');act('Overlay');act('Page0');act('Order');act('Speed');stage=6;last=t
  elif stage==6 and t-last>2:act('ZoneNext');act('Order');act('Speed');stage=7;last=t
  elif stage==7 and t-last>.15:
   if not active_shot and any(e.is_welding() and e.get_visible_spark_count()>0 for e in plant().get_components_by_class(unreal.FoundrylineWeldingEffectComponent)):
    shot('Active_Welding');active_shot=True
   if plant().get_completed_zone_count()==2:act('Run');camera();stage=8;last=t
   else:unreal.FoundrylineNetworkTestTools.advance_factory_test(pc(),200 if active_shot else 1);last=t
  elif stage==8 and t-last>3:
   q['checks']['both_products_complete']=plant().get_completed_zone_count()==2;q['checks']['live_gun_tip_sparks_observed']=active_shot;shot('Completed_Tankers');stage=9;last=t
  elif stage==9 and t-last>3:act('Toggle');act('Overlay');camera(unreal.Vector(9000,3000,4200),unreal.Vector(2800,2500,400));stage=10;last=t
  elif stage==10 and t-last>3:shot('Loading_Wall');stage=11;last=t
  elif stage==11 and t-last>3:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_end_play();stage=12;last=t
  elif stage==12 and t-last>3:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=13;last=t
  elif stage==13 and t-last>14:act('Toggle');act('Page2');act('Overlay');act('Save');stage=14;last=t
  elif stage==14 and t-last>3:
   q['checks']['fresh_twin_bay_layout_saved']=plant().get_zone_count()==2 and report()['pallets']==29 and not report()['running'] and not pc().get_factory_feedback();q['completed']=True;(base/'Logs/TwinWorkZonesVisualQA.json').write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);return
  if t>300:raise RuntimeError('Twin work-zone review timed out at '+str(stage))
  (base/'Logs/TwinWorkZonesVisualQA.json').write_text(json.dumps({**q,'stage':stage},indent=2))
 except Exception:
  q['errors'].append(traceback.format_exc());(base/'Logs/TwinWorkZonesVisualQA.json').write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
