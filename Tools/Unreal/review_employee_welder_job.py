"""Transient larger workshop fixture: a generic hire claims an actual zone-mounted welder."""
import unreal,time,json,traceback,runpy,builtins,re
from pathlib import Path
T=unreal.FoundrylineNetworkTestTools;unreal.EditorPythonScripting.set_keep_python_script_alive(True)
base=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()));out=base/'Screenshots/Reviews';log=base/'Logs/EmployeeWelderJobNativeQA.json';start=time.monotonic();last=0;stage=0;employee=0;zone=0;oldwelder=0;known=set();name='';camera=None;initial_count=0;q={'fixture':'standard larger stocked workshop; transient PIE only','checks':{},'errors':[],'screenshots':[]}
def pc():return next(p for w in unreal.EditorLevelLibrary.get_pie_worlds(False) for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller())
def report():return json.loads(T.employee_report(pc()))
def person():return next(e for e in report()['employees'] if e['id']==employee)
def ui():return json.loads(T.operations_ui_report(pc()))
def action(s):T.queue_commerce_action(pc(),s)
def factory(s):T.queue_factory_action(pc(),s)
def check(k,v):q['checks'][k]=bool(v)
def aim(pos):
 global camera
 if camera is None:camera=T.create_review_camera(pc())
 at=unreal.Vector(*pos);eye=at+unreal.Vector(290,-300,190);target=at+unreal.Vector(35,0,110);camera.set_actor_location(eye,False,False);camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(eye,target),False);camera.get_components_by_class(unreal.CameraComponent)[0].set_field_of_view(57);pc().set_view_target_with_blend(camera,0)
def shot(n):
 p=out/('EmployeeJobs_'+n+'.png');unreal.SystemLibrary.execute_console_command(pc().get_world(),'Shot filename="'+str(p)+'"');q['screenshots'].append(str(p))
def finish():
 if hasattr(builtins,'foundryline_stop_job_visual_probe'):builtins.foundryline_stop_job_visual_probe()
 q['completed']=True;q['screenshots']=[str(max(fs,key=lambda x:x.stat().st_mtime)) for p in q['screenshots'] if (fs:=[f for f in out.iterdir() if re.fullmatch(re.escape(Path(p).stem)+r'\d{5}\.png',f.name)])];log.write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.EditorPythonScripting.set_keep_python_script_alive(False)
def tick(dt):
 global stage,last,employee,zone,oldwelder,known,name,initial_count
 try:
  t=time.monotonic()-start
  if t>280:raise RuntimeError('Welder native timeout '+str(stage))
  if stage==0 and t>4:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1;last=t
  elif stage==1 and t-last>16:
   unreal.SystemLibrary.execute_console_command(pc().get_world(),'LevelEditor.ToggleImmersive');unreal.SystemLibrary.execute_console_command(pc().get_world(),'r.MotionBlurQuality 0');check('real_larger_workshop_zones',len(report()['zones'])==2);check('own_office_fixture',T.setup_employee_job_office(pc()));T.setup_operations_review(pc(),1);zone=report()['zones'][0]['id'];oldwelder=next(e['id'] for e in report()['employees'] if e['role']==1 and e['assignment']==zone);known={e['id'] for e in report()['employees']};initial_count=len(known);builtins.foundryline_job_visual_mode='welder';runpy.run_path(str(Path(__file__).with_name('observe_employee_job_visuals.py')));action('Office');action('Page7');action('Workspace2');stage=2;last=t
  elif stage==2 and t-last>2:check('own_ERP_hiring_open',ui()['screen']==1 and ui()['page']==7);action('HirePerson');stage=3;last=t
  elif stage==3 and t-last>2:
   e=next(e for e in report()['employees'] if e['id'] not in known);employee=e['id'];name=e['name'];check('generic_hire_before_welding_job',e['role']==3 and not e['assignment']);action('SelectEmployee'+str(employee));action('JobRole1');stage=4;last=t
  elif stage==4 and t-last>2:shot('ChooseWeldingStation');stage=41;last=t
  elif stage==41 and t-last>1:action('AssignEmployee'+str(oldwelder)+'_3_0');stage=5;last=t
  elif stage==5 and t-last>2:action('AssignEmployee'+str(employee)+'_1_'+str(zone));stage=6;last=t
  elif stage==6 and t-last>2:
   e=person();check('specific_zone_welder_claim_same_person',e['role']==1 and e['assignment']==zone and e['name']==name and report()['count']==initial_count+1);shot('NamedWelderAssigned');stage=61;last=t
  elif stage==61 and t-last>1:T.queue_factory_order_zone(pc(),zone);action('Close');stage=7;last=t
  elif stage==7 and t-last>.15:
   e=person()
   if e['moving'] and e['distance']>35:check('welder_walks_to_assigned_zone',len(e['path'])>1);aim(e['position']);stage=8;last=t
   else:T.advance_commerce_test(pc(),2);last=t
  elif stage==8 and t-last>1:shot('WelderWalkingToJob');stage=9;last=t
  elif stage==9 and t-last>.2:
   e=person()
   if e['working'] and not e['moving']:check('welder_operates_actual_product_job',e['job']!='No job assigned' and next(z for z in report()['zones'] if z['id']==zone)['active_step']>=0);aim(e['position']);stage=10;last=t
   else:T.advance_commerce_test(pc(),5);last=t
  elif stage==10 and t-last>3:shot('WelderAtProduct');stage=11;last=t
  elif stage==11 and t-last>3:
   check('assigned_welder_progresses_real_zone',any(z['id']==zone and (z['work']>0 or z['completed']>0) for z in report()['zones']));factory('Run');stage=12;last=t
  elif stage==12 and t-last>2:check('paused_welding_job_stops',not person()['working']);shot('WelderJobPaused');stage=13;last=t
  elif stage==13 and t-last>2:finish();return
  q['stage']=stage;log.write_text(json.dumps(q,indent=2))
 except Exception:q['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
