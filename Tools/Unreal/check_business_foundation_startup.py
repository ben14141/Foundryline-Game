import unreal,time,json,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
start=time.monotonic();stage=0;q={'checks':{},'errors':[]};out=Path(unreal.Paths.project_saved_dir())/'Logs/BusinessFoundationStartupQA.json'
def tick(dt):
 global stage
 try:
  t=time.monotonic()-start
  if stage==0 and t>3:stage=1;unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
  elif stage==1 and t>18:
   w=unreal.EditorLevelLibrary.get_pie_worlds(False)[0];p=next(p for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller());r=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.FoundrylineBusinessRegistry)[0];plant=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.FoundrylineFactoryPlant)[0];s=json.loads(plant.get_factory_report());own=json.loads(unreal.FoundrylineNetworkTestTools.own_business_report(p));q['checks']['normal_coop_business']=r.get_company_count()==1 and own.get('has_company',False) and 'plant_digest' in own;q['checks']['starter_stays_fresh_and_paused']=s['raw']==10 and not s['running'] and plant.get_zone_count()==0;q['completed']=True;out.write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);return
  if t>90:raise RuntimeError('Startup timeout')
 except Exception:q['errors'].append(traceback.format_exc());out.write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
