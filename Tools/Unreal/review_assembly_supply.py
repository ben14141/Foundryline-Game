"""Rendered collector/bin/catalog review using real PIE controller commands. No saves."""
import unreal,time,json,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
T=unreal.FoundrylineNetworkTestTools
root=Path(unreal.Paths.project_saved_dir()).resolve();shots=root/'Screenshots/Reviews';shots.mkdir(parents=True,exist_ok=True)
start=time.monotonic();stage=0;last=0;camera=None;station=0;collector=0
report={'checks':{},'shots':[],'errors':[]};busy=False
def players():return [a for world in unreal.EditorLevelLibrary.get_pie_worlds(False) for a in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.PlayerController) if a.is_local_controller()]
def player():return players()[0]
def staff():return json.loads(T.employee_report(player()))
def guid(text):
 value=unreal.Guid()
 for field,i in zip('abcd',range(0,32,8)):value.set_editor_property(field,int(text.replace('-','')[i:i+8],16))
 return value
def look(location,target):
 global camera
 if not camera:camera=T.create_review_camera(player())
 camera.set_actor_location_and_rotation(location,unreal.MathLibrary.find_look_at_rotation(location,target),False,False);camera.camera_component.set_field_of_view(65);player().set_view_target_with_blend(camera,0)
def shot(name):
 global busy
 busy=True
 try:
  path=str(shots/(name+'.png'));unreal.AutomationLibrary.take_high_res_screenshot(1600,1000,path);report['shots'].append(path)
 finally:busy=False
def finish():
 report['completed']=True;(root/'Logs/AssemblySupplyVisualQA.json').write_text(json.dumps(report,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(unreal.EditorLevelLibrary.get_pie_worlds(False)[0],'QUIT_EDITOR')
def tick(dt):
 global stage,last,station,collector
 if busy:return
 try:
  now=time.monotonic()-start
  if now>160:raise RuntimeError('Review timeout at stage '+str(stage))
  if stage==0 and now>3:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1;last=now
  elif stage==1 and now-last>12 and players():
   p=player();T.queue_property_view(p);T.queue_factory_lane(p,0,unreal.Vector(-2217.6,1275,20),unreal.Vector(-2217.6,2170,20),304.8);T.queue_factory_lane(p,1,unreal.Vector(-1455,1214.04,20),unreal.Vector(-810,1214.04,20),121.92);T.queue_factory_lane(p,2,unreal.Vector(-1370,1450,20),unreal.Vector(-1370,1450,20),200);stage=2;last=now
  elif stage==2 and now-last>3:
   preview=json.loads(T.preview_welding_station(player(),unreal.Vector(-1768.02,1823.12,20),0,False,True));report['checks']['hologram_valid']=preview['valid'];T.queue_factory_action(player(),'ToolDone');stage=3;last=now
  elif stage==3 and now-last>3:
   placed=json.loads(T.welding_station_report(player()))['stations'];assert placed,'Station purchase failed';station=placed[0]['id'];T.setup_welding_supply_review(player(),station);T.queue_factory_action(player(),'StationSupply');T.queue_employee_hire(player(),3,guid('96703d7179e446fe9534199886830d01'),guid(staff()['plant']));stage=4;last=now
  elif stage==4 and now-last>2:
   person=next(p for p in staff()['employees'] if p['role']==3);collector=person['id'];T.queue_employee_assignment(player(),collector,4,station,guid(staff()['plant']));T.queue_factory_action(player(),'Toggle');unreal.SystemLibrary.execute_console_command(player(),'showhud');look(unreal.Vector(-1680,1530,270),unreal.Vector(-2020,1920,80));stage=5;last=now
  elif stage==5 and now-last>3:
   shot('AssemblySupply_InGame_Bin');stage=6;last=now
  elif stage==6:
   person=next((p for p in staff()['employees'] if p['id']==collector),None)
   if person and person['speed']>20:
    pos=unreal.Vector(*person['position']);look(pos+unreal.Vector(220,340,220),pos+unreal.Vector(55,0,80))
    if now-last>5:shot('AssemblySupply_InGame_Collector');report['checks']['collector_walks_with_cart']=True;stage=7;last=now
   elif now-last>35:raise RuntimeError('Collector did not move: '+json.dumps(staff()))
  elif stage==7 and now-last>3:
   unreal.SystemLibrary.execute_console_command(player(),'showhud');T.queue_factory_action(player(),'Toggle');T.queue_factory_action(player(),'Page3');look(unreal.Vector(-1400,1600,330),unreal.Vector(-1800,1850,80));stage=8;last=now
  elif stage==8 and now-last>3:shot('AssemblySupply_PlanningCatalog');report['staff']=staff();stage=9;last=now
  elif stage==9 and now-last>2:finish();return
  report['stage']=stage;(root/'Logs/AssemblySupplyVisualQA.json').write_text(json.dumps(report,indent=2))
 except Exception:report['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
