"""Read-only native employee interaction and shared machine-screen review."""
import unreal,time,json,traceback
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve();out=root/'Saved/Screenshots/Reviews';out.mkdir(parents=True,exist_ok=True)
N=unreal.FoundrylineNetworkTestTools;L=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
L.load_level('/Game/Foundryline/Maps/L_FirstShopStarter');unreal.EditorPythonScripting.set_keep_python_script_alive(True)
start=time.monotonic();last=start;stage=0;target_id=0;target_seated=False;machine=2;q={'checks':{},'errors':[]}
def worlds():return list(unreal.EditorLevelLibrary.get_pie_worlds(False))
def host():return next(p for w in worlds() if unreal.GameplayStatics.get_game_mode(w) for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller())
def ui():return json.loads(N.operations_ui_report(host()))
def factory(s):N.queue_factory_action(host(),unreal.Name(s))
def shot(n):unreal.SystemLibrary.execute_console_command(host(),'Shot filename="'+str(out/(n+'.png'))+'"')
def check(k,v):
 q['checks'][k]=bool(v)
 if not v:q['errors'].append(k)
def place_employee(seated):
 global target_id,target_seated
 p=host();staff=json.loads(N.employee_report(p))['employees'];e=next(e for e in staff if e['seated']==seated);target_id=e['id'];target_seated=seated
 plant=unreal.GameplayStatics.get_all_actors_of_class(p,unreal.FoundrylineFactoryPlant)[0]
 v=next(v for v in plant.get_components_by_class(unreal.FoundrylineEmployeeVisualComponent) if unreal.Name(e['key']) in v.get_editor_property('component_tags'))
 target=v.get_world_location()+unreal.Vector(0,0,205 if seated else 140)
 at=target+unreal.Vector(160,0,-(95 if seated else 45))
 N.place_player_collision_review(p,at,180)
 p.set_control_rotation(unreal.MathLibrary.find_look_at_rotation(at+unreal.Vector(0,0,70),target))
 q['employee_'+('driver' if seated else 'operator')]=e

def finish():
 q['completed']=True;(root/'Saved/Logs/EmployeeMachineNativeQA.json').write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(host() if worlds() else None,'QUIT_EDITOR')
def tick(dt):
 global stage,last,machine
 try:
  now=time.monotonic()
  if now-start>220:raise RuntimeError('Native interaction timeout at '+str(stage))
  if stage==0 and now-last>3:L.editor_request_begin_play();stage=1;last=now
  elif stage==1 and now-last>18:N.setup_operations_review(host(),1);stage=2;last=now
  elif stage==2 and now-last>2:place_employee(False);stage=3;last=now
  elif stage==3 and now-last>2:
   q['operator_aim']=ui();check('standing_employee_focus',ui()['focused_employee']==target_id);factory('Interact');stage=4;last=now
  elif stage==4 and now-last>2:
   q['operator_ui']=ui();check('standing_employee_e_opens_jobs',ui()['screen']==4 and ui()['inspected_employee']==target_id);shot('Playtest_EmployeeJobs');stage=5;last=now
  elif stage==5 and now-last>1:N.queue_commerce_action(host(),'Close');stage=6;last=now
  elif stage==6 and now-last>1:place_employee(True);stage=7;last=now
  elif stage==7 and now-last>2:
   q['driver_aim']=ui();check('seated_driver_focus',ui()['focused_employee']==target_id);factory('Interact');stage=8;last=now
  elif stage==8 and now-last>2:
   q['driver_ui']=ui();check('seated_driver_e_opens_jobs',ui()['screen']==4 and ui()['inspected_employee']==target_id);shot('Playtest_DriverJobs');stage=9;last=now
  elif stage==9 and now-last>1:L.editor_request_end_play();stage=10;last=now
  elif stage==10 and now-last>4:
   if worlds():return
   L.load_level('/Game/Foundryline/Maps/L_FlatForest_Grass');stage=11;last=now
  elif stage==11 and now-last>3:L.editor_request_begin_play();stage=12;last=now
  elif stage==12 and now-last>18:factory('WalkView');stage=13;last=now
  elif stage==13 and now-last>2:N.queue_factory_console(host(),machine);stage=14;last=now
  elif stage==14 and now-last>2:factory('Interact');stage=15;last=now
  elif stage==15 and now-last>2:
   check('machine_%d_physical_screen'%machine,ui()['machine']==machine);shot('Playtest_'+('PressBrakeControl' if machine==2 else 'RollerControl'));stage=16;last=now
  elif stage==16 and now-last>1:
   if machine==2:factory('CloseComputer');machine=3;stage=13;last=now
   else:finish()
 except Exception:q['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
