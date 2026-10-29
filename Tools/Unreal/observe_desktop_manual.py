"""Read-only state sampler for physical desktop mouse/keyboard acceptance."""
import unreal,json,time,traceback
from pathlib import Path
T=unreal.FoundrylineNetworkTestTools
base=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()))
log=base/'Logs/ERPDesktopV2ManualState.json'
start=time.monotonic();last='';result={'transitions':[],'errors':[]}
def tick(dt):
 global last
 try:
  p=next(p for w in unreal.EditorLevelLibrary.get_pie_worlds(False) for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller())
  ui=json.loads(T.operations_ui_report(p));c=json.loads(T.commerce_report(p))
  state={k:ui.get(k) for k in ['screen','page','sort','sort_descending','document_tab','workspace_tab','selected_order','selected_booking','walking','property_access']}
  state.update(cash=c['cash'],cost=c['cost'],revenue=c['revenue'],pallets=len(c['pallets']))
  key=json.dumps(state,sort_keys=True)
  if key!=last:
   state['seconds']=round(time.monotonic()-start,2);result['transitions'].append(state);last=key;log.write_text(json.dumps(result,indent=2))
  if time.monotonic()-start>600:unreal.unregister_slate_post_tick_callback(handle)
 except Exception:result['errors'].append(traceback.format_exc());log.write_text(json.dumps(result,indent=2));unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
