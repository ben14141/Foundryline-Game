"""Read-only physical employee UI interaction sampler. No game commands."""
import unreal,time,json,traceback
from pathlib import Path
T=unreal.FoundrylineNetworkTestTools
out=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()))/'Logs/EmployeeManualState.json'
start=time.monotonic();last='';q={'transitions':[],'errors':[]}
def tick(dt):
 global last
 try:
  worlds=unreal.EditorLevelLibrary.get_pie_worlds(False)
  if not worlds:return
  p=next(p for w in worlds for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller())
  u=json.loads(T.operations_ui_report(p));e=json.loads(T.employee_report(p));c=json.loads(T.commerce_report(p))
  s={'screen':u['screen'],'page':u['page'],'walking':u['walking'],'company':e.get('company'),'plant':e.get('plant'),'employees':[{k:x[k] for k in ['key','id','role','assignment','name','vehicle']} for x in e.get('employees',[])],'cash':c['cash'],'pallets':len(c['pallets']),'running':c['running']}
  key=json.dumps(s,sort_keys=True)
  if key!=last:s['seconds']=round(time.monotonic()-start,2);q['transitions'].append(s);last=key;out.write_text(json.dumps(q,indent=2))
  if time.monotonic()-start>300:unreal.unregister_slate_post_tick_callback(handle)
 except Exception:q['errors'].append(traceback.format_exc());out.write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
