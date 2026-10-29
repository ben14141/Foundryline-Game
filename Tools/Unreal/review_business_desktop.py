"""Rendered desktop presentation review, no commerce writes; leaves the desktop open."""
import unreal,time,json,traceback,runpy
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
T=unreal.FoundrylineNetworkTestTools
start=time.monotonic();last=0;stage=0;page=0;pages=[9,0,1,2,3,10,11,4,5,6,7]
out=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()))/'Screenshots/Reviews'
out.mkdir(parents=True,exist_ok=True)
q={'checks':{},'errors':[]};report=out.parents[1]/'Logs/DesktopVisualQA.json'
def pc():
 return next(p for w in unreal.EditorLevelLibrary.get_pie_worlds(False) for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller())
def action(s):T.queue_commerce_action(pc(),s)
def finish():
 q['completed']=True;report.write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.EditorPythonScripting.set_keep_python_script_alive(False);runpy.run_path(str(Path(__file__).with_name('observe_desktop_manual.py')))
def tick(dt):
 global stage,last,page
 try:
  t=time.monotonic()-start
  if t>180:raise RuntimeError('Desktop review timeout')
  if stage==0 and t>4:
   unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1;last=t
  elif stage==1 and t-last>15:
   unreal.SystemLibrary.execute_console_command(pc().get_world(),'LevelEditor.ToggleImmersive');T.setup_operations_review(pc(),1);T.queue_factory_action(pc(),'Interact');data=json.loads(T.commerce_report(pc()));action('Order'+str(data['orders'][0]['id']));stage=2;last=t
  elif stage==2 and t-last>1:
   action('Page'+str(pages[page]));stage=3;last=t
  elif stage==3 and t-last>1:
   q['checks']['page_'+str(pages[page])]=json.loads(T.operations_ui_report(pc()))['screen']==1 and json.loads(T.operations_ui_report(pc()))['page']==pages[page]
   unreal.SystemLibrary.execute_console_command(pc().get_world(),'Shot filename="'+str(out/('Desktop_App'+str(pages[page])+'.png'))+'"');stage=4;last=t
  elif stage==4 and t-last>1:
   page+=1
   if page==len(pages):action('Page0');finish();return
   stage=2;last=t
  q['stage']=stage;report.write_text(json.dumps(q,indent=2))
 except Exception:q['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
