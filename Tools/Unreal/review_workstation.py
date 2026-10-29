"""Workstation presentation smoke: real monitor material and unchanged ERP interaction."""
import unreal,time,json,re,traceback
from pathlib import Path
T=unreal.FoundrylineNetworkTestTools
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
base=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()));out=base/'Screenshots/Reviews';log=base/'Logs/WorkstationNativeQA.json';out.mkdir(parents=True,exist_ok=True)
start=time.monotonic();last=0;stage=0;q={'checks':{},'errors':[],'screenshots':[]};oldblur=0

def pc():return next(p for w in unreal.EditorLevelLibrary.get_pie_worlds(False) for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller())
def ui():return json.loads(T.operations_ui_report(pc()))
def action(v):T.queue_commerce_action(pc(),v)
def shot(n):
 p=out/('Workstation_'+n+'.png');unreal.SystemLibrary.execute_console_command(pc().get_world(),'Shot filename="'+str(p)+'"');q['screenshots'].append(str(p))
def finish():
 q['completed']=True
 q['screenshots']=[str(max(fs,key=lambda f:f.stat().st_mtime)) for p in q['screenshots'] if (fs:=[f for f in out.iterdir() if re.fullmatch(re.escape(Path(p).stem)+r'\d{5}\.png',f.name)])]
 log.write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.EditorPythonScripting.set_keep_python_script_alive(False)
def tick(dt):
 global stage,last,oldblur
 try:
  t=time.monotonic()-start
  if t>160:raise RuntimeError('Workstation review timeout '+str(stage))
  if stage==0 and t>4:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1;last=t
  elif stage==1 and t-last>16:
   unreal.SystemLibrary.execute_console_command(pc().get_world(),'LevelEditor.ToggleImmersive');oldblur=unreal.SystemLibrary.get_console_variable_int_value('r.MotionBlurQuality');unreal.SystemLibrary.execute_console_command(pc().get_world(),'r.MotionBlurQuality 0');T.setup_operations_review(pc(),1);stage=2;last=t
  elif stage==2 and t-last>3:
   terminals=unreal.GameplayStatics.get_all_actors_of_class(pc().get_world(),unreal.FoundrylineOperationsTerminal);mats=[]
   for a in terminals:
    for c in a.get_components_by_class(unreal.StaticMeshComponent):
     for i in range(c.get_num_materials()):
      m=c.get_material(i)
      if m and 'OfficeDesktopScreen' in str(c.get_material_slot_names()[i]):mats.append({'slot':str(c.get_material_slot_names()[i]),'dynamic':isinstance(m,unreal.MaterialInstanceDynamic)})
   q['checks']['three_live_office_materials_bound']=len(mats)==3 and all(m['dynamic'] for m in mats);q['materials']=mats;q['checks']['first_person_at_computer']=ui()['walking'] and ui()['screen']==0;shot('FirstPerson');stage=3;last=t
  elif stage==3 and t-last>2:T.queue_factory_action(pc(),'Interact');stage=4;last=t
  elif stage==4 and t-last>2:q['checks']['E_opens_ERP']=ui()['screen']==1;shot('ERP');stage=5;last=t
  elif stage==5 and t-last>2:action('Page7');action('Workspace2');stage=6;last=t
  elif stage==6 and t-last>2:q['checks']['hiring_preserved']=ui()['page']==7 and ui()['workspace_tab']==2;shot('Hiring');stage=7;last=t
  elif stage==7 and t-last>2:action('Close');stage=8;last=t
  elif stage==8 and t-last>2:q['checks']['close_returns_first_person']=ui()['screen']==0 and ui()['walking'];q['final']=json.loads(T.commerce_report(pc()));q['checks']['baseline_untouched']=q['final']['cash']==400000 and not q['final']['running'] and not q['final']['pallets'];shot('ReadyToPlay');stage=9;last=t
  elif stage==9 and t-last>2:unreal.SystemLibrary.execute_console_command(pc().get_world(),'r.MotionBlurQuality '+str(oldblur));finish();return
  q['stage']=stage;log.write_text(json.dumps(q,indent=2))
 except Exception:q['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
