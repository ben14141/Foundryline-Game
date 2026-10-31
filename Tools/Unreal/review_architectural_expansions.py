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
    (root/'Saved/Logs/ArchitecturalExpansionQA.json').write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(host() if worlds() else None,'QUIT_EDITOR')
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
            queue(peers()[0],'Building',(1000,4000),(2700,5300));stage=2;last=now
        elif stage==2 and now-last>3:
            q['checks']['odd_sized_shop_shared']=agree() and 'Building_1700_1300' in desc((1400,4500))
            if not q['checks']['odd_sized_shop_shared']:raise RuntimeError('Shop review placement failed: '+str([str(player(w).get_last_construction_error()) for w in worlds()])+' / '+desc((1400,4500)))
            queue(peers()[1],'Shop2',(2700,4400),(4400,5300));stage=3;last=now
        elif stage==3 and now-last>3:
            q['checks']['partial_tall_expansion_shared']=agree() and 'Shop2_1700_900' in desc((3500,4500))
            queue(peers()[2],'OfficeArea',(100,4000),(1000,5100));stage=4;last=now
        elif stage==4 and now-last>3:
            q['checks']['office_connected_and_shared']=agree() and 'Office_900_1300' in desc((500,4500))
            queue(peers()[0],'Personnel',(1800,4000),(1800,4000));stage=5;last=now
        elif stage==5 and now-last>3:
            q['checks']['accepted_without_pending']=all(player(w).get_last_construction_error()==0 and not player(w).is_construction_pending() for w in worlds())
            q['checks']['late_join_requested']=N.request_late_join();N.setup_operations_review(host(),3)
            for w in worlds():player(w).toggle_roofs()
            aim((6500,8000,3400),(2300,4600,650));stage=6;last=now
        elif stage==6 and now-last>8:
            q['checks']['fifth_player_receives_expansions']=len(worlds())==5 and agree()
            shot('Architecture_Connected_Exterior');stage=7;last=now
        elif stage==7 and now-last>2:
            aim((2150,3800,230),(2300,4300,240));stage=8;last=now
        elif stage==8 and now-last>3:
            shot('Architecture_Wall_Material');stage=9;last=now
        elif stage==9 and now-last>2:
            aim((1800,5100,210),(2800,4600,430));stage=10;last=now
        elif stage==10 and now-last>3:
            shot('Architecture_Interior_Expansion');stage=11;last=now
        elif stage==11 and now-last>2:
            for w in worlds():
                if not json.loads(N.operations_ui_report(player(w)))['walking']:player(w).toggle_roofs()
            aim((6500,8000,3600),(2300,4600,200));stage=12;last=now
        elif stage==12 and now-last>3:
            shot('Architecture_Open_Expansion');q['glass_dimensions']=[];q['material_parents']=set()
            for a in unreal.GameplayStatics.get_all_actors_of_class(host().get_world(),unreal.Actor):
                at=a.get_actor_location()
                if not a.actor_has_tag('FoundrylinePropertyVisual') or not(0<at.x<4500 and 3900<at.y<5400):continue
                for c in a.get_components_by_class(unreal.InstancedStaticMeshComponent):
                    material=c.get_material(0)
                    if not material:continue
                    parent=material.get_editor_property('parent') if isinstance(material,unreal.MaterialInstanceDynamic) else material
                    q['material_parents'].add(parent.get_name())
                    if 'Glass' in parent.get_name():
                        for i in range(c.get_instance_count()):
                            t=c.get_instance_transform(i,False);v=t.scale3d;q['glass_dimensions'].append([round(v.x*100,2),round(v.y*100,2),round(v.z*100,2)])
            q['material_parents']=sorted(q['material_parents']);q['checks']['detailed_finishes_used']=all(n in q['material_parents'] for n in ['M_WallCladding','M_InteriorFinish','M_ArchitecturalTrim','M_FoundationConcrete'])
            q['checks']['native_glass_not_stretched']=bool(q['glass_dimensions']) and all(abs(sorted(v)[0]-2)<.1 and abs(sorted(v)[1]-168)<.1 and abs(sorted(v)[2]-188)<.1 for v in q['glass_dimensions'])
            queue(peers()[0],'Demolish',(3500,4500),(3500,4500));stage=13;last=now
        elif stage==13 and now-last>3:
            q['checks']['removed_expansion_restores_perimeter']=agree() and desc((3500,4500))=='EMPTY' and 'Building_1700_1300' in desc((1400,4500))
            queue(peers()[0],'Building',(2700,4400),(4400,5300));stage=14;last=now
        elif stage==14 and now-last>3:
            q['checks']['same_height_replacement_connects']=agree() and 'Building_1700_900' in desc((3500,4500));shot('Architecture_SameHeight_Expansion');stage=15;last=now
        elif stage==15 and now-last>2:
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
        elif stage==22 and now-last>3:
            shot('Manufacturing_Sign_InGame');queue(host(),'PlannerSelect',(-1300,800),(-1300,800));aim((-400,-600,2000),(-1200,1100,100));stage=23;last=now
        elif stage==23 and now-last>3:
            shot('Planner_Machine_Selection');stage=24;last=now
        elif stage==24 and now-last>2:finish()
    except Exception:
        q['errors'].append(traceback.format_exc())
        if isinstance(q.get('material_parents'),set):q['material_parents']=sorted(q['material_parents'])
        finish()
handle=unreal.register_slate_post_tick_callback(tick)
