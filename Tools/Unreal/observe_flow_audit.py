"""Read-only recording during ordinary player UI flow; no time or stock grants."""
import unreal,time,json,traceback,builtins
from pathlib import Path
T=unreal.FoundrylineNetworkTestTools;start=time.monotonic();last=0
out=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()))/'Logs/PlayFlowAudit.json'
q={'samples':[],'errors':[],'completed':False}
def stop():q['completed']=True;out.write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle)
def tick(dt):
 global last
 try:
  t=time.monotonic()-start
  if t-last<2:return
  last=t
  ps=[p for w in unreal.EditorLevelLibrary.get_pie_worlds(False) for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller()]
  if not ps:return
  p=ps[0];q['samples'].append({'seconds':t,'commerce':json.loads(T.commerce_report(p)),'staff':json.loads(T.employee_report(p)),'ui':json.loads(T.operations_ui_report(p))});out.write_text(json.dumps(q,indent=2))
  if t>900:stop()
 except Exception:q['errors'].append(traceback.format_exc());stop()
handle=unreal.register_slate_post_tick_callback(tick);builtins.foundryline_stop_flow_audit=stop
