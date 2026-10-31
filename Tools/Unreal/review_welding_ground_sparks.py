"""Rendered floor-impact and replicated welding VFX review. No saves or production changes."""
import unreal,time,json,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
root=Path(unreal.Paths.project_dir()).resolve();saved=root/'Saved';shots=saved/'Screenshots/Reviews';shots.mkdir(parents=True,exist_ok=True)
exec((root/'Tools/Unreal/prepare_welding_sparks.py').read_text())
T=unreal.FoundrylineNetworkTestTools
start=time.monotonic();stage=0;last=0;camera=None;demo=None;busy=False;taken=set()
report={'checks':{},'errors':[],'max_sparks':0,'ground_impacts':0,'completed':False}
def players():return [p for w in unreal.EditorLevelLibrary.get_pie_worlds(False) for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller()]
def host():return next(p for p in players() if p.has_authority())
def effects():return [a.welding_effect for w in unreal.EditorLevelLibrary.get_pie_worlds(False) for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.FoundrylineWeldingStation) if a.get_actor_location().y<2300]
def look(loc,target):
 global camera
 if not camera:camera=T.create_review_camera(host())
 camera.set_actor_location_and_rotation(loc,unreal.MathLibrary.find_look_at_rotation(loc,target),False,False);camera.camera_component.set_field_of_view(60);host().set_view_target_with_blend(camera,0)
def shot(name):
 global busy
 if name in taken:return
 taken.add(name);busy=True
 try:unreal.AutomationLibrary.take_high_res_screenshot(1600,1000,str(shots/(name+'.png')))
 finally:busy=False
def finish():
 report['completed']=True;(saved/'Logs/WeldingGroundSparkQA.json').write_text(json.dumps(report,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(host(),'QUIT_EDITOR')
def tick(dt):
 global stage,last,demo
 if busy:return
 try:
  now=time.monotonic()-start
  if now>110:raise RuntimeError('Welding review timeout at '+str(stage))
  if stage==0 and now>5:
   unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(unreal.FoundrylineWeldingStation,unreal.Vector(-1800,1800,20))
   stage=1;last=now;unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
  elif stage==1 and now-last>12 and players():
   demo=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(host(),unreal.FoundrylineWeldingStation) if a.get_actor_location().y<2300)
   unreal.SystemLibrary.execute_console_command(host(),'showhud');look(unreal.Vector(-1390,1490,230),unreal.Vector(-1675,1800,85));stage=2;last=now
  elif stage>=2:
   es=effects();report['peers']=len(es);report['checks']['four_players']=len(players())==4
   if es:
    report['max_sparks']=max(report['max_sparks'],max(e.get_visible_spark_count() for e in es));report['ground_impacts']=max(e.get_ground_impact_count() for e in es)
    report['checks']['bounded_particles']=all(e.get_visible_spark_count()<=640 for e in es)
    report['checks']['all_peers_emit']=len(es)==4 and all(e.get_ground_impact_count()>0 for e in es)
    report['checks']['sparks_hit_real_floor']=report['ground_impacts']>0
    report['checks']['replicated_welding_observed']=report['checks'].get('replicated_welding_observed',False) or (len(es)==4 and all(e.is_welding() for e in es))
    report['checks']['dense_close_sparks']=report['max_sparks']>250
    if stage==2 and now-last>4 and demo.welding_effect.is_welding() and demo.welding_effect.get_visible_spark_count()>250:
     shot('Welding_Improved_Arc');stage=3;last=now
    elif stage==3 and now-last>2:
     look(unreal.Vector(-1420,1600,95),unreal.Vector(-1685,1800,57));stage=4;last=now
    elif stage==4 and now-last>4 and demo.welding_effect.is_welding() and demo.welding_effect.get_ground_impact_count()>0:
     shot('Welding_Floor_Sparks');stage=5;last=now
    elif stage==5 and now-last>3:finish();return
  report['stage']=stage;(saved/'Logs/WeldingGroundSparkQA.json').write_text(json.dumps(report,indent=2))
 except Exception:report['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
