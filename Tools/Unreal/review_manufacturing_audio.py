"""Four-listener runtime audio routing and near/far/paused mix capture. Unsaved review only."""
import unreal,time,json,traceback,os
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
root=Path(unreal.Paths.project_dir()).resolve();saved=root/'Saved';out=saved/'AudioReview/Mixes';out.mkdir(parents=True,exist_ok=True)
exec((root/'Tools/Unreal/import_manufacturing_audio.py').read_text())
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/Foundryline/Maps/L_FlatForest_Grass')
SINGLE='-FoundryAudioSingle' in unreal.SystemLibrary.get_command_line()
submix=unreal.load_asset('/Game/Foundryline/Audio/Manufacturing/SM_Manufacturing');submix.set_editor_property('auto_disable',False)
if not SINGLE:out=saved/'AudioReview/MultiplayerMixes';out.mkdir(parents=True,exist_ok=True)
REPORT=saved/'Logs'/('ManufacturingAudioMixQA.json' if SINGLE else 'ManufacturingAudioRuntimeQA.json')
T=unreal.FoundrylineNetworkTestTools;start=time.monotonic();stage=0;last=0;cameras=[];demo=None
q={'checks':{},'errors':[],'samples':[],'completed':False}
def players():return [p for w in unreal.EditorLevelLibrary.get_pie_worlds(False) for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller()]
def host():return next(p for p in players() if p.has_authority())
def reports():return [json.loads(unreal.FoundrylineManufacturingAudio.for_world(p).get_audio_report()) for p in players()]
def look(far=False):
 global cameras
 if not cameras:cameras=[T.create_review_camera(p) for p in players()]
 for p,c in zip(players(),cameras):
  at=unreal.Vector(-900,2200,250 if not far else 9000);target=unreal.Vector(-1300,2200,100)
  c.set_actor_location_and_rotation(at,unreal.MathLibrary.find_look_at_rotation(at,target),False,False);p.set_view_target_with_blend(c,0)
def record():unreal.AudioMixerLibrary.start_recording_output(host(),16,submix)
def stop(name):unreal.AudioMixerLibrary.stop_recording_output(host(),unreal.AudioRecordingExportType.WAV_FILE,name,str(out),submix)
def finish():
 q['completed']=True;REPORT.write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(host(),'QUIT_EDITOR')
def tick(dt):
 global stage,last,demo
 try:
  now=time.monotonic()-start
  if now>160:raise RuntimeError('Audio review timeout '+str(stage))
  if stage==0 and now>4:
   unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(unreal.FoundrylineWeldingStation,unreal.Vector(-1300,1800,20))
   stage=1;last=now;unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
  elif stage==1 and now-last>14 and players():
   q['peers']=len(players());unreal.SystemLibrary.execute_console_command(host(),'SoloAudio');unreal.SystemLibrary.execute_console_command(host(),'au.DisableAppVolume 1')
   q['checks']['listeners_connected']=len(players())==(1 if SINGLE else 4)
   for p in players():T.queue_property_view(p)
   look();stage=2;last=now
  elif stage==2 and now-last>3:
   q['checks']['valid_equipment_exercise']=T.start_equipment_exercise(host());assert q['checks']['valid_equipment_exercise'],'Full workshop exercise failed'
   stage=3;last=now
  elif stage==3:
   rs=reports();q['samples'].append(rs)
   q['checks']['all_profiles_loaded']=all(r['missing_assets']==0 for r in rs)
   q['checks']['voice_budget']=q['checks'].get('voice_budget',True) and all(r['voices']<=8 for r in rs)
   if now-last>5:record();stage=4;last=now
  elif stage==4:
   rs=reports();q['samples'].append(rs)
   roles={s['role'] for r in rs for s in r['sources'] if s['playing']}
   q['roles_observed']=sorted(set(q.get('roles_observed',[]))|roles)
   if now-last>8:
    stop('Manufacturing_Near');T.queue_factory_action(host(),'Run')
    for a in unreal.GameplayStatics.get_all_actors_of_class(host(),unreal.FoundrylineWeldingStation):a.destroy_actor()
    stage=5;last=now
  elif stage==5 and now-last>2:
   q['checks']['paused_factory_quiet']=all(r['voices']==0 for r in reports());record();stage=6;last=now
  elif stage==6 and now-last>5:
   stop('Manufacturing_Paused');T.queue_factory_action(host(),'Run');look(True);stage=7;last=now
  elif stage==7 and now-last>3:
   q['checks']['distant_sources_culled']=all(r['voices']==0 for r in reports());record();stage=8;last=now
  elif stage==8 and now-last>5:stop('Manufacturing_Far');stage=9;last=now
  elif stage==9 and now-last>3:
   q['checks']['machines_and_welding_observed']=all(x in q.get('roles_observed',[]) for x in ['WeldingArc','PlasmaGantry','BrakeHydraulics','SheetRoller']);finish();return
  q['stage']=stage;REPORT.write_text(json.dumps(q,indent=2))
 except Exception:q['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
