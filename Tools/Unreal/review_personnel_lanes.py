import unreal,time,json,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
base=Path(unreal.Paths.project_saved_dir());out=base/'Screenshots/Reviews';start=time.monotonic();stage=0;last=0;q={'checks':{},'errors':[]}
def pc():return next(p for w in unreal.EditorLevelLibrary.get_pie_worlds(False) for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller())
def plant():return unreal.GameplayStatics.get_all_actors_of_class(pc().get_world(),unreal.FoundrylineFactoryPlant)[0]
def act(a):unreal.FoundrylineNetworkTestTools.queue_factory_action(pc(),a)
def camera(location,target):
 unreal.FoundrylineNetworkTestTools.setup_workshop_review(pc(),True);c=pc().get_view_target();c.set_actor_location(location,False,False);c.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(location,target),False)
def shot(name):unreal.SystemLibrary.execute_console_command(pc().get_world(),'Shot SHOWUI filename="'+str(out/('PersonnelLanes_'+name+'.png'))+'"')
def tick(dt):
 global stage,last
 try:
  t=time.monotonic()-start
  if stage==0 and t>3:stage=1;last=t;unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
  elif stage==1 and t-last>14:camera(unreal.Vector(2200,6500,7000),unreal.Vector(800,2600,100));act('Toggle');act('Page2');stage=2;last=t
  elif stage==2 and t-last>4:stage=3;last=t;shot('Solid_People_And_Work_Fill')
  elif stage==3 and t-last>3:camera(unreal.Vector(-500,1300,2000),unreal.Vector(-300,200,20));stage=4;last=t
  elif stage==4 and t-last>4:stage=5;last=t;shot('Automatic_Crossing')
  elif stage==5 and t-last>3:
   a=unreal.Vector(1855.6,948.96,20);r=json.loads(unreal.FoundrylineNetworkTestTools.preview_factory_brush(pc(),'Zone',a,a,False,True));q['checks']['work_zone_over_personnel_lane_blocked']=not r['valid'] and not r['sent'];act('ToolDone');stage=6;last=t
  elif stage==6 and t-last>2:
   q['checks']['both_work_zones_preserved']=plant().get_zone_count()==2;q['checks']['stock_preserved']=json.loads(plant().get_factory_report())['pallets']==29
   camera(unreal.Vector(2200,6500,7000),unreal.Vector(800,2600,100));act('Save');stage=7;last=t
  elif stage==7 and t-last>3:
   q['checks']['normal_layout_saved']=not pc().get_factory_feedback();q['completed']=True;(base/'Logs/PersonnelLanesVisualQA.json').write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);return
  if t>140:raise RuntimeError('Personnel review timeout '+str(stage))
  (base/'Logs/PersonnelLanesVisualQA.json').write_text(json.dumps({**q,'stage':stage},indent=2))
 except Exception:
  q['errors'].append(traceback.format_exc());(base/'Logs/PersonnelLanesVisualQA.json').write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
