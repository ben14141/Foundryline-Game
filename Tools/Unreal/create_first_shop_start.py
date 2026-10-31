"""Create a separate plasma-only starter and review its live four-player layout."""
import unreal,time,json,traceback
from pathlib import Path
r=Path(unreal.Paths.project_dir()).resolve();out=r/'Saved/Screenshots/Reviews';out.mkdir(parents=True,exist_ok=True)
dest='/Game/Foundryline/Maps/L_FirstShopStarter';levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
exists=unreal.EditorAssetLibrary.does_asset_exist(dest)
if not levels.load_level(dest if exists else '/Game/Foundryline/Maps/L_BasicShopStarter'):raise RuntimeError('Starter environment unavailable')
w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
w.get_world_settings().set_editor_property('default_game_mode',unreal.load_class(None,'/Script/Foundryline.FoundrylineCutPartsGameMode'))
if not exists and not unreal.EditorLoadingAndSavingUtils.save_map(w,dest):raise RuntimeError('Separate starter map save failed')
levels.load_level(dest)
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
N=unreal.FoundrylineNetworkTestTools;start=time.monotonic();last=0;stage=0;cameras={};q={'map':dest,'checks':{},'errors':[],'screenshots':[]}
def worlds():return list(unreal.EditorLevelLibrary.get_pie_worlds(False))
def pc(w):return next(p for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller())
def host():return next(pc(w) for w in worlds() if unreal.GameplayStatics.get_game_mode(w))
def check(name,value):
 q['checks'][name]=bool(value)
 if not value:q['errors'].append(name)
def aim(at,target):
 for w in worlds():
  p=pc(w);key=p.get_path_name()
  if key not in cameras:cameras[key]=N.create_review_camera(p)
  c=cameras[key];c.set_actor_location_and_rotation(unreal.Vector(*at),unreal.MathLibrary.find_look_at_rotation(unreal.Vector(*at),unreal.Vector(*target)),False,True);p.set_view_target_with_blend(c,0)
def shot(name):
 q['screenshots'].append(name);unreal.SystemLibrary.execute_console_command(host(),'Shot filename="'+str(out/(name+'.png'))+'"')
def finish():
 q['completed']=True;q['seconds']=round(time.monotonic()-start,2)
 (r/'Saved/Logs/FirstShopQA.json').write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(host() if worlds() else None,'QUIT_EDITOR')
def tick(dt):
 global stage,last
 try:
  now=time.monotonic()
  if now-start>160:raise RuntimeError('Starter review timeout at '+str(stage))
  if stage==0 and now-start>3:levels.editor_request_begin_play();stage=1;last=now
  elif stage==1 and now-last>16:
   if len(worlds())!=4:return
   q['peer_reports']=[];door_sets=[]
   for w in worlds():
    actors=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.Actor)
    types=[a.get_class().get_name() for a in actors]
    check(w.get_path_name()+'_one_plasma',types.count('FoundrylinePlasmaTable')==1)
    check(w.get_path_name()+'_no_advanced_equipment',not any(t in types for t in ['FoundrylinePressBrakeDisplay','FoundrylineWeldingStation','FoundrylineMaterialHandlingCell','FoundrylineFreightRig']))
    p=pc(w);pr=json.loads(N.pavement_report(p));ui=json.loads(N.operations_ui_report(p));q['peer_reports'].append(pr)
    check(w.get_path_name()+'_one_small_shipping_bay',len(pr['loading_bays'])==1 and pr['loading_bays'][0]['tier']==1 and pr['loading_bays'][0]['small_clear'])
    check(w.get_path_name()+'_starts_first_person_with_desk',ui['walking'] and len(ui['terminals'])==1)
    doors=[d for a in actors for d in a.get_components_by_class(unreal.FoundrylinePersonnelDoorComponent)]
    check(w.get_path_name()+'_personnel_doors_ready',len(doors)>=3 and all(d.static_mesh and d.is_open() for d in doors))
    door_sets.append(sorted((round(d.get_world_location().x),round(d.get_world_location().y)) for d in doors))
   check('four_players_share_construction',len({p['digest'] for p in q['peer_reports']})==1)
   check('door_positions_match_all_players',all(s==door_sets[0] for s in door_sets))
   q['door_positions']=door_sets[0]
   for w in worlds():N.queue_commerce_action(pc(w),'Office')
   stage=2;last=now
  elif stage==2 and now-last>3:
   check('all_players_can_use_office_computer',all(json.loads(N.operations_ui_report(pc(w)))['screen']==1 for w in worlds()))
   for w in worlds():N.queue_commerce_action(pc(w),'PropertyView')
   aim((-600,-1400,1600),(-1000,700,0));stage=3;last=now
  elif stage==3 and now-last>3:
   aim((-600,-1400,1600),(-1000,700,0));unreal.SystemLibrary.execute_console_command(host(),'ShowHUD')
   for a in unreal.GameplayStatics.get_all_actors_of_class(next(w for w in worlds() if unreal.GameplayStatics.get_game_mode(w)),unreal.Actor):
    for c in a.get_components_by_class(unreal.SceneComponent):
     if c.component_has_tag('FoundrylineRoof'):c.set_visibility(False,True)
   stage=4;last=now
  elif stage==4 and now-last>3:shot('FirstShop_Overview');stage=5;last=now
  elif stage==5 and now-last>2:aim((-2200,200,300),(-700,1300,130));stage=6;last=now
  elif stage==6 and now-last>3:shot('FirstShop_Interior');stage=7;last=now
  elif stage==7 and now-last>2:aim((2200,1100,800),(300,1200,180));stage=8;last=now
  elif stage==8 and now-last>3:shot('FirstShop_ShippingDoors');stage=9;last=now
  elif stage==9 and now-last>2:finish()
 except Exception:q['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
