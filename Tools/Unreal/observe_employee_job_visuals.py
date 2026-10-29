"""Read-only native employee-job presentation probe.
Set builtins.foundryline_job_visual_mode to 'starter' or 'welder' before exec(text,{}).
Each physical-world fixture has its own explicit checks/report; no state mutations.
"""
import unreal,time,json,traceback,builtins,math
from pathlib import Path
mode=getattr(builtins,'foundryline_job_visual_mode','starter');mode='welder' if mode=='welder' else 'starter'
names=['unique_employee_bodies','unassigned_people_on_foot','same_visual_component_across_job_changes']
names+=['welder_walks_with_gait','same_employee_unassigned_then_welder','welder_glove_tracks_actual_gun','native_welding_sparks_at_gun_tip','idle_gun_is_holstered'] if mode=='welder' else ['operator_walks_with_gait','driver_seated_collision_disabled','same_employee_operator_then_driver']
q={'scope':mode,'checks':{n:False for n in names},'samples':0,'errors':[],'duplicate_samples':0,'role_history':{},'component_paths':{},'grip_samples':0,'aligned_grip_samples':0,'max_grip_error_cm':0,'max_sparks':0}
out=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()))/'Logs'/('EmployeeJobVisual_'+mode+'.json')
start=time.monotonic();last=0;previous={};components={};roles={}
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
def finish():
 q['completed']=True;out.write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle)
builtins.foundryline_stop_job_visual_probe=finish
def tick(dt):
 global last
 try:
  now=time.monotonic()
  if now-last<.1:return
  last=now;worlds=unreal.EditorLevelLibrary.get_pie_worlds(False)
  if worlds:
   w=worlds[0];pcs=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController);rows={}
   if pcs:
    report=json.loads(unreal.FoundrylineNetworkTestTools.employee_report(pcs[0]));rows={r['key']:r for r in report.get('employees',[])}
   seen=[]
   for plant in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.FoundrylineFactoryPlant):
    for c in plant.get_components_by_class(unreal.FoundrylineEmployeeVisualComponent):
     r=json.loads(c.get_presentation_report());key=r['key'];seen.append(key);components.setdefault(key,set()).add(c.get_path_name());row=rows.get(key);v=c.get_world_location();p=(v.x,v.y,v.z);old=previous.get(key)
     if row:
      role=int(row['role']);roles.setdefault(key,set()).add(role)
      if role==3 and not r['seated'] and r['walkingCollision']:q['checks']['unassigned_people_on_foot']=True
      moved=old and r['moving'] and math.dist(p,old['p'])>1 and abs(r.get('knee0Pitch',0)-old['r'].get('knee0Pitch',0))>.1
      if mode=='starter':
       if role==0 and moved:q['checks']['operator_walks_with_gait']=True
       if role==2 and r['seated'] and not r['walkingCollision']:q['checks']['driver_seated_collision_disabled']=True
      else:
       if role==1 and moved:q['checks']['welder_walks_with_gait']=True
       error=r.get('weldingGripErrorCm',-1)
       if role==1 and {1,3}.issubset(roles[key]) and r['activity']=='Welding' and error>=0:
        q['grip_samples']+=1;q['max_grip_error_cm']=max(q['max_grip_error_cm'],error)
        if error<1:q['aligned_grip_samples']+=1
     previous[key]={'p':p,'r':r}
    if mode=='welder':
     for effect in plant.get_components_by_class(unreal.FoundrylineWeldingEffectComponent):
      gun=effect.get_attach_parent()
      if not gun:continue
      if not effect.is_welding() and not gun.is_visible():q['checks']['idle_gun_is_holstered']=True
      sparks=effect.get_visible_spark_count();q['max_sparks']=max(q['max_sparks'],sparks)
      if effect.is_welding() and sparks>0:
       tip=gun.get_socket_location('WeldTip');at=effect.get_world_location()
       if math.dist((tip.x,tip.y,tip.z),(at.x,at.y,at.z))<.1:q['checks']['native_welding_sparks_at_gun_tip']=True
   if seen:
    q['samples']+=1
    if len(seen)!=len(set(seen)):q['duplicate_samples']+=1
    q['checks']['unique_employee_bodies']=q['duplicate_samples']==0
    transferred=[k for k,v in roles.items() if len(v)>1]
    q['checks']['same_visual_component_across_job_changes']=bool(transferred) and all(len(components[k])==1 for k in transferred)
    if mode=='starter':q['checks']['same_employee_operator_then_driver']=any({0,2,3}.issubset(v) for v in roles.values())
    else:
     q['checks']['same_employee_unassigned_then_welder']=any({1,3}.issubset(v) for v in roles.values())
     q['checks']['welder_glove_tracks_actual_gun']=q['aligned_grip_samples']>=5
    q['role_history']={k:sorted(v) for k,v in roles.items()};q['component_paths']={k:sorted(v) for k,v in components.items()}
  out.write_text(json.dumps(q,indent=2))
  if now-start>300:finish()
 except Exception:
  q['errors'].append(traceback.format_exc());out.write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
