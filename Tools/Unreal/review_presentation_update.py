"""Unsaved four-peer presentation review; uses actual local UI entry points."""
import unreal,time,json,traceback
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve();out=root/'Saved/Screenshots/Reviews';out.mkdir(parents=True,exist_ok=True)
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
T=unreal.FoundrylineNetworkTestTools;start=time.monotonic();stage=0;last=0;fixture=None;camera=None;before=[]
q={'checks':{},'errors':[],'screenshots':[]}
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/Foundryline/Maps/L_CutPartsStarter')
def players():return [p for w in unreal.EditorLevelLibrary.get_pie_worlds(False) for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller()]
def host():return next(p for p in players() if p.has_authority())
def rigs():return [next(a for a in unreal.GameplayStatics.get_all_actors_of_class(p.get_world(),unreal.FoundrylineCarrierSceneActor) if any(c.get_name()=='FlatbedTrailer' for c in a.get_components_by_class(unreal.SceneComponent))) for p in players()]
def meshes(a):return [c for c in a.get_components_by_class(unreal.StaticMeshComponent) if c.get_editor_property('static_mesh')]
def shot(name):
    path=out/(name+'.png');unreal.SystemLibrary.execute_console_command(host(),'Shot filename="'+str(path)+'"');q['screenshots'].append(str(path))
def finish():
    q['screenshots']=[str(max(out.glob(Path(name).stem+'*.png'),key=lambda p:p.stat().st_mtime,default=Path(name))) for name in q['screenshots']]
    q['completed']=True;q['elapsed_seconds']=round(time.monotonic()-start,2)
    (root/'Saved/Logs/PresentationRuntimeQA.json').write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle)
    unreal.SystemLibrary.execute_console_command(host() if players() else None,'QUIT_EDITOR')
def tick(dt):
    global stage,last,fixture,camera,before
    try:
        now=time.monotonic();elapsed=now-start
        if elapsed>180:raise RuntimeError('Presentation timeout stage '+str(stage))
        if stage==0 and elapsed>3:
            fixture=unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(unreal.FoundrylineCarrierSceneActor,unreal.Vector(1260,1700,20))
            fixture.configure_flatbed_review(False);stage=1;last=now
            unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
        elif stage==1 and now-last>15:
            if len(players())<4:return
            q['checks']['four_local_players']=len(players())==4
            q['checks']['flatbed_replicated_four_peers']=len(rigs())==4
            q['checks']['flatbed_animated_meshes_all_peers']=all(len([c for c in meshes(a) if any(n in c.get_editor_property('static_mesh').get_name() for n in ['Trailer_Wheel','Tractor_Wheel'])])==8 for a in rigs())
            before=[tuple(c.get_editor_property('relative_rotation').get_editor_property(k) for k in ['pitch','yaw','roll']) for c in meshes(rigs()[0]) if 'Wheel' in c.get_editor_property('static_mesh').get_name()]
            q['initial_people']=[json.loads(T.employee_report(p)) for p in players()]
            T.setup_operations_review(host(),1);stage=2;last=now
        elif stage==2 and now-last>3:
            u=json.loads(T.operations_ui_report(host()));q['checks']['first_person_roofs_and_input']=u['walking'] and u['roofs'] and not u['ignore_move'] and not u['cursor'];shot('FirstPerson_OfficePrompt')
            stage=21;last=now
        elif stage==21 and now-last>1:
            unreal.FoundrylineUIAudio.play(host(),'Hover');base=json.loads(unreal.FoundrylineUIAudio.playback_report(host())).get('Hover',0)
            for i in range(100):unreal.FoundrylineUIAudio.play(host(),'Hover')
            q['checks']['hover_burst_throttled']=json.loads(unreal.FoundrylineUIAudio.playback_report(host())).get('Hover',0)==base
            T.queue_commerce_action(host(),'Office');stage=3;last=now
        elif stage==3 and now-last>2:
            q['checks']['office_opens_locks_walk']=json.loads(T.operations_ui_report(host()))['screen']==1
            T.queue_commerce_action(host(),'Tablet');T.queue_commerce_action(host(),'Page8');T.queue_commerce_action(host(),'Carrier4');stage=4;last=now
        elif stage==4 and now-last>2:
            shot('Flatbed_ShippingChoice');stage=41;last=now
        elif stage==41 and now-last>1:T.queue_commerce_action(host(),'Close');stage=5;last=now
        elif stage==5 and now-last>2:
            q['ui_audio']=[json.loads(unreal.FoundrylineUIAudio.playback_report(p)) for p in players()]
            q['checks']['ui_sound_local_to_user']=json.loads(unreal.FoundrylineUIAudio.playback_report(host())).get('Open',0)>0 and all(json.loads(unreal.FoundrylineUIAudio.playback_report(p)).get('Open',0)==0 for p in players() if p!=host())
            q['checks']['softline_runtime_paths']=all('/Softline/Clean_Paper/' in v for r in q['ui_audio'] for k,v in r.items() if k.endswith('_asset'))
            a=rigs()[0];a.set_actor_location_and_rotation(a.get_actor_location()+unreal.Vector(200,300,0),unreal.Rotator(yaw=30),False,True);stage=6;last=now
        elif stage==6 and now-last>3:
            after=[tuple(c.get_editor_property('relative_rotation').get_editor_property(k) for k in ['pitch','yaw','roll']) for c in meshes(rigs()[0]) if 'Wheel' in c.get_editor_property('static_mesh').get_name()];q['checks']['wheels_rotate']=before!=after
            q['checks']['coupling_stays_connected']=True
            for a in rigs():
                frames={c.get_name():c for c in a.get_components_by_class(unreal.SceneComponent)}
                tractor=frames['FlatbedTractor'];trailer=frames['FlatbedTrailer'];h=unreal.MathLibrary.transform_location(tractor.get_world_transform(),unreal.Vector(42,0,0))
                q['checks']['coupling_stays_connected'] &= (h-trailer.get_world_location()).length()<1
            a=rigs()[0];a.set_actor_location_and_rotation(unreal.Vector(1260,1700,20),unreal.Rotator(),False,True);a.configure_flatbed_review(True)
            camera=T.create_review_camera(host());at=unreal.Vector(3700,-600,1550);target=unreal.Vector(1260,2400,150);camera.set_actor_location_and_rotation(at,unreal.MathLibrary.find_look_at_rotation(at,target),False,True);host().set_view_target_with_blend(camera,0);stage=7;last=now
        elif stage==7 and now-last>4:
            shot('Flatbed_InGame');q['people_visuals']=[]
            for p in players():
                for plant in unreal.GameplayStatics.get_all_actors_of_class(p.get_world(),unreal.FoundrylineFactoryPlant):
                    for c in plant.get_components_by_class(unreal.FoundrylineEmployeeVisualComponent):q['people_visuals'].append(json.loads(c.get_presentation_report()))
            q['checks']['smooth_employees_present']=bool(q['people_visuals']) and all(all('_SE' in n for n in r.get('outfit',[])) for r in q['people_visuals'])
            stage=8;last=now
        elif stage==8 and now-last>2:
            e=json.loads(T.employee_report(host()))['employees'][0];at=unreal.Vector(*e['position']);eye=at+unreal.Vector(230,270,150);camera.set_actor_location_and_rotation(eye,unreal.MathLibrary.find_look_at_rotation(eye,at+unreal.Vector(0,0,100)),False,True);stage=9;last=now
        elif stage==9 and now-last>2:shot('Smooth_Employee');stage=10;last=now
        elif stage==10 and now-last>1:
            T.setup_operations_review(host(),3);at=unreal.Vector(-1600,3400,3400);target=unreal.Vector(-500,1000,20);camera.set_actor_location_and_rotation(at,unreal.MathLibrary.find_look_at_rotation(at,target),False,True);host().set_view_target_with_blend(camera,0);stage=11;last=now
        elif stage==11 and now-last>3:shot('Floor_ForkliftOnly');stage=12;last=now
        elif stage==12 and now-last>2:finish()
    except Exception:q['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
