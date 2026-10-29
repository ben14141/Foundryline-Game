import unreal,time,json,re
from pathlib import Path
T=unreal.FoundrylineNetworkTestTools;p=next(p for w in unreal.EditorLevelLibrary.get_pie_worlds(False) for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller());base=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()));out=base/'Screenshots/Reviews';start=time.monotonic();last=0;stage=0;q={'fixture':'transient larger-workshop review, camera exposure +5 EV only','checks':{},'screenshots':[]};camera=None

def report():return json.loads(T.employee_report(p))
def ui():return json.loads(T.operations_ui_report(p))
def action(s):T.queue_commerce_action(p,s)
def shot(n):
 path=out/('EmployeeJobs_'+n+'.png');unreal.SystemLibrary.execute_console_command(p.get_world(),'Shot filename="'+str(path)+'"');q['screenshots'].append(str(path))
def finish():
 q['completed']=True;q['screenshots']=[str(max(fs,key=lambda x:x.stat().st_mtime)) for x in q['screenshots'] if (fs:=[f for f in out.iterdir() if re.fullmatch(re.escape(Path(x).stem)+r'\d{5}\.png',f.name)])];(base/'Logs/EmployeeWelderVisualReview.json').write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(h)
def tick(dt):
 global stage,last,camera
 t=time.monotonic()-start
 if stage==0:
  if not report()['running']:T.queue_factory_action(p,'Run')
  stage=1;last=t
 elif stage==1 and t-last>1:
  e=max((e for e in report()['employees'] if e['role']==1),key=lambda e:e['id']);q['employee']=e;camera=T.create_review_camera(p);at=unreal.Vector(*e['position']);eye=at+unreal.Vector(230,-280,185);target=at+unreal.Vector(35,0,117);camera.set_actor_location(eye,False,False);camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(eye,target),False);c=camera.get_components_by_class(unreal.CameraComponent)[0];c.set_field_of_view(50);pp=c.get_editor_property('post_process_settings');pp.set_editor_property('override_auto_exposure_bias',True);pp.set_editor_property('auto_exposure_bias',5.0);pp.set_editor_property('override_bloom_intensity',True);pp.set_editor_property('bloom_intensity',.15);c.set_editor_property('post_process_settings',pp);c.set_editor_property('post_process_blend_weight',1.0);p.set_view_target_with_blend(camera,0);stage=2;last=t
 elif stage==2 and t-last>3:q['checks']['actual_welder_working']=next(e for e in report()['employees'] if e['id']==q['employee']['id'])['working'];shot('WelderAtProductClear');stage=3;last=t
 elif stage==3 and t-last>2:T.setup_operations_review(p,1);stage=4;last=t
 elif stage==4 and t-last>2:action('Office');action('Page7');action('SelectEmployee'+str(q['employee']['id']));action('JobRole1');stage=5;last=t
 elif stage==5 and t-last>2:q['checks']['real_welder_job_chooser_open']=ui()['screen']==1 and ui()['selected_employee']==q['employee']['id'];shot('WeldingJobChooserClear');stage=6;last=t
 elif stage==6 and t-last>2:
  if report()['running']:T.queue_factory_action(p,'Run')
  stage=7;last=t
 elif stage==7 and t-last>2:finish();return
 (base/'Logs/EmployeeWelderVisualReview.json').write_text(json.dumps(q,indent=2))
h=unreal.register_slate_post_tick_callback(tick)
