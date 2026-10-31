"""Unsaved four-player architecture review: odd sizes, partial joins, height steps and undo."""
import unreal,time,json,traceback
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve();out=root/'Saved/Screenshots/Reviews';out.mkdir(parents=True,exist_ok=True)
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
N=unreal.FoundrylineNetworkTestTools;start=time.monotonic();stage=0;last=0;camera=None
q={'checks':{},'errors':[],'screenshots':[]}
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/Foundryline/Maps/L_CutPartsStarter')
def worlds():return list(unreal.EditorLevelLibrary.get_pie_worlds(False))
def player(w):return next(p for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller())
def host():return next(player(w) for w in worlds() if unreal.GameplayStatics.get_game_mode(w))
def peers():return [player(w) for w in worlds() if player(w)!=host()]
def site(w):return next(iter(unreal.GameplayStatics.get_all_actors_of_class(w,unreal.FoundrylineBuildSite)))
def agree():return len({site(w).get_property_digest() for w in worlds()})==1
def queue(p,t,a,b):N.queue_construction_tool(p,t,unreal.IntVector(a[0],a[1],0),unreal.IntVector(b[0],b[1],0),0,unreal.LinearColor(1,1,1,1))
def desc(p):return N.describe_construction(host(),unreal.IntVector(p[0],p[1],0))
def shot(name):
    q['screenshots'].append(name);unreal.SystemLibrary.execute_console_command(host(),'Shot filename="'+str(out/(name+'.png'))+'"')
def aim(at,target):
    global camera
    if not camera:camera=N.create_review_camera(host())
    camera.set_actor_location_and_rotation(unreal.Vector(*at),unreal.MathLibrary.find_look_at_rotation(unreal.Vector(*at),unreal.Vector(*target)),False,True);host().set_view_target_with_blend(camera,0)
def finish():
    q['completed']=True;q['elapsed_seconds']=round(time.monotonic()-start,2)
    q['screenshots']=[str(max(out.glob(name+'*.png'),key=lambda p:p.stat().st_mtime,default=out/(name+'.png'))) for name in q['screenshots']]
    (root/'Saved/Logs/SignAndPlannerQA.json').write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(host() if worlds() else None,'QUIT_EDITOR')
def tick(dt):
    global stage,last
    try:
        now=time.monotonic();elapsed=now-start
        if elapsed>150:raise RuntimeError('Architecture review timeout at stage '+str(stage))
        if stage==0 and elapsed>3:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1;last=now
        elif stage==1 and now-last>14:
            if len(worlds())<4:return
            q['checks']['four_players']=len(worlds())==4
            for i,w in enumerate(worlds()):
                N.place_player_collision_review(player(w),unreal.Vector(-2940+i*45,710,112),-90)
                N.queue_commerce_action(player(w),'Office')
            stage=17;last=now
        elif stage==17 and now-last>2:
            q['authority_before']=[json.loads(N.operations_ui_report(p)) for p in unreal.GameplayStatics.get_all_actors_of_class(host().get_world(),unreal.PlayerController)]
            for w in worlds():N.queue_commerce_action(player(w),'PropertyView')
            stage=16;last=now
        elif stage==16 and now-last>4:
            q['planning_reports']=[json.loads(N.operations_ui_report(player(w))) for w in worlds()]
            q['authority_after']=[json.loads(N.operations_ui_report(p)) for p in unreal.GameplayStatics.get_all_actors_of_class(host().get_world(),unreal.PlayerController)]
            q['checks']['planning_view_available']=all(not r['walking'] for r in q['planning_reports'])
            for w in worlds():unreal.SystemLibrary.execute_console_command(player(w),'r.MotionBlurQuality 0');unreal.SystemLibrary.execute_console_command(player(w),'r.DepthOfFieldQuality 0')
            queue(peers()[0],'Sign',(700,3300),(700,3300));stage=20;last=now
        elif stage==20 and now-last>3:
            q['checks']['sign_placement_shared']=agree() and 'ManufacturingSign' in desc((700,3300))
            N.queue_construction_tool(peers()[0],'SignEdit',unreal.IntVector(700,3300,0),unreal.IntVector(15,0,0),0,unreal.LinearColor(.035,.045,.055,1));stage=21;last=now
        elif stage==21 and now-last>3:
            text_counts=[]
            for w in worlds():
                texts=[str(c.get_editor_property('text')) for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.Actor) if a.actor_has_tag('FoundrylinePropertyVisual') for c in a.get_components_by_class(unreal.TextRenderComponent)]
                text_counts.append(texts.count('IRON VALLEY WORKS'))
            q['sign_name_counts']=text_counts;q['checks']['sign_name_colors_rotation_shared']=agree() and all(n==2 for n in text_counts)
            queue(host(),'Select',(700,3300),(700,3300));aim((1000,2200,230),(700,3300,210));stage=22;last=now
        elif stage==22 and now-last>4:
            signs=[a for a in unreal.GameplayStatics.get_all_actors_of_class(host().get_world(),unreal.Actor) if a.actor_has_tag('FoundrylineManufacturingSign')]
            q['sign_locations']=[str(a.get_actor_location()) for a in signs]
            q['checks']['physical_sign_matches_placement']=len(signs)==1 and signs[0].get_actor_location().distance(unreal.Vector(500,3100,0))<1
            q['camera_location']=str(camera.get_actor_location());q['camera_rotation']=str(camera.get_actor_rotation())
            shot('Manufacturing_Sign_InGame');queue(host(),'SignName',(0,0),(0,0));stage=23;last=now
        elif stage==23 and now-last>3:
            q['checks']['name_editor_opens']=json.loads(N.operations_ui_report(host()))['sign_name_editor'];shot('Manufacturing_Sign_Name_Editor');stage=24;last=now
        elif stage==24 and now-last>2:
            queue(host(),'SignClose',(0,0),(0,0));queue(host(),'PlannerSelect',(-1300,800),(-1300,800));aim((-400,-600,2000),(-1200,1100,100));stage=25;last=now
        elif stage==25 and now-last>4:
            shot('Planner_Machine_Selection');q['before_machine_storage']=N.scoped_plant_digest(host());stage=26;last=now
        elif stage==26 and now-last>2:
            N.queue_construction_tool(peers()[0],'PlannerStore',unreal.IntVector(1,0,0),unreal.IntVector(0,0,0),1,unreal.LinearColor());stage=27;last=now
        elif stage==27 and now-last>3:
            q['store_ui']=json.loads(N.operations_ui_report(peers()[0]));q['checks']['successful_edit_releases_pending']=not q['store_ui']['planner_pending'];q['stored_digest']=N.scoped_plant_digest(host());q['checks']['client_stores_machine_with_shared_state']=q['stored_digest']!=q['before_machine_storage'] and len({N.scoped_plant_digest(player(w)) for w in worlds()})==1
            q['checks']['late_join_requested']=N.request_late_join();stage=28;last=now
        elif stage==28 and now-last>8:
            q['checks']['late_join_has_stored_machine']=len(worlds())==5 and len({N.scoped_plant_digest(player(w)) for w in worlds()})==1
            N.queue_construction_tool(peers()[0],'PlannerPlace',unreal.IntVector(1,0,0),unreal.IntVector(-1300,800,0),1,unreal.LinearColor());stage=29;last=now
        elif stage==29 and now-last>3:
            q['restore_ui']=json.loads(N.operations_ui_report(peers()[0]));q['restored_digest']=N.scoped_plant_digest(host());q['checks']['client_restores_machine_with_shared_state']=q['restored_digest']!=q['stored_digest'] and len({N.scoped_plant_digest(player(w)) for w in worlds()})==1
            stage=30;last=now
        elif stage==30 and now-last>2:finish()
    except Exception:
        q['errors'].append(traceback.format_exc())
        if isinstance(q.get('material_parents'),set):q['material_parents']=sorted(q['material_parents'])
        finish()
handle=unreal.register_slate_post_tick_callback(tick)
