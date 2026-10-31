"""Create and validate a separate basic shop start. Never saves the existing shop map."""
import unreal,json,time,traceback
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve();dest='/Game/Foundryline/Maps/L_BasicShopStarter'
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not levels.load_level(dest if unreal.EditorAssetLibrary.does_asset_exist(dest) else '/Game/Foundryline/Maps/L_RoadOnlyStarter'):raise RuntimeError('Could not load environment')
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property('default_game_mode',unreal.load_class(None,'/Script/Foundryline.FoundrylineGameMode'))
for actor in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.PlayerStart):actor.set_actor_location(unreal.Vector(0,0,100),False,True)
if not unreal.EditorLoadingAndSavingUtils.save_map(world,dest):raise RuntimeError('Could not save separate basic shop map')
if not levels.load_level(dest):raise RuntimeError('Could not reopen basic shop map')
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
start=time.monotonic();stage=0
q={'map':dest,'saved':True,'checks':{},'errors':[]}
def tick(dt):
 global stage
 try:
  if stage==0 and time.monotonic()-start>2:levels.editor_request_begin_play();stage=1
  elif stage==1 and time.monotonic()-start>13:
   worlds=list(unreal.EditorLevelLibrary.get_pie_worlds(False))
   if not worlds:return
   w=worlds[0];q['world']=w.get_path_name();actors=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.Actor)
   sites=[a for a in actors if a.get_class().get_name()=='FoundrylineBuildSite'];q['checks']['site_ready']=len(sites)==1
   # Generated construction actors carry their definition in their label/name.
   kinds=['FoundrylineMaterialHandlingCell','FoundrylinePlasmaTable','FoundrylinePressBrakeDisplay','FoundrylineFreightRig']
   q['checks']['no_factory_or_equipment']=not any(a.get_class().get_name() in kinds for a in actors)
   player=unreal.GameplayStatics.get_player_controller(w,0)
   report=json.loads(unreal.FoundrylineNetworkTestTools.pavement_report(player));q['report']=report
   q['checks']['one_small_shipping_bay']=len(report.get('loading_bays',[]))==1 and report['loading_bays'][0]['tier']==1
   q['building_count']=sum(any(c.component_has_tag('FoundrylineRoof') for c in a.get_components_by_class(unreal.SceneComponent)) for a in actors)
   q['checks']['shop_and_office_only']=q['building_count']==2
   q['road_sections']=sum(a.actor_has_tag('FoundrylinePropertyVisual') for a in actors)
   q['checks']['standard_site_surfaces']=q['road_sections']==12
   q['checks']['multiplayer_same_layout']=len(worlds)==4 and len({json.loads(unreal.FoundrylineNetworkTestTools.pavement_report(unreal.GameplayStatics.get_player_controller(v,0)))['digest'] for v in worlds})==1
   q['checks']['first_person_at_desk']=unreal.GameplayStatics.get_player_pawn(w,0).get_class().get_name()=='FoundrylineWalkCharacter'
   q['checks']['office_desk_present']=len(unreal.GameplayStatics.get_all_actors_of_class(w,unreal.FoundrylineOperationsTerminal))==1
   (root/'Saved/Logs/BasicShopStartQA.json').write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(player,'QUIT_EDITOR');stage=2
 except Exception:
  q['errors'].append(traceback.format_exc());(root/'Saved/Logs/BasicShopStartQA.json').write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(None,'QUIT_EDITOR')
handle=unreal.register_slate_post_tick_callback(tick)
