"""Read-only PIE hire/body acceptance; native flow publishes expected stable keys.
builtins.foundryline_hire_visual_expected = {'Operator': key, 'Welder': key, 'Driver': key}
Optional builtins.foundryline_released_driver_key identifies the existing seated driver.
Stop with builtins.foundryline_stop_hire_visual_probe(). No game state or camera edits.
"""
import unreal,time,json,traceback,builtins,math
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()))/'Logs'/getattr(builtins,'foundryline_hire_visual_report','HiredEmployeeVisualProbe.json')
start=time.monotonic();last=0;previous={};foot_seen=set();seat_seen=set();idle_positions={}
q={'checks':{k:False for k in ['unique_employee_bodies','three_hires_appear_on_foot','idle_hires_have_separate_waiting_positions','hired_operator_walks_with_same_key','hired_driver_seats_with_same_key','released_driver_returns_to_foot']},'samples':0,'errors':[],'duplicate_key_samples':0,'max_count':0,'count_history':[],'keys':[],'expected':{},'waiting_positions':{},'minimum_waiting_distance_cm':None}
def finish():
 q['completed']=True;out.write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle)
builtins.foundryline_stop_hire_visual_probe=finish
def tick(dt):
 global last
 try:
  now=time.monotonic()
  if now-last<.1:return
  last=now;worlds=unreal.EditorLevelLibrary.get_pie_worlds(False)
  if worlds:
   expected=getattr(builtins,'foundryline_hire_visual_expected',{});q['expected']=dict(expected);keys=[]
   for plant in unreal.GameplayStatics.get_all_actors_of_class(worlds[0],unreal.FoundrylineFactoryPlant):
    for c in plant.get_components_by_class(unreal.FoundrylineEmployeeVisualComponent):
     r=json.loads(c.get_presentation_report());key=r['key'];keys.append(key);v=c.get_world_location();p=(v.x,v.y,v.z);old=previous.get(key)
     if r['seated']:seat_seen.add(key)
     else:
      foot_seen.add(key)
      if not r['moving'] and r['activity'] in ['Idle','Paused','Waiting'] and abs(p[2]-20)<.5:idle_positions.setdefault(key,p)
     if key==expected.get('Operator') and key in foot_seen and old and r['moving'] and math.dist(p,old['p'])>1 and abs(r.get('knee0Pitch',0)-old['r'].get('knee0Pitch',0))>.1:q['checks']['hired_operator_walks_with_same_key']=True
     if key==expected.get('Driver') and key in foot_seen and r['seated']:q['checks']['hired_driver_seats_with_same_key']=True
     if key==getattr(builtins,'foundryline_released_driver_key',None) and key in seat_seen and not r['seated']:q['checks']['released_driver_returns_to_foot']=True
     previous[key]={'p':p,'r':r}
   if keys:
    if len(keys)!=len(set(keys)):q['duplicate_key_samples']+=1
    q['checks']['unique_employee_bodies']=q['duplicate_key_samples']==0;q['max_count']=max(q['max_count'],len(keys));q['keys']=sorted(keys);q['samples']+=1
    if not q['count_history'] or q['count_history'][-1]['count']!=len(keys):q['count_history'].append({'seconds':round(now-start,2),'count':len(keys)})
   wanted=set(expected.values())
   q['checks']['three_hires_appear_on_foot']=len(wanted)==3 and wanted.issubset(foot_seen)
   if len(wanted)==3 and wanted.issubset(idle_positions):
    points=[idle_positions[k] for k in sorted(wanted)];minimum=min(math.dist(points[a],points[b]) for a in range(3) for b in range(a+1,3));q['minimum_waiting_distance_cm']=minimum;q['waiting_positions']={k:list(idle_positions[k]) for k in wanted};q['checks']['idle_hires_have_separate_waiting_positions']=minimum>=70
  out.write_text(json.dumps(q,indent=2))
  if now-start>300:finish()
 except Exception:
  q['errors'].append(traceback.format_exc());out.write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
