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
def shot(name):unreal.AutomationLibrary.take_high_res_screenshot(1920,1200,str(out/('Factory_'+name+'.png')))
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
  elif stage==7 and t-last>3:act('CloseComputer');act('Order');act('Speed');stage=8;last=t
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
   shot('Part_Sized_Bay')
   report['completed']=True;(base/'Logs/FactoryVisualQA.json').write_text(json.dumps(report,indent=2));unreal.unregister_slate_post_tick_callback(handle);return
  if t>240:raise RuntimeError('Runtime review timed out at '+str(stage))
  (base/'Logs/FactoryVisualQA.json').write_text(json.dumps({**report,'stage':stage},indent=2))
 except Exception:
  report['errors'].append(traceback.format_exc());(base/'Logs/FactoryVisualQA.json').write_text(json.dumps(report,indent=2));unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
