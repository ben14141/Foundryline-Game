import unreal,time,json,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
base=Path(unreal.Paths.project_saved_dir());out=base/'Screenshots/Reviews';start=time.monotonic();stage=0;last=0;q={'checks':{},'errors':[]}
def pc():return next(p for w in unreal.EditorLevelLibrary.get_pie_worlds(False) for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller())
def plant():return unreal.GameplayStatics.get_all_actors_of_class(pc().get_world(),unreal.FoundrylineFactoryPlant)[0]
def report():return json.loads(plant().get_factory_report())
def act(a):unreal.FoundrylineNetworkTestTools.queue_factory_action(pc(),a)
def bodies():return [c for c in plant().get_components_by_class(unreal.StaticMeshComponent) if c.get_editor_property('static_mesh') and c.get_editor_property('static_mesh').get_name()=='SM_Forklift_Body']
def rear_steers():return any(abs(c.get_editor_property('relative_rotation').yaw)>1 for c in plant().get_components_by_class(unreal.StaticMeshComponent) if c.get_editor_property('static_mesh') and c.get_editor_property('static_mesh').get_name() in ('SM_Forklift_Wheel_RL','SM_Forklift_Wheel_RR'))
def camera(p,target):
 unreal.FoundrylineNetworkTestTools.setup_workshop_review(pc(),True);c=pc().get_view_target();c.set_actor_location(p,False,False);c.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(p,target),False)
def shot(n):unreal.SystemLibrary.execute_console_command(pc().get_world(),'Shot SHOWUI filename="'+str(out/('ForkliftRouting_'+n+'.png'))+'"')
def tick(dt):
 global stage,last
 try:
  t=time.monotonic()-start
  if stage==0 and t>3:stage=1;last=t;unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
  elif stage==1 and t-last>14:
   camera(unreal.Vector(2200,6500,7000),unreal.Vector(800,2600,100));act('Toggle');act('Page2');act('Overlay');act('Order');act('Speed');stage=2;last=t
  elif stage==2 and t-last>.12:
   r=report();q['checks']['live_diagonal_route']=q['checks'].get('live_diagonal_route',False) or r.get('diagonal_segments',0)>0;q['checks']['live_rounded_route']=q['checks'].get('live_rounded_route',False) or r.get('curved_segments',0)>0;q['checks']['rear_wheel_steering']=q['checks'].get('rear_wheel_steering',False) or rear_steers()
   if r.get('curved_segments',0)>0 and rear_steers():
    p=bodies()[0].get_world_location();act('Run');camera(p+unreal.Vector(800,1100,1300),p+unreal.Vector(0,0,80));stage=3;last=t
   else:unreal.FoundrylineNetworkTestTools.advance_factory_test(pc(),1);last=t
  elif stage==3 and t-last>3:stage=4;last=t;shot('Rounded_Corner')
  elif stage==4 and t-last>3:stage=5;last=t;unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_end_play()
  elif stage==5 and t-last>3:stage=6;last=t;unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
  elif stage==6 and t-last>14:act('Toggle');act('Page2');act('Save');stage=7;last=t
  elif stage==7 and t-last>3:
   r=report();q['checks']['fresh_two_bay_scene_restored']=plant().get_zone_count()==2 and r['pallets']==29 and not r['running'] and not pc().get_factory_feedback();q['completed']=True;(base/'Logs/ForkliftRoutingVisualQA.json').write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);return
  if t>150:raise RuntimeError('Forklift render review timed out at '+str(stage))
  (base/'Logs/ForkliftRoutingVisualQA.json').write_text(json.dumps({**q,'stage':stage},indent=2))
 except Exception:
  q['errors'].append(traceback.format_exc());(base/'Logs/ForkliftRoutingVisualQA.json').write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
