"""Exercise rendered floor grid, edge-snapped zone, lane brush and crossing stamp."""
import unreal,time,json,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
base=Path(unreal.Paths.project_saved_dir());out=base/'Screenshots/Reviews';start=time.monotonic();stage=15 if unreal.EditorLevelLibrary.get_pie_worlds(False) else 0;last=0;q={'checks':{},'errors':[]};expected=None

def pc():return next(p for w in unreal.EditorLevelLibrary.get_pie_worlds(False) for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller())
def plant():return unreal.GameplayStatics.get_all_actors_of_class(pc().get_world(),unreal.FoundrylineFactoryPlant)[0]
def act(a):unreal.FoundrylineNetworkTestTools.queue_factory_action(pc(),a)
def brush(tool,start,end,drag=False,commit=False):return json.loads(unreal.FoundrylineNetworkTestTools.preview_factory_brush(pc(),tool,start,end,drag,commit))
def aim(p):return unreal.FoundrylineNetworkTestTools.aim_factory_brush(pc(),p)
def shot(n):unreal.SystemLibrary.execute_console_command(pc().get_world(),'Shot SHOWUI filename="'+str(out/('FloorGrid_'+n+'.png'))+'"')

def camera():
 unreal.FoundrylineNetworkTestTools.setup_workshop_review(pc(),True);c=pc().get_view_target();p=unreal.Vector(2200,6500,7000);target=unreal.Vector(800,2600,100);c.set_actor_location(p,False,False);c.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(p,target),False)
def tick(dt):
 global stage,last,expected
 try:
  t=time.monotonic()-start
  if stage==0 and t>3:stage=1;last=t;unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
  elif stage==1 and t-last>14:
   camera();stage=15;last=t
  elif stage==15 and t-last>3:
   p=unreal.Vector(1855.6,956.96,20);expected=brush('Zone',p,p);q['checks']['nearby_zone_edge_snap_valid']=expected['valid'] and expected['snapped'];q['checks']['neighbor_edges_touch_exactly']=abs(expected['max'][1]-1406.16)<.01;aim(p);stage=2;last=t
  elif stage==2 and t-last>4:
   guides=unreal.GameplayStatics.get_all_actors_of_class(pc().get_world(),unreal.FoundrylineFloorGuide);q['checks']['local_surface_grid_exists']=bool(guides) and all(not g.get_editor_property('replicates') for g in guides)
   q['checks']['grid_and_footprint_at_floor_height']=bool(guides) and all(abs(d.get_world_location().z-20)<.01 for g in guides for d in g.get_components_by_class(unreal.DecalComponent))
   grids=[d for g in guides for d in g.get_components_by_class(unreal.DecalComponent) if d.get_name()=='FloorGrid']
   q['grid_scale']=[[c.r,c.g] for d in grids for c in [d.get_decal_material().get_vector_parameter_value('UVScale')]]
   q['checks']['grid_spacing_matches_one_foot']=bool(q['grid_scale']) and all(abs(c[0]-5140/243.84)<.001 and abs(c[1]-4940/243.84)<.001 for c in q['grid_scale'])
   footprints=[d for g in guides for d in g.get_components_by_class(unreal.DecalComponent) if d.get_name()=='PaintFootprint'];q['checks']['visible_preview_aligned_to_neighbor']=any(d.is_visible() and abs(d.get_world_location().x-1855.6)<.01 and abs(d.get_world_location().y-948.96)<.01 for d in footprints)
   stage=3;last=t;shot('Zone_Edge_Snap')
  elif stage==3 and t-last>3:
   p=unreal.Vector(1855.6,956.96,20);r=brush('Zone',p,p,False,True);q['checks']['snapped_zone_committed']=r['sent'];stage=4;last=t
  elif stage==4 and t-last>2:
   q['checks']['authoritative_zone_matches_preview']=plant().get_zone_count()==3;act('ToolDone');stage=5;last=t
  elif stage==5 and t-last>2:
   a=unreal.Vector(-450.37,1800.19,20);b=unreal.Vector(-145.57,1800.19,20);r=brush('Forklift',a,b,True);q['checks']['lane_preview_valid']=r['valid'];aim(b);stage=6;last=t
  elif stage==6 and t-last>4:stage=7;last=t;shot('Lane_Brush')
  elif stage==7 and t-last>3:
   p=unreal.Vector(-300,2800,20);r=brush('Crossing',p,p);q['checks']['crossing_preview_matches_real_stamp_size']=r['valid'] and abs(r['max'][0]-r['min'][0]-200)<.01 and abs(r['max'][1]-r['min'][1]-500)<.01;aim(p);stage=8;last=t
  elif stage==8 and t-last>4:stage=9;last=t;shot('Crossing_Stamp')
  elif stage==9 and t-last>3:stage=10;last=t;unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_end_play()
  elif stage==10 and t-last>3:stage=11;last=t;unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
  elif stage==11 and t-last>14:act('Toggle');act('Page2');act('Save');stage=12;last=t
  elif stage==12 and t-last>3:q['checks']['normal_two_bay_scene_restored']=plant().get_zone_count()==2;q['completed']=True;(base/'Logs/FloorGridVisualQA.json').write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);return
  if t>160:raise RuntimeError('Grid review timeout '+str(stage))
  (base/'Logs/FloorGridVisualQA.json').write_text(json.dumps({**q,'stage':stage},indent=2))
 except Exception:
  q['errors'].append(traceback.format_exc());(base/'Logs/FloorGridVisualQA.json').write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
