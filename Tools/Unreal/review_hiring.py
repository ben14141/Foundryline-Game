"""Real ERP free-hire, names, crew binding, staffed production and roster overflow acceptance."""
import unreal,time,json,traceback,runpy,re,builtins
from pathlib import Path
T=unreal.FoundrylineNetworkTestTools
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
base=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()));out=base/'Screenshots/Reviews';out.mkdir(parents=True,exist_ok=True);log=base/'Logs/HiringNativeQA.json'
start=time.monotonic();last=0;stage=0;oldblur=0;employee=0;order=0;camera=None;phase_seen=set();hire_roles=[0,1,2];hire_index=0;before_count=0;hired={};known=set();original_employee=0;original_driver=0;original_vehicle=0;names={};bulk=[];before_bulk_cash=0;q={'checks':{},'errors':[],'screenshots':[]}
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
 path=out/('Hiring_'+n+'.png');unreal.SystemLibrary.execute_console_command(pc().get_world(),'Shot filename="'+str(path)+'"');q['screenshots'].append(str(path))
def finish():
 try:
  pause(False)
  unreal.SystemLibrary.execute_console_command(pc().get_world(),'r.MotionBlurQuality '+str(oldblur))
  if hasattr(builtins,'foundryline_stop_employee_visual_probe'):builtins.foundryline_stop_employee_visual_probe()
  if hasattr(builtins,'foundryline_stop_hire_visual_probe'):builtins.foundryline_stop_hire_visual_probe()
 except Exception:pass
 q['completed']=True
 q['screenshots']=[str(max([f for f in out.iterdir() if re.fullmatch(re.escape(Path(p).stem)+r'\d{5}\.png',f.name)],key=lambda f:f.stat().st_mtime)) for p in q['screenshots'] if [f for f in out.iterdir() if re.fullmatch(re.escape(Path(p).stem)+r'\d{5}\.png',f.name)]]
 log.write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.EditorPythonScripting.set_keep_python_script_alive(False)
def tick(dt):
 global stage,last,employee,order,camera,oldblur,hire_index,before_count,hired,known,original_employee,original_driver,original_vehicle,names,bulk,before_bulk_cash
 try:
  t=time.monotonic()-start
  if t>480:raise RuntimeError('Employee native timeout at stage '+str(stage))
  if stage==0 and t>4:
   if unreal.EditorLevelLibrary.get_pie_worlds(False):unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_end_play();stage=-1;last=t
   else:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1;last=t
  elif stage==-1 and t-last>4:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1;last=t
  elif stage==1 and t-last>15:
   unreal.SystemLibrary.execute_console_command(pc().get_world(),'LevelEditor.ToggleImmersive');oldblur=unreal.SystemLibrary.get_console_variable_int_value('r.MotionBlurQuality');unreal.SystemLibrary.execute_console_command(pc().get_world(),'r.MotionBlurQuality 0');p=people();check('starter_own_roster_two_people',p['count']==2 and p['operators']==1 and p['drivers']==1 and p['welders']==0);check('fresh_first_person_no_free_stock',ui()['walking'] and commerce()['cash']==400000 and not commerce()['pallets']);original_employee=next(e['id'] for e in p['employees'] if e['role']==0);original_driver=next(e['id'] for e in p['employees'] if e['role']==2);original_vehicle=p['vehicles'][0]['id'];known={e['id'] for e in p['employees']};names={e['id']:e['name'] for e in p['employees']};runpy.run_path(str(Path(__file__).with_name('observe_hired_employee_visuals.py')));check('starting_staff_have_full_names',all(len(e['name'].split())>=2 for e in p['employees']));T.setup_operations_review(pc(),1);factory('Interact');action('Page7');stage=2;last=t
  elif stage==2 and t-last>2:check('own_company_team_screen',ui()['screen']==1 and ui()['page']==7);shot('CompanyTeam');action('Workspace2');stage=201;last=t
  elif stage==201 and t-last>2:check('ERP_hiring_screen_available',ui()['workspace_tab']==2);shot('RecruitmentDesk');stage=202;last=t
  elif stage==202 and t-last>1:before_count=people()['count'];action('Hire'+str(hire_roles[hire_index]));stage=203;last=t
  elif stage==203 and t-last>.5:
   p=people()
   if p['count']==before_count+1:
    role=hire_roles[hire_index];e=next(e for e in p['employees'] if e['id'] not in known);hired[role]=dict(e);known.add(e['id']);names[e['id']]=e['name'];check('free_hire_role_'+str(role),p['cash']==400000 and p['forklifts']==1 and not e['assignment'] and not e['seated']);hire_index+=1
    if hire_index==3:
     employee=hired[0]['id'];builtins.foundryline_hire_visual_expected={'Operator':hired[0]['key'],'Welder':hired[1]['key'],'Driver':hired[2]['key']};builtins.foundryline_released_driver_key=next(e['key'] for e in p['employees'] if e['id']==original_driver);check('random_full_names_unique_and_saved_on_roster',all(len(e['name'].split())>=2 for e in p['employees']) and len({e['name'].casefold() for e in p['employees']})==p['count']);stage=204
    else:stage=202
    last=t
  elif stage==204 and t-last>2:shot('NamedRecruitment');action('Close');stage=205;last=t
  elif stage==205 and t-last>1:
   positions=[e['position'] for e in people()['employees'] if e['id'] in [hired[0]['id'],hired[1]['id'],hired[2]['id']]];centre=unreal.Vector(sum(p[0] for p in positions)/3,sum(p[1] for p in positions)/3,20)
   if camera is None:camera=T.create_review_camera(pc())
   eye=unreal.Vector(centre.x,70,210);target=centre+unreal.Vector(0,0,95);camera.set_actor_location(eye,False,False);camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(eye,target),False);camera.get_components_by_class(unreal.CameraComponent)[0].set_field_of_view(90);pc().set_view_target_with_blend(camera,0);stage=206;last=t
  elif stage==206 and t-last>2:shot('NewHiresWaiting');stage=207;last=t
  elif stage==207 and t-last>1:
   pc().set_view_target_with_blend(pc().get_controlled_pawn(),0);T.setup_operations_review(pc(),1);factory('Interact');action('Page7');action('AssignEmployee'+str(original_driver)+'_2_0');stage=208;last=t
  elif stage==208 and t-last>2:
   p=people();check('release_driver_keeps_owned_forklift',p['forklifts']==1 and next(e for e in p['employees'] if e['id']==original_driver)['vehicle']==0 and p['vehicles'][0]['employee']==0);action('AssignEmployee'+str(hired[2]['id'])+'_2_'+str(original_vehicle));stage=209;last=t
  elif stage==209 and t-last>2:
   p=people();e=next(e for e in p['employees'] if e['id']==hired[2]['id']);check('hired_driver_binds_existing_vehicle_without_duplicate_person',e['vehicle']==original_vehicle and e['name']==hired[2]['name'] and e['seated'] and p['forklifts']==1 and p['count']==5);shot('NamedCrewAssigned');action('AssignEmployee'+str(original_employee)+'_0_0');stage=4;last=t
  elif stage==4 and t-last>2:
   check('new_operator_waits_unassigned',operator()['assignment']==0 and next(e for e in people()['employees'] if e['id']==original_employee)['assignment']==0);action('Close');driver=next(e for e in people()['employees'] if e['role']==2);aim(driver['position'],True);stage=5;last=t
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
  elif stage==22 and t-last>2:shot('EmployeeIdle');q['final_production']=people();check('names_preserved_through_binding_and_production',all(e['name']==names[e['id']] for e in people()['employees']));T.setup_operations_review(pc(),1);stage=230;last=t
  elif stage==230 and t-last>2:
   factory('Interact');action('Page7');action('Workspace2');p=people();bulk=[role for role in range(3) for _ in range(4-sum(e['role']==role for e in p['employees']))];before_bulk_cash=p['cash'];stage=231;last=t
  elif stage==231 and t-last>.5:
   if bulk:before_count=people()['count'];action('Hire'+str(bulk.pop(0)));stage=232;last=t
   else:stage=233;last=t
  elif stage==232 and t-last>.5:
   if people()['count']==before_count+1:stage=231;last=t
  elif stage==233 and t-last>2:
   p=people();check('twelve_named_staff_no_extra_forklifts_or_charge',p['count']==12 and p['forklifts']==1 and p['cash']==before_bulk_cash and all(sum(e['role']==r for e in p['employees'])==4 for r in range(3)) and len({e['name'].casefold() for e in p['employees']})==12);shot('RecruitmentAtTestingLimit');action('Workspace0');stage=234;last=t
  elif stage==234 and t-last>2:shot('NamedRosterPage1');action('Next');stage=235;last=t
  elif stage==235 and t-last>2:check('roster_paginates_without_overlap',ui()['list_page']==1);shot('NamedRosterPage2');q['hired_roster']=people();stage=236;last=t
  elif stage==236 and t-last>1:
   if hasattr(builtins,'foundryline_stop_employee_visual_probe'):builtins.foundryline_stop_employee_visual_probe()
   if hasattr(builtins,'foundryline_stop_hire_visual_probe'):builtins.foundryline_stop_hire_visual_probe()
   unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_end_play();camera=None;stage=24;last=t
  elif stage==24 and t-last>4:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=25;last=t
  elif stage==25 and t-last>15:check('left_fresh_first_person',ui()['walking'] and ui()['screen']==0 and not people()['running'] and commerce()['cash']==400000 and not commerce()['pallets']);q['fresh_employee_report']=people();shot('ReadyToPlay');stage=26;last=t
  elif stage==26 and t-last>1:finish();return
  q['stage']=stage;log.write_text(json.dumps(q,indent=2))
 except Exception:q['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
