"""Fresh four-player office review, real queued commands; no grants or user save writes."""
import unreal,time,json,traceback,math
from pathlib import Path
R=Path(unreal.Paths.project_dir()).resolve();OUT=R/'Saved/Logs/OfficeAssignmentsNativeQA.json';SHOTS=R/'Saved/Screenshots/Reviews';SHOTS.mkdir(parents=True,exist_ok=True)
N=unreal.FoundrylineNetworkTestTools;L=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
L.load_level('/Game/Foundryline/Maps/L_FirstShopStarter');unreal.EditorPythonScripting.set_keep_python_script_alive(True)
q={'checks':{},'errors':[],'completed':False};start=time.monotonic();next_at=start+3;camera=None
def worlds():return list(unreal.EditorLevelLibrary.get_pie_worlds(False))
def player(w):return next(p for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller())
def peers():return [player(w) for w in worlds()]
def host():return next(player(w) for w in worlds() if unreal.GameplayStatics.get_game_mode(w))
def report(p=None):return json.loads(N.office_storey_report(p or host()))
def action(p,s):N.queue_commerce_action(p,s)
def factory(p,s):N.queue_factory_action(p,unreal.Name(s))
def tool(p,s,a=(0,0,0),b=None):return N.queue_construction_tool(p,unreal.Name(s),unreal.IntVector(*map(int,a)),unreal.IntVector(*map(int,b or a)),0,unreal.LinearColor(1,1,1,1))
def check(n,v):
 q['checks'][n]=bool(v)
 if not v:q['errors'].append(n)
def assign(p,room,person):action(p,'OfficeAssign_%s_%s_%d_%d'%(room['building'],room['room'],room['floor'],person))
def aim(at,offset=(300,-400,900)):
 global camera
 if camera is None:camera=N.create_review_camera(host())
 target=unreal.Vector(*at);eye=target+unreal.Vector(*offset);camera.set_actor_location_and_rotation(eye,unreal.MathLibrary.find_look_at_rotation(eye,target),False,True);host().set_view_target_with_blend(camera,0)
def shot(name):unreal.SystemLibrary.execute_console_command(host(),'Shot filename="'+str(SHOTS/('OfficeAssigned_'+name+'.png'))+'"')
def flow():
 L.editor_request_begin_play();yield 20
 check('four_players',len(worlds())==4);h=host();c=[p for p in peers() if p!=h];builder=c[0]
 check('engraving_culls_reverse_lettering',not unreal.load_asset('/Game/Foundryline/Office/Decor/Materials/M_OfficeEngraving').get_editor_property('two_sided'))
 for p in [h,builder]:N.setup_operations_review(p,1)
 yield 1
 if json.loads(N.commerce_report(h))['running']:factory(h,'Run')
 yield 1
 initial=report();q['initial']=initial;cash=initial['cash'];room=initial['rooms'][0]
 action(h,'Office');yield 1;action(h,'Page7');action(h,'Workspace3');yield 1
 assign(h,room,0);yield 1.5
 q['president']=[report(p) for p in peers()]
 check('president_assignment_on_four_peers',all(len(report(p)['assignments'])==1 for p in peers()))
 check('existing_computer_reused',report()['assignments'][0]['desk']==0 and report()['desk_count']==initial['desk_count'])
 check('free_president_furnishing',all(report(p)['cash']==cash for p in peers()))
 check('president_glass_name_doors_all_peers',all(any(d['title']=='PRESIDENT' and d['name'] for d in report(p)['office_doors']) for p in peers()))
 check('expanded_president_wall_decor_all_peers',all({12,13,14}.issubset({d['kind'] for d in report(p)['assignments'][0]['decor']}) for p in peers()))
 shot('AssignmentUI');yield 1
 action(h,'Close');yield 1;action(h,'Office');yield 1;action(h,'PropertyView');yield 1
 aim((-2800,900,110));yield 1;shot('President');yield 1
 # Release restores the ordinary door and keeps the original terminal.
 N.setup_operations_review(h,1);yield 1;action(h,'Office');yield 1;action(h,'OfficeRelease0');yield 1
 check('unassignment_restores_doors',all(not any(d['title'] for d in report(p)['office_doors']) for p in peers()))
 check('unassignment_keeps_workstation',report()['desk_count']==initial['desk_count'])
 action(builder,'Office');yield 1;action(builder,'PropertyView');yield 1
 tool(builder,'OfficeRoom',(-3200,800,0),(-2800,1400,0));yield 1
 tool(builder,'OfficeRoom',(-2800,800,0),(-2400,1400,0));yield 1
 tool(builder,'RoomDoor',(-3000,800,0));yield 1;tool(builder,'RoomDoor',(-2600,800,0));yield 1
 rooms=[r for r in report()['rooms'] if r['room']!='0'*32];check('two_private_rooms',len(rooms)==2 and all(r['doors'] for r in rooms))
 if len(rooms)!=2:raise RuntimeError('Room creation failed: '+str(report()))
 action(h,'HireEngineer');yield 1.5
 check('engineer_hire_replicated',all(len(report(p)['office_people'])==1 for p in peers()))
 person=report()['office_people'][0]['id'];assign(h,rooms[0],0);yield 1.5;assign(h,rooms[1],person);yield 1
 q['separate_offices']=[report(p) for p in peers()]
 check('independent_offices_all_peers',all(len(report(p)['assignments'])==2 for p in peers()))
 check('free_engineer_and_offices',all(report(p)['cash']==cash for p in peers()))
 check('named_engineer_door_all_peers',all(any(d['title']=='ENGINEER' and d['name']==report(p)['office_people'][0]['name'] for d in report(p)['office_doors']) for p in peers()))
 check('engineer_office_clothes',all(any(v.get('officeClothes') and v.get('officeTie') for v in report(p)['employee_visuals']) for p in peers()))
 check('engineer_seated_keyboard',all(any(v.get('officeClothes') and v.get('activity')=='Office computer' for v in report(p)['employee_visuals']) for p in peers()))
 check('expanded_engineer_wall_decor_all_peers',all(sum(d['kind']>=12 for a in report(p)['assignments'] if a['person']==person for d in a['decor'])>=2 for p in peers()))
 before=report()['assignments'];assign(h,rooms[0],person);yield 1
 check('occupied_office_rejected_atomically',report()['assignments']==before)
 action(h,'OfficePerson'+str(person));action(h,'Page7');action(h,'Workspace3');yield 1;shot('EngineerUI');yield 1
 # Glass on shared interior partition, placed by a client.
 tool(builder,'OfficeInteriorGlass',(-2800,1100,0));yield 1.5
 # A second panel replaces the wall carrying the new clock, beside the door.
 tool(builder,'OfficeInteriorGlass',(-2700,800,0));yield 1
 check('interior_glass_property_synced',len({report(p)['digest'] for p in peers()})==1)
 glass_probe=json.loads(N.probe_player_collision(c[1],unreal.Vector(-2865,1150,112),unreal.Vector(-2735,1150,112)));q['glass_collision']=glass_probe
 check('glass_blocks_actual_player_capsule',glass_probe.get('valid') and glass_probe.get('blocked') and not glass_probe.get('penetrating'))
 q['after_glass']=[report(p) for p in peers()]
 check('glass_clears_new_wall_decor_all_peers',all(any(d['kind']>=12 and d['hidden'] for a in report(p)['assignments'] if a['person']==person for d in a['decor']) for p in peers()))
 # Planner hides roof, furniture still visible; inspect actual models from above.
 action(h,'Close');yield 1;action(h,'Office');yield 1;action(h,'PropertyView');yield 1
 aim((-2800,1060,90),(300,-400,900));yield 1;shot('PrivateOffices');yield 1
 # First-person camera keeps roof; inspect dressed seated employee at real workstation.
 N.setup_operations_review(h,1);yield 1;action(h,'Close');yield 1
 eng=next(a for a in report()['assignments'] if a['person']==person);at=eng['position'];yaw=math.radians(eng['quarter']*90);seat=[at[0]-85*math.sin(yaw),at[1]+85*math.cos(yaw),at[2]+95]
 aim(seat,(150,-30,70));yield 1;shot('EngineerComputer');yield 1
 # E uses the actual visible open leaf, then the physical closed leaf.
 door_component=None
 for actor in unreal.GameplayStatics.get_all_actors_of_class(h.get_world(),unreal.Actor):
  for component in actor.get_components_by_class(unreal.FoundrylinePersonnelDoorComponent):
   if component.get_office_title()=='ENGINEER' and abs(component.get_world_location().y-802.55)<5:door_component=component;break
  if door_component:break
 if not door_component:raise RuntimeError('Engineer glass door missing')
 N.place_player_collision_review(h,unreal.Vector(-2550,650,112),90);yield 1
 target=unreal.MathLibrary.transform_location(door_component.get_world_transform(),unreal.Vector(45.72,0,140))
 h.set_view_target_with_blend(h.get_controlled_pawn(),0);yield .5
 h.set_control_rotation(unreal.MathLibrary.find_look_at_rotation(h.player_camera_manager.get_camera_location(),target));yield 1
 tool(h,'Key:E');yield 1
 check('named_glass_door_closes_with_E',not door_component.is_open())
 check('door_state_replicated',all(any(d['title']=='ENGINEER' and not d['open'] for d in report(p)['office_doors']) for p in peers()))
 aim((-2550,800,145),(0,-220,8));yield 1;shot('NamedGlassDoor');yield 1
 closed=json.loads(N.probe_player_collision(h,unreal.Vector(-2550,735,112),unreal.Vector(-2550,865,112)));q['closed_door_collision']=closed;check('closed_door_blocks_player',closed.get('blocked') and not closed.get('penetrating'))
 h.set_view_target_with_blend(h.get_controlled_pawn(),0);yield .5
 h.set_control_rotation(unreal.MathLibrary.find_look_at_rotation(h.player_camera_manager.get_camera_location(),unreal.Vector(-2550,800,140)));yield 1;tool(h,'Key:E');yield 1
 opened=json.loads(N.probe_player_collision(h,unreal.Vector(-2550,735,112),unreal.Vector(-2550,865,112)));q['open_door_collision']=opened;check('open_door_allows_player_through',not opened.get('blocked') and not opened.get('penetrating'))
 N.place_player_collision_review(h,unreal.Vector(-2450,1185,112),180);yield 1
 h.set_view_target_with_blend(h.get_controlled_pawn(),0);h.set_control_rotation(unreal.MathLibrary.find_look_at_rotation(h.player_camera_manager.get_camera_location(),unreal.Vector(-2600,1185,155)));yield 1
 q['employee_focus']=json.loads(N.operations_ui_report(h));check('seated_engineer_can_be_focused',q['employee_focus']['focused_employee']==person)
 tool(h,'Key:E');yield 1;q['employee_inspection']=json.loads(N.operations_ui_report(h));check('E_opens_engineer_office_info',q['employee_inspection']['screen']==4 and q['employee_inspection']['inspected_employee']==person)
 factory(h,'Run');yield .2
 typing=[]
 for i in range(5):
  typing.append(next(v['palm0LocalZ'] for v in report()['employee_visuals'] if v.get('officeClothes')));yield .13
 q['typing_samples']=typing;check('engineer_typing_actually_moves',max(typing)-min(typing)>.02);factory(h,'Run');yield .5
 q['final']=[report(p) for p in peers()];q['completed']=True;yield 1
FLOW=flow()
def finish():
 OUT.write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(host() if worlds() else None,'QUIT_EDITOR')
def tick(dt):
 global next_at
 if time.monotonic()<next_at:return
 try:
  if time.monotonic()-start>300:raise TimeoutError('Office assignment review timeout')
  next_at=time.monotonic()+next(FLOW);q['elapsed_seconds']=round(time.monotonic()-start,1);OUT.write_text(json.dumps(q,indent=2))
 except StopIteration:finish()
 except Exception:q['errors'].append(traceback.format_exc());q['failure']=[report(p) for p in peers()] if worlds() else [];finish()
handle=unreal.register_slate_post_tick_callback(tick)
