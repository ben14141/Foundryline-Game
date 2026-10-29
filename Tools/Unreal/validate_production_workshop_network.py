import unreal,time,json,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
out=Path(unreal.Paths.project_saved_dir())/'Logs/ProductionWorkshopNetworkQA.json';start=time.monotonic();stage=0;q={'checks':{},'errors':[],'completed':False}
def all_of(cls):return [a for w in unreal.EditorLevelLibrary.get_pie_worlds(False) for a in unreal.GameplayStatics.get_all_actors_of_class(w,cls)]
def finish():
 q['completed']=True;out.write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world(),'QUIT_EDITOR')
def inspect(prefix,count):
 sites=all_of(unreal.FoundrylineBuildSite);cells=all_of(unreal.FoundrylineMaterialHandlingCell)
 q['checks'][prefix+'_property_count']=len(sites)==count
 q['checks'][prefix+'_same_construction']=len(set(a.get_property_digest() for a in sites))==1
 q['checks'][prefix+'_parts_areas']=len(all_of(unreal.FoundrylinePartsArea))==count
 q['checks'][prefix+'_raw_steel_areas']=len(all_of(unreal.FoundrylineRawSteelReceivingArea))==count
 q['checks'][prefix+'_rack_position']=len(cells)==count and all((c.get_rack_world_position(1)-unreal.Vector(650,1500,190)).length()<.1 for c in cells)
 q['checks'][prefix+'_stock_quantities']=len(cells)==count and all(sum(s.count for s in c.stock)==20 for c in cells)
def tick(dt):
 global stage
 try:
  t=time.monotonic()-start
  if t>80:raise RuntimeError('Workshop network test timed out')
  if stage==0 and t>3:stage=1;unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
  elif stage==1 and t>18:
   inspect('four_players',4);q['checks']['late_join_requested']=unreal.FoundrylineNetworkTestTools.request_late_join();stage=2
  elif stage==2 and t>31:inspect('five_players',5);finish();return
  out.write_text(json.dumps(q,indent=2))
 except Exception:q['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
