"""Real starter employee workflow and floor-level visual acceptance; no inventory grants."""
import unreal,time,json,traceback,runpy,re,builtins
from pathlib import Path
T=unreal.FoundrylineNetworkTestTools
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
base=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()));out=base/'Screenshots/Reviews';out.mkdir(parents=True,exist_ok=True);log=base/'Logs/EmployeeNativeQA.json'
start=time.monotonic();last=0;stage=0;oldblur=0;employee=0;order=0;camera=None;phase_seen=set();q={'checks':{},'errors':[],'screenshots':[]}
def pc():return next(p for w in unreal.EditorLevelLibrary.get_pie_worlds(False) for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller())
def people():return json.loads(T.employee_report(pc()))
def commerce():return json.loads(T.commerce_report(pc()))
def ui():return json.loads(T.operations_ui_report(pc()))
def operator():return next(e for e in people()['employees'] if e['id']==employee and e['role']==0)
def action(s):T.queue_commerce_action(pc(),s)
def factory(s):T.queue_factory_action(pc(),s)
def check(n,v):q['checks'][n]=bool(v)
def pause(v):unreal.GameplayStatics.set_game_paused(pc().get_world(),v)
def aim(position,seat=False,work=False,side=False):
 global camera
 if camera is None:camera=T.create_review_camera(pc())
 at=unreal.Vector(*position);eye=at+(unreal.Vector(-145,360,170) if side else unreal.Vector(-430,320,220) if seat else unreal.Vector(150,-400,170) if work else unreal.Vector(350,350,170));target=at+(unreal.Vector(-145,0,150) if seat else unreal.Vector(0,0,95));camera.set_actor_location(eye,False,False);camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(eye,target),False);camera.get_components_by_class(unreal.CameraComponent)[0].set_field_of_view(60);pc().set_view_target_with_blend(camera,0)
def shot(n):
 path=out/('Employees_'+n+'.png');unreal.SystemLibrary.execute_console_command(pc().get_world(),'Shot filename="'+str(path)+'"');q['screenshots'].append(str(path))
def finish():
 try:
  pause(False)
  unreal.SystemLibrary.execute_console_command(pc().get_world(),'r.MotionBlurQuality '+str(oldblur))
  if hasattr(builtins,'foundryline_stop_employee_visual_probe'):builtins.foundryline_stop_employee_visual_probe()
 except Exception:pass
 q['completed']=True
 q['screenshots']=[str(max([f for f in out.iterdir() if re.fullmatch(re.escape(Path(p).stem)+r'\d{5}\.png',f.name)],key=lambda f:f.stat().st_mtime)) for p in q['screenshots'] if [f for f in out.iterdir() if re.fullmatch(re.escape(Path(p).stem)+r'\d{5}\.png',f.name)]]
 log.write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.EditorPythonScripting.set_keep_python_script_alive(False)
def tick(dt):
 global stage,last,employee,order,camera,oldblur
 try:
  t=time.monotonic()-start
  if t>360:raise RuntimeError('Employee native timeout at stage '+str(stage))
  if stage==0 and t>4:
   if unreal.EditorLevelLibrary.get_pie_worlds(False):unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_end_play();stage=-1;last=t
   else:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1;last=t
  elif stage==-1 and t-last>4:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1;last=t
  elif stage==1 and t-last>15:
   unreal.SystemLibrary.execute_console_command(pc().get_world(),'LevelEditor.ToggleImmersive');oldblur=unreal.SystemLibrary.get_console_variable_int_value('r.MotionBlurQuality');unreal.SystemLibrary.execute_console_command(pc().get_world(),'r.MotionBlurQuality 0');runpy.run_path(str(Path(__file__).with_name('observe_employee_visuals.py')));p=people();check('starter_own_roster_two_people',p['count']==2 and p['operators']==1 and p['drivers']==1 and p['welders']==0);check('fresh_first_person_no_free_stock',ui()['walking'] and commerce()['cash']==400000 and not commerce()['pallets']);employee=next(e['id'] for e in p['employees'] if e['role']==0);T.setup_operations_review(pc(),1);factory('Interact');action('Page7');stage=2;last=t
  elif stage==2 and t-last>2:check('own_company_team_screen',ui()['screen']==1 and ui()['page']==7);shot('CompanyTeam');stage=3;last=t
  elif stage==3 and t-last>1:action('AssignEmployee'+str(employee)+'_0_0');stage=4;last=t
  elif stage==4 and t-last>2:
   check('ERP_off_duty_reaches_server',operator()['assignment']==0);action('Close');driver=next(e for e in people()['employees'] if e['role']==2);aim(driver['position'],True);stage=5;last=t
  elif stage==5 and t-last>2:shot('ForkliftDriver');stage=51;last=t
  elif stage==51 and t-last>1:aim(next(e for e in people()['employees'] if e['role']==2)['position'],True,side=True);stage=52;last=t
  elif stage==52 and t-last>2:shot('ForkliftDriverSide');stage=6;last=t
  elif stage==6 and t-last>1:
   pc().set_view_target_with_blend(pc().get_controlled_pawn(),0);factory('Interact');action('Page0');order=commerce()['orders'][0]['id'];action('Order'+str(order));action('Accept');stage=7;last=t
  elif stage==7 and t-last>2:action('Assign');action('Page3');action('SheetsMinus');stage=71;last=t
  elif stage==71 and t-last>2:action('BuySteel');stage=8;last=t
  elif stage==8 and t-last>2:check('real_steel_purchase',len(commerce()['bookings'])==1 and commerce()['cash']<400000);action('Plasma');stage=9;last=t
  elif stage==9 and t-last>1:factory('Apply');stage=10;last=t
  elif stage==10 and t-last>.2:
   T.advance_commerce_test(pc(),200);c=commerce();p=people()
   if any(x['part']==0 and x['place'] in [1,2] for x in c['pallets']):check('unassigned_operator_blocks_cutting',all(x['part']==0 for x in c['pallets']) and p['machines'][0]['batch']==10 and p['machines'][0]['remaining']==0);action('Office');action('Page7');stage=11;last=t
   else:last=t
  elif stage==11 and t-last>2:shot('WaitingForOperator');stage=12;last=t
  elif stage==12 and t-last>1:action('AssignEmployee'+str(employee)+'_0_1');action('Close');stage=13;last=t
  elif stage==13 and t-last>.1:
   e=operator()
   if e['moving'] and e['distance']>35:check('assigned_employee_walks_real_path',len(e['path'])>1 and e['assignment']==1);aim(e['position']);stage=14;last=t
   else:T.advance_commerce_test(pc(),1);last=t
  elif stage==14 and t-last>.75:shot('WalkingToPlasma');stage=15;last=t
  elif stage==15 and t-last>1:pause(False);factory('Run');stage=16;last=t
  elif stage==16 and t-last>2:check('pause_stops_employee',not operator()['moving'] and not operator()['working']);shot('PausedEmployee');stage=17;last=t
  elif stage==17 and t-last>1:factory('Run');stage=18;last=t
  elif stage==18 and t-last>.2:
   e=operator()
   if e['working'] and not e['moving']:check('operator_arrives_before_cutting',people()['machines'][0]['staff_ready'] and not e['moving']);aim(e['position'],work=True);stage=19;last=t
   else:T.advance_commerce_test(pc(),10);last=t
  elif stage==19 and t-last>2:shot('PlasmaOperator');stage=20;last=t
  elif stage==20 and t-last>.2:
   e=operator()
   if e['working']:phase_seen.add(round(e['phase'],2))
   T.advance_commerce_test(pc(),10)
   if sum(x['quantity'] for x in commerce()['pallets'] if x['part']==1)==10 and people()['machines'][0]['batch']==0 and people()['machines'][0]['remaining']==0:check('real_staffed_batch_makes_ten_parts',True);check('work_phase_advances',len(phase_seen)>1);stage=21;last=t
   else:last=t
  elif stage==21 and t-last>.2:
   e=operator();distance=sum((e['position'][i]-e['home'][i])**2 for i in range(2))**.5
   if distance<3 and not e['moving']:check('operator_returns_home_when_batch_complete',not e['working']);factory('Run');aim(e['position']);stage=22;last=t
   else:T.advance_commerce_test(pc(),20);last=t
  elif stage==22 and t-last>2:shot('EmployeeIdle');q['final_production']=people();stage=23;last=t
  elif stage==23 and t-last>1:
   if hasattr(builtins,'foundryline_stop_employee_visual_probe'):builtins.foundryline_stop_employee_visual_probe()
   unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_end_play();camera=None;stage=24;last=t
  elif stage==24 and t-last>4:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=25;last=t
  elif stage==25 and t-last>15:check('left_fresh_first_person',ui()['walking'] and ui()['screen']==0 and not people()['running'] and commerce()['cash']==400000 and not commerce()['pallets']);q['fresh_employee_report']=people();shot('ReadyToPlay');stage=26;last=t
  elif stage==26 and t-last>1:finish();return
  q['stage']=stage;log.write_text(json.dumps(q,indent=2))
 except Exception:q['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
