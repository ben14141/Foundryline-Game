"""Capture actual runtime menus and product progression, then leave the shop open."""
import unreal,time,json,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
base=Path(unreal.Paths.project_saved_dir());out=base/'Screenshots/Reviews';out.mkdir(parents=True,exist_ok=True)
report={'checks':{},'errors':[]};start=time.monotonic();stage=0;last=0
def players():return [p for w in unreal.EditorLevelLibrary.get_pie_worlds(False) for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller()]
def pc():return players()[0]
def plant():return unreal.GameplayStatics.get_all_actors_of_class(pc().get_world(),unreal.FoundrylineFactoryPlant)[0]
def act(a):return unreal.FoundrylineNetworkTestTools.queue_factory_action(pc(),a)
def shot(name):unreal.AutomationLibrary.take_high_res_screenshot(1920,1200,str(out/('Workshop_'+name+'.png')))
def tick(dt):
 global stage,last
 try:
  t=time.monotonic()-start
  if stage==0 and t>3:stage=1;last=t;unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
  elif stage==1 and t-last>14:
   unreal.FoundrylineNetworkTestTools.setup_workshop_review(pc(),True);act('Computer1');stage=2;last=t
  elif stage==2 and t-last>5:report['checks']['plasma_computer_opens']=pc().get_open_factory_computer()==1;shot('Plasma_Computer');stage=3;last=t
  elif stage==3 and t-last>3:act('CloseComputer');act('Computer2');stage=4;last=t
  elif stage==4 and t-last>3:report['checks']['brake_computer_opens']=pc().get_open_factory_computer()==2;shot('Brake_Computer');stage=5;last=t
  elif stage==5 and t-last>3:act('CloseComputer');act('Requirements');stage=6;last=t
  elif stage==6 and t-last>3:report['checks']['requirements_opens']=pc().get_open_factory_computer()==4;shot('Requirements');stage=7;last=t
  elif stage==7 and t-last>3:
   act('CloseComputer');act('Page1');stage=61;last=t
  elif stage==61 and t-last>3:
   meshes=[c.get_editor_property('static_mesh') for c in plant().get_components_by_class(unreal.StaticMeshComponent)]
   report['checks']['seven_real_rack_frames']=sum(m is not None and m.get_name()=='SM_Rack_Upright' for m in meshes)==14
   report['checks']['twenty_one_real_shelves']=sum(m is not None and m.get_name()=='SM_Rack_Shelf400' for m in meshes)==21
   shot('Storage_Page1');act('StorageNext');stage=62;last=t
  elif stage==62 and t-last>3:shot('Storage_Page2');act('StorageNext');stage=63;last=t
  elif stage==63 and t-last>3:shot('Storage_Page3');act('Page0');act('Order');act('Speed');stage=8;last=t
  elif stage==8 and t-last>2:act('Speed');act('Overlay');stage=9;last=t
  elif stage==9 and t-last>.3:
   r=json.loads(plant().get_factory_report())
   if r['installed']>=24:act('Run');stage=10;last=t
   else:unreal.FoundrylineNetworkTestTools.advance_factory_test(pc(),80);last=t
  elif stage==10 and t-last>3:shot('Assembly_Progress');stage=11;last=t
  elif stage==11 and t-last>3:act('Run');stage=12;last=t
  elif stage==12 and t-last>.3:
   r=json.loads(plant().get_factory_report())
   if r['mask']==63:act('Run');stage=13;last=t
   else:unreal.FoundrylineNetworkTestTools.advance_factory_test(pc(),100);last=t
  elif stage==13 and t-last>3:shot('Finished_Tanker');report['checks']['finished_tanker']=json.loads(plant().get_factory_report())['mask']==63;stage=14;last=t
  elif stage==14 and t-last>3:act('Toggle');act('WalkView');stage=15;last=t
  elif stage==15 and t-last>3:
   unreal.FoundrylineNetworkTestTools.queue_factory_console(pc(),1);stage=16;last=t
  elif stage==16 and t-last>3:
   # The same E key path used by a player, queued outside editor Python execution.
   report['walk_pose']=unreal.FoundrylineNetworkTestTools.inspect_player(pc());act('Interact');stage=17;last=t
  elif stage==17 and t-last>3:
   report['checks']['walk_up_E_opens_computer']=pc().get_open_factory_computer()==1;shot('Walk_Computer');act('CloseComputer');act('BuildView');stage=18;last=t
  elif stage==18 and t-last>3:
   unreal.FoundrylineNetworkTestTools.setup_workshop_review(pc(),True);act('Toggle');stage=19;last=t
  elif stage==19 and t-last>3:
   act('Page2');stage=20;last=t
  elif stage==20 and t-last>3:
   shot('Color_Coded_Zones');act('Page0');act('PartDetail6');stage=21;last=t
  elif stage==21 and t-last>3:
   shot('Part_Sized_Bay');unreal.FoundrylineNetworkTestTools.setup_workshop_review(pc(),False);stage=211;last=t
  elif stage==211 and t-last>3:
   shot('Property_Overview');stage=212;last=t
  elif stage==212 and t-last>3:
   unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_end_play();stage=22;last=t
  elif stage==22 and t-last>3:
   unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=23;last=t
  elif stage==23 and t-last>14:
   act('Toggle');act('Page2');act('Save');stage=24;last=t
  elif stage==24 and t-last>3:
   report['checks']['starter_reset_ready']=json.loads(plant().get_factory_report())['mask']==0 and not json.loads(plant().get_factory_report())['running']
   report['checks']['starter_save_no_error']=not pc().get_factory_feedback()
   report['completed']=True;(base/'Logs/WorkshopVisualQA.json').write_text(json.dumps(report,indent=2));unreal.unregister_slate_post_tick_callback(handle);return
  if t>300:raise RuntimeError('Runtime review timed out at '+str(stage))
  (base/'Logs/WorkshopVisualQA.json').write_text(json.dumps({**report,'stage':stage},indent=2))
 except Exception:
  report['errors'].append(traceback.format_exc());(base/'Logs/WorkshopVisualQA.json').write_text(json.dumps(report,indent=2));unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
