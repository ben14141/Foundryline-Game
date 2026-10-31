"""Four-player native office-floor acceptance. Unsaved PIE, no grants or user save writes.
Initial review positions are test fixtures; traversal is not teleported. Ascent/descent use real pawn input.
"""
import unreal,time,json,traceback,math
from pathlib import Path
R=Path(unreal.Paths.project_dir()).resolve();OUT=R/'Saved/Logs/OfficeStoreysNativeQA.json';SHOTS=R/'Saved/Screenshots/Reviews';SHOTS.mkdir(parents=True,exist_ok=True)
STRAIGHT='FoundrylineStraightReview' in unreal.SystemLibrary.get_command_line()
if STRAIGHT:OUT=R/'Saved/Logs/OfficeStoreysStraightNativeQA.json'
STAIR_TOOL='OfficeStairsStraight' if STRAIGHT else 'OfficeStairs'
N=unreal.FoundrylineNetworkTestTools;L=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
exec(compile((R/'Tools/Unreal/prepare_office_stair_materials.py').read_text(),'prepare_office_stair_materials.py','exec'))
for name in ['T_Catalog_OfficeStairs','T_Catalog_OfficeStairsStraight']:
 p=R/('SourceAssets/UI/BuildCatalogue/'+name+'.png')
 if p.exists():
  t=unreal.AssetImportTask();t.filename=str(p);t.destination_path='/Game/Foundryline/UI/BuildCatalogue';t.destination_name=p.stem;t.automated=True;t.replace_existing=True;t.save=True;unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t]);a=unreal.load_asset(t.destination_path+'/'+t.destination_name);a.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON);a.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI);a.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS);unreal.EditorAssetLibrary.save_loaded_asset(a)
L.load_level('/Game/Foundryline/Maps/L_FirstShopStarter');unreal.EditorPythonScripting.set_keep_python_script_alive(True)
q={'checks':{},'errors':[],'completed':False,'notes':['Fresh unsaved four-player PIE. No cash/stock grants, map saves or player save writes. Real movement input after initial test placement.']}
start=time.monotonic();next_at=start+3;started=False;walker=None;camera=None

def worlds():return list(unreal.EditorLevelLibrary.get_pie_worlds(False))
def player(w):return next(p for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller())
def peers():return [player(w) for w in worlds()]
def host():return next(player(w) for w in worlds() if unreal.GameplayStatics.get_game_mode(w))
def report(p=None):return json.loads(N.office_storey_report(p or host()))
def ui(p=None):return json.loads(N.operations_ui_report(p or host()))
def action(p,s):N.queue_commerce_action(p,s)
def factory(p,s):N.queue_factory_action(p,unreal.Name(s))
def tool(p,s,a=(0,0,0),b=None,channel=0):
 b=b or a;return N.queue_construction_tool(p,unreal.Name(s),unreal.IntVector(*map(int,a)),unreal.IntVector(*map(int,b)),channel,unreal.LinearColor(1,1,1,1))
def check(n,v):
 q['checks'][n]=bool(v)
 if not v:q['errors'].append(n)
def agree():return len({report(p).get('digest') for p in peers()})==1
def office(p=None):return report(p)['offices'][0]
def point(o,x,y,z=0):
 yaw=math.radians(o['quarter']*90);return [o['anchor'][0]+x*math.cos(yaw)-y*math.sin(yaw),o['anchor'][1]+x*math.sin(yaw)+y*math.cos(yaw),z]
def shot(name):unreal.SystemLibrary.execute_console_command(host(),'Shot filename="'+str(SHOTS/(('OfficeStraight_' if STRAIGHT else 'OfficeStoreys_')+name+'.png'))+'"')
def aim(at,offset=(1300,-1500,1400)):
 global camera
 if camera is None:camera=N.create_review_camera(host())
 target=unreal.Vector(*at);eye=target+unreal.Vector(*offset);camera.set_actor_location_and_rotation(eye,unreal.MathLibrary.find_look_at_rotation(eye,target),False,True);host().set_view_target_with_blend(camera,0)
def navigate(p,route,label):
 pawn=p.get_controlled_pawn();capsule=pawn.get_component_by_class(unreal.CapsuleComponent);half=capsule.get_scaled_capsule_half_height();index=1;began=time.monotonic();peak=-1;minimum=1e9;samples=[]
 while index<len(route):
  pos=pawn.get_actor_location();feet=unreal.Vector(pos.x,pos.y,pos.z-half);target=unreal.Vector(*route[index]);dx=target.x-feet.x;dy=target.y-feet.y;distance=math.hypot(dx,dy);peak=max(peak,feet.z);minimum=min(minimum,feet.z)
  if len(samples)<100:samples.append([round(feet.x,2),round(feet.y,2),round(feet.z,2),index])
  if distance<18 and abs(feet.z-target.z)<35:index+=1;continue
  if time.monotonic()-began>45:raise RuntimeError(label+' stuck at '+str(index)+' feet '+str(feet)+' target '+str(target))
  direction=unreal.Vector(dx/max(distance,.01),dy/max(distance,.01),0);pawn.add_movement_input(direction,min(1.,max(.18,distance/90)),False);p.set_control_rotation(unreal.Rotator(pitch=-8,yaw=math.degrees(math.atan2(dy,dx)),roll=0));yield .01
 yield .7
 server=json.loads(N.authoritative_player_collision_position(p));q[label]={'peak_feet_z':peak,'minimum_feet_z':minimum,'samples':samples,'server':server};check(label+'_authoritative_pawn',server['found']);check(label+'_arrived',abs(server['position'][2]-server['half_height']-route[-1][2])<8);check(label+'_no_floor_fall',minimum>10)

def flow():
 global walker
 L.editor_request_begin_play();yield 20
 if len(worlds())!=4:raise RuntimeError('Expected four peers, got '+str(len(worlds())))
 check('four_players',True);h=host();clients=[p for p in peers() if p!=h];builder=clients[0];walker=clients[1]
 for p in [h,builder]:N.setup_operations_review(p,1)
 yield 2
 for p in [h,builder]:action(p,'Office')
 yield 1
 for p in [h,builder]:action(p,'PropertyView')
 yield 2
 if json.loads(N.commerce_report(h))['running']:factory(h,'Run')
 yield 1
 o=office();q['original_office']=o
 if STRAIGHT:
  check('idle_player_clear_of_long_stair',N.place_player_collision_review(clients[2],unreal.Vector(*point(o,700,900,112)),90));yield 1
 check('fresh_single_floor',not o['upper']);stair=point(o,425,50);tool(builder,STAIR_TOOL,stair,channel=o['quarter'])
 yield 3
 q['stair_result']=[report(p) for p in peers()];check('client_built_upper_floor',all(office(p)['upper'] for p in peers()));
 if not office()['upper']:raise RuntimeError('Stair placement rejected: '+str(q['stair_result']))
 check('saved_stair_style_correct',all(office(p)['stair_style']==int(STRAIGHT) for p in peers()));check('upper_floor_record_synced',agree());check('four_peers_have_upper_visual',all(report(p)['upper_actors']==1 for p in peers()));o=office();q['built_office']=o
 tool(builder,'OfficeFloor',channel=1);tool(h,'OfficeFloor',channel=0)
 yield 1
 check('planner_floor_preferences_independent',report(builder)['view_level']==1 and report(h)['view_level']==0);check('ground_planner_hides_upper',report(h)['upper_visible']==0);check('upstairs_planner_shows_upper',report(builder)['upper_visible']==1);check('first_person_keeps_upper_visible',report(walker)['upper_visible']==1)
 aim(point(o,o['width']/2,o['depth']/2,150));shot('GroundPlanning');yield 1
 before=report(h)['cash'];factory(builder,'ShopItem7');yield 1
 at=point(o,200,350,340) if STRAIGHT else point(o,o['width']-250,o['depth']-250,340);p=unreal.Vector(*at);q['desk_preview']=json.loads(N.preview_factory_brush(builder,unreal.Name('Equipment'),p,p,False,False));check('upper_desk_valid',q['desk_preview']['valid']);factory(builder,'CommitBrush');yield 3
 q['desk_result']=[report(p) for p in peers()];q['desk_ui']=ui(builder);check('upper_desk_replicated',all(report(p)['upper_desks']==1 for p in peers()));check('desk_charge_once_shared',all(report(p)['cash']==before-50000 for p in peers()));factory(builder,'ToolDone');yield 1
 tool(builder,'OfficeRoom',point(o,0,600,320),point(o,400,1000,320));yield 2
 check('upper_room_synced',all(office(p)['upper_rooms']==1 for p in peers()));tool(builder,'RoomDoor',point(o,400,800,320));yield 2
 tool(builder,'RoomWindow',point(o,200,o['depth'],320));yield 2
 check('upper_manual_window_synced',all(office(p)['upper_windows']==1 for p in peers()));check('room_window_property_agreement',agree())
 tool(h,'OfficeFloor',channel=1);yield 1;shot('UpperPlanning');yield 1
 # A floor opening/landing must remain unavailable for desk placement.
 factory(h,'ShopItem7');yield 1
 hole=point(o,625,300,340);p=unreal.Vector(*hole);bad=json.loads(N.preview_factory_brush(h,unreal.Name('Equipment'),p,p,False,True));check('stairwell_desk_blocked',not bad['valid'] and not bad['sent']);factory(h,'ToolDone');yield 1
 # Keep this client in first person while other users plan on different floors.
 action(walker,'Close');yield 1
 route=o['route'];position=unreal.Vector(*route[0]);capsule=walker.get_controlled_pawn().get_component_by_class(unreal.CapsuleComponent);position.z+=capsule.get_scaled_capsule_half_height()+2
 check('initial_entry_fixture_placed',N.place_player_collision_review(walker,position,90));yield 1
 yield from navigate(walker,route,'walk_up')
 check('climbed_real_320cm',q['walk_up']['peak_feet_z']>325);before_digest=report(h)['digest'];tool(builder,STAIR_TOOL,point(o,450,50),channel=o['quarter']);yield 2
 check('occupied_stairs_edit_blocked',report(h)['digest']==before_digest);q['occupied_edit_status']=ui(builder).get('status');shot('UpstairsWalk')
 yield from navigate(walker,list(reversed(route)),'walk_down')
 check('returned_ground_floor',q['walk_down']['server']['position'][2]-q['walk_down']['server']['half_height']<28);check('final_shared_property_agreement',agree())
 # Render outside with both office levels and roof visible, then downstairs stair view.
 N.setup_operations_review(h,1);yield 2;action(h,'Close');yield 1
 eye=unreal.Vector(*point(o,385,25,170));target=unreal.Vector(*point(o,580,300,210));camera.set_actor_location_and_rotation(eye,unreal.MathLibrary.find_look_at_rotation(eye,target),False,True);h.set_view_target_with_blend(camera,0);yield 2;shot('InteriorStairs');yield 1
 aim(point(o,o['width']/2,o['depth']/2,320),(1800,-2000,1500));shot('Exterior');yield 1
 q['final']=[report(p) for p in peers()];q['completed']=True;yield 1

FLOW=flow()
def finish():
 OUT.write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(host() if worlds() else None,'QUIT_EDITOR')
def tick(dt):
 global next_at
 if time.monotonic()<next_at:return
 try:
  if time.monotonic()-start>360:raise TimeoutError('Office review exceeded360seconds')
  next_at=time.monotonic()+next(FLOW);q['elapsed_seconds']=round(time.monotonic()-start,1);OUT.write_text(json.dumps(q,indent=2))
 except StopIteration:finish()
 except Exception:q['errors'].append(traceback.format_exc());q['failure']=[report(p) for p in peers()] if worlds() else [];finish()
handle=unreal.register_slate_post_tick_callback(tick)
