import unreal,time,json,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
base=Path(unreal.Paths.project_saved_dir());out=base/'Screenshots/Reviews';start=time.monotonic();stage=0;last=0;frozen={};samples=[];q={'checks':{},'errors':[]}
def pc():return next(p for w in unreal.EditorLevelLibrary.get_pie_worlds(False) for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller())
def plant():return unreal.GameplayStatics.get_all_actors_of_class(pc().get_world(),unreal.FoundrylineFactoryPlant)[0]
def report():return json.loads(unreal.FoundrylineNetworkTestTools.equipment_motion_report(pc()))
def act(a):unreal.FoundrylineNetworkTestTools.queue_factory_action(pc(),a)
def camera(p,target):
 unreal.FoundrylineNetworkTestTools.setup_workshop_review(pc(),True);c=pc().get_view_target();c.set_actor_location(p,False,False);c.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(p,target),False)
def shot(n):unreal.SystemLibrary.execute_console_command(pc().get_world(),'Shot SHOWUI filename="'+str(out/('EquipmentMotion_'+n+'.png'))+'"')
def tick(dt):
 global stage,last,frozen
 try:
  t=time.monotonic()-start
  if 2<=stage<=12:
   if stage<=9:samples.append(report())
   for c in plant().get_components_by_class(unreal.FoundrylineWeldingEffectComponent):
    if c.is_welding() and not c.get_editor_property('bPlasmaCutting'):
     v=c.get_world_location();pos=[v.x,v.y,v.z];q['checks']['welding_native_sparks']=q['checks'].get('welding_native_sparks',False) or c.get_visible_spark_count()>0
     old=q.get('weld_position',pos);q['checks']['welding_gun_moves']=q['checks'].get('welding_gun_moves',False) or sum((a-b)**2 for a,b in zip(pos,old))>.01;q['weld_position']=pos
  if stage==0 and t>3:stage=1;last=t;unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
  elif stage==1 and t-last>14:
   q['checks']['real_stock_exercise']=unreal.FoundrylineNetworkTestTools.start_equipment_exercise(pc());camera(unreal.Vector(-600,1700,820),unreal.Vector(-1300,800,130));stage=2;last=t
  elif stage==2 and t-last>4:stage=3;last=t;shot('Plasma')
  elif stage==3 and t-last>2:camera(unreal.Vector(-750,2950,670),unreal.Vector(-1248,2300,160));stage=4;last=t
  elif stage==4 and t-last>2:stage=5;last=t;shot('Press')
  elif stage==5 and t-last>2:camera(unreal.Vector(-550,4250,570),unreal.Vector(-1300,3800,135));stage=6;last=t
  elif stage==6 and t-last>2:stage=7;last=t;shot('Roller')
  elif stage==7 and t-last>2:act('Run');stage=8;last=t
  elif stage==8 and t-last>1:frozen=report();stage=9;last=t
  elif stage==9 and t-last>.8:
   r=report();q['checks']['paused_physical_pose']=not r['running'] and not r['cutting'] and abs(r['ram']-frozen['ram'])<.01 and abs(r['roller']-frozen['roller'])<.01;act('Run');stage=10;last=t
  elif stage==10 and t-last>8 and all(report()['remaining'+str(i)]==0 for i in range(1,4)):
   q['checks']['plasma_native_sparks']=any(s['cutting'] and s['sparks']>0 for s in samples);q['checks']['ram_travel']=max(s['ram'] for s in samples)-min(s['ram'] for s in samples)>25;q['checks']['roller_forms_stock']=any(s['forming_visible'] and s['forming_segments']==32 and abs(s['forming_angle'])>5 for s in samples);q['checks']['cycles_complete']=all(report()['remaining'+str(i)]==0 for i in range(1,4));act('Order');camera(unreal.Vector(2200,2100,650),unreal.Vector(1591,1455,150));stage=11;last=t
  elif stage==11 and t-last>5 and q['checks'].get('welding_native_sparks',False) and q['checks'].get('welding_gun_moves',False):stage=12;last=t;shot('Welding')
  elif stage==12 and t-last>3:stage=13;last=t;unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_end_play()
  elif stage==13 and t-last>3:stage=14;last=t;unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
  elif stage==14 and t-last>14:act('Toggle');act('Page2');act('Save');stage=15;last=t
  elif stage==15 and t-last>3:
   r=json.loads(plant().get_factory_report());q['checks']['fresh_scene_restored']=plant().get_zone_count()==2 and r['pallets']==29 and not r['running'];q['completed']=True;(base/'Logs/EquipmentMotionVisualQA.json').write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);return
  if t>240:raise RuntimeError('Equipment review timeout '+str(stage))
  (base/'Logs/EquipmentMotionVisualQA.json').write_text(json.dumps({**q,'stage':stage},indent=2))
 except Exception:
  q['errors'].append(traceback.format_exc());(base/'Logs/EquipmentMotionVisualQA.json').write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
