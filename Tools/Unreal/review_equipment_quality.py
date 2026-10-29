"""Transient standard-workshop equipment review; no normal saves or cash/stock grants."""
import unreal,time,json,traceback,re
from pathlib import Path
T=unreal.FoundrylineNetworkTestTools
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
base=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()));out=base/'Screenshots/Reviews'
label=globals().get('REVIEW_LABEL','Before');log=base/'Logs'/('EquipmentQuality_'+label+'.json');start=time.monotonic();stage=0;last=0;cam=None;samples=[];frozen=None
q={'fixture':'standard stocked workshop, real existing stock one cycle per machine; no user saves','checks':{},'screenshots':[],'errors':[]};machine_probe=1
def pc():return next(p for w in unreal.EditorLevelLibrary.get_pie_worlds(False) for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller())
def report():
 r=json.loads(T.equipment_motion_report(pc()))
 for a in unreal.GameplayStatics.get_all_actors_of_class(pc().get_world(),unreal.FoundrylineMaterialHandlingCell):
  parts={c.get_name():c for c in a.get_components_by_class(unreal.StaticMeshComponent)}
  upper=parts.get('SheetRoller_Upper');carrier=parts.get('SheetRoller_UpperCarriers')
  if upper and carrier:
   delta=upper.get_world_location()-carrier.get_world_location();r['carrier_shaft_alignment_cm']=(delta.x**2+delta.y**2+delta.z**2)**.5
 return r
def camera(i):
 global cam
 if cam is None:cam=T.create_review_camera(pc())
 centres=[unreal.Vector(-1300,800,130),unreal.Vector(-1300,2300,150),unreal.Vector(-1300,3800,130)]
 c=centres[i];eye=c+unreal.Vector(350,40,230)
 cam.set_actor_location(eye,False,False);cam.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(eye,c),False)
 cc=cam.get_components_by_class(unreal.CameraComponent)[0];cc.set_field_of_view(80);pp=cc.get_editor_property('post_process_settings');pp.set_editor_property('override_auto_exposure_bias',True);pp.set_editor_property('auto_exposure_bias',5.0);cc.set_editor_property('post_process_settings',pp);cc.set_editor_property('post_process_blend_weight',1.0);pc().set_view_target_with_blend(cam,0)
def shot(name):
 p=out/('EquipmentQuality_'+label+'_'+name+'.png');unreal.SystemLibrary.execute_console_command(pc().get_world(),'Shot filename="'+str(p)+'"');q['screenshots'].append(str(p))
def finish():
 q['completed']=True;q['samples']=samples;q['screenshots']=[str(max(fs,key=lambda f:f.stat().st_mtime)) for p in q['screenshots'] if(fs:=[f for f in out.iterdir() if re.fullmatch(re.escape(Path(p).stem)+r'\d{5}\.png',f.name)])];log.write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(h);unreal.EditorPythonScripting.set_keep_python_script_alive(False)
def tick(dt):
 global stage,last,frozen,machine_probe
 try:
  t=time.monotonic()-start
  if t>130:raise RuntimeError('Equipment review timeout '+str(stage))
  if stage>=2 and stage<10:samples.append(report())
  if stage==0 and t>3:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1;last=t
  elif stage==1 and t-last>16:
   T.queue_factory_action(pc(),'WalkView');stage=15;last=t
  elif stage==15 and t-last>1:
   unreal.SystemLibrary.execute_console_command(pc().get_world(),'r.MotionBlurQuality 0');q['checks']['real_stock_cycle_started']=T.start_equipment_exercise(pc());camera(0);stage=2;last=t
  elif stage==2 and t-last>2:shot('Plasma');stage=3;last=t
  elif stage==3 and t-last>1:camera(1);stage=4;last=t
  elif stage==4 and t-last>2:shot('Brake');stage=5;last=t
  elif stage==5 and t-last>1:camera(2);stage=6;last=t
  elif stage==6 and t-last>2:shot('Roller');T.queue_factory_action(pc(),'Run');stage=7;last=t
  elif stage==7 and t-last>1:frozen=report();stage=8;last=t
  elif stage==8 and t-last>1:
   r=report();q['checks']['paused_mechanical_pose']=not r['running'] and not r['cutting'] and abs(r['ram']-frozen['ram'])<.01 and abs(r['roller']-frozen['roller'])<.01;T.queue_factory_action(pc(),'Run');stage=9;last=t
  elif stage==9 and t-last>5 and all(report()['remaining'+str(i)]==0 for i in range(1,4)):
   q['checks']['all_real_cycles_complete']=True;q['checks']['plasma_cutting_sparks']=any(s.get('cutting') and s.get('sparks',0)>0 for s in samples);q['checks']['ram_moves']=max(s['ram'] for s in samples)-min(s['ram'] for s in samples)>25;q['checks']['roller_moves_and_forms']=len({round(s['roller'],1) for s in samples})>2 and any(s.get('forming_visible') for s in samples);q['checks']['roller_bearings_follow_actual_shaft']=all(s.get('carrier_shaft_alignment_cm',999)<.01 for s in samples);stage=10;last=t
  elif stage==10 and t-last>1:T.queue_factory_console(pc(),machine_probe);stage=11;last=t
  elif stage==11 and t-last>1:
   q['checks']['machine_'+str(machine_probe)+'_first_person_reachable']=json.loads(T.operations_ui_report(pc()))['focused_machine']==machine_probe;T.queue_factory_action(pc(),'Interact');stage=12;last=t
  elif stage==12 and t-last>1:
   state=json.loads(T.operations_ui_report(pc()));q['checks']['machine_'+str(machine_probe)+'_E_opens_controls']=state['machine']==machine_probe;q.setdefault('machine_UI',{})[str(machine_probe)]=state;T.queue_factory_action(pc(),'CloseComputer');stage=13;last=t
  elif stage==13 and t-last>1:
   if machine_probe<3:machine_probe+=1;stage=10;last=t
   else:finish();return
  q['stage']=stage;log.write_text(json.dumps(q,indent=2))
 except Exception:q['errors'].append(traceback.format_exc());finish()
h=unreal.register_slate_post_tick_callback(tick)
