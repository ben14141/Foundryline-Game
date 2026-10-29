"""Read-only PIE employee visual acceptance. No inventory, camera, save or state mutation."""
import unreal,time,json,traceback,builtins
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()))/'Logs/EmployeeVisualProbe.json'
start=time.monotonic();last=0;previous={};idle_seen={}
q={'checks':{k:False for k in ['unique_company_plant_employee_keys','walking_moves_articulated_legs','idle_gait_stops','walking_feet_touch_floor','working_wrists_reach_keyboard','working_arms_move','paused_pose_frozen','driver_seated_on_forklift']},'samples':0,'errors':[],'employees':{},'max_count':0}
def finish_employee_visual_probe():
 q['completed']=True;out.write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle)
builtins.foundryline_stop_employee_visual_probe=finish_employee_visual_probe
def tick(dt):
 global last
 try:
  now=time.monotonic()
  if now-last<.1:return
  last=now;worlds=unreal.EditorLevelLibrary.get_pie_worlds(False)
  if worlds:
   seen=[];all_unique=True
   for plant in unreal.GameplayStatics.get_all_actors_of_class(worlds[0],unreal.FoundrylineFactoryPlant):
    for c in plant.get_components_by_class(unreal.FoundrylineEmployeeVisualComponent):
     r=json.loads(c.get_presentation_report());key=r['key'];seen.append(key);p=c.get_world_location();position=(p.x,p.y,p.z);old=previous.get(key)
     if old and r['moving'] and sum((position[i]-old['position'][i])**2 for i in range(2))>1 and abs(r.get('knee0Pitch',0)-old['report'].get('knee0Pitch',0))>.1:q['checks']['walking_moves_articulated_legs']=True
     if old and not r['moving'] and not r['seated'] and not old['report']['moving'] and r['activity'] in ['Idle','Paused','Waiting'] and r['gaitPhase']==old['report']['gaitPhase']:q['checks']['idle_gait_stops']=True
     if old and r['activity']=='Paused' and old['report']['activity']=='Paused' and r['gaitPhase']==old['report']['gaitPhase'] and all(abs(r.get('shoulder%dPitch'%i,0)-old['report'].get('shoulder%dPitch'%i,0))<.001 for i in range(2)):q['checks']['paused_pose_frozen']=True
     if old and r['activity'] in ['Operating','Welding'] and old['report']['activity']==r['activity'] and abs(r.get('shoulder0Pitch',0)-old['report'].get('shoulder0Pitch',0))>.01:q['checks']['working_arms_move']=True
     if r['moving'] and not r['seated'] and all(r.get('foot%dWorldZ'%i,-999)>=r['footRootWorldZ']-.2 for i in range(2)) and min(r.get('foot%dWorldZ'%i,999) for i in range(2))<=r['footRootWorldZ']+.3:q['checks']['walking_feet_touch_floor']=True
     if r['activity'] in ['Operating','Welding'] and abs(r.get('hand0LocalZ',0)-113)<2 and 34<r.get('hand0LocalX',0)<38:q['checks']['working_wrists_reach_keyboard']=True
     if r['seated'] and not r['walkingCollision'] and 101<r.get('foot0WorldZ',0)<105:q['checks']['driver_seated_on_forklift']=True
     previous[key]={'position':position,'report':r};q['employees'][key]=r
   if seen:q['checks']['unique_company_plant_employee_keys']=len(set(seen))==len(seen);q['max_count']=max(q['max_count'],len(seen));q['samples']+=1
  if now-start>220:finish_employee_visual_probe()
  out.write_text(json.dumps(q,indent=2))
 except Exception:
  q['errors'].append(traceback.format_exc());out.write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
