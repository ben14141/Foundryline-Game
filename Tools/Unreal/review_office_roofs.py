"""Unsaved four-peer pavement acceptance, input exercise, physics, late join and rendered review."""
import unreal,time,json,traceback,math
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve();out=root/'Saved/Screenshots/Reviews';out.mkdir(parents=True,exist_ok=True)
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
N=unreal.FoundrylineNetworkTestTools;start=time.monotonic();stage=0;last=0;cameras={};actions=[];index=0
q={'checks':{},'errors':[],'screenshots':[]};last_shot=-100;pending_aim=None
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/Foundryline/Maps/L_CutPartsStarter')
def worlds():return list(unreal.EditorLevelLibrary.get_pie_worlds(False))
def pc(w):return next(p for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller())
def host():return next(pc(w) for w in worlds() if unreal.GameplayStatics.get_game_mode(w))
def peer(i=0):return [pc(w) for w in worlds() if pc(w)!=host()][i]
def report(p=None):return json.loads(N.pavement_report(p or host()))
def agree():return len({report(pc(w))['digest'] for w in worlds()})==1
def vectors(points):return [unreal.Vector(x,y,0) for x,y in points]
def shape(p,mode,points,width=0,lanes=0,truck=False,curbs=True):return N.queue_pavement(p,mode,vectors(points),width,lanes,truck,curbs)
def paint(p,kind,points,width):return N.queue_pavement_paint(p,kind,vectors(points),width)
def option(p,at,action):return N.queue_pavement_option(p,unreal.IntVector(*at,0),action)
def tool(p,name,a,b=None):
    b=b or a;return N.queue_construction_tool(p,name,unreal.IntVector(*a,0),unreal.IntVector(*b,0),0,unreal.LinearColor(1,1,1,1))
def check(name,value):
    q['checks'][name]=bool(value)
    if not value:q['errors'].append(name+' failed: '+json.dumps(report()))
def aim(at,target):
    global pending_aim
    if time.monotonic()-last_shot<.5:
        pending_aim=(at,target);return
    for w in worlds():
        player=pc(w);key=player.get_path_name()
        if key not in cameras:cameras[key]=N.create_review_camera(player)
        camera=cameras[key]
        camera.set_actor_location_and_rotation(unreal.Vector(*at),unreal.MathLibrary.find_look_at_rotation(unreal.Vector(*at),unreal.Vector(*target)),False,True);player.set_view_target_with_blend(camera,0)

def shot(name):
    global last_shot
    last_shot=time.monotonic()
    q['screenshots'].append(name);unreal.SystemLibrary.execute_console_command(host(),'Shot filename="'+str(out/(name+'.png'))+'"')
def finish():
    q['completed']=True;q['elapsed_seconds']=round(time.monotonic()-start,2);q['final_reports']=[report(pc(w)) for w in worlds()] if worlds() else []
    q['screenshots']=[str(max(out.glob(name+'*.png'),key=lambda p:p.stat().st_mtime,default=out/(name+'.png'))) for name in q['screenshots']]
    (root/'Saved/Logs/OfficeRoofQA.json').write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(host() if worlds() else None,'QUIT_EDITOR')
def kit(w,preview=False):
    result={'components':0,'instances':0,'bad_materials':0,'colliding':0,'finishes':{}}
    for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.Actor):
        if bool(a.actor_has_tag('FoundrylinePlacementPreview'))!=preview:continue
        for c in a.get_components_by_class(unreal.InstancedStaticMeshComponent):
            if not c.component_has_tag('FoundrylineExteriorKit'):continue
            result['components']+=1;result['instances']+=c.get_instance_count()
            if not c.get_material(0):result['bad_materials']+=1
            if c.get_collision_enabled()!=unreal.CollisionEnabled.NO_COLLISION:result['colliding']+=1
            for tag in c.get_editor_property('component_tags'):
                key=str(tag)
                if key!='FoundrylineExteriorKit':result['finishes'][key]=result['finishes'].get(key,0)+c.get_instance_count()
    return result

def parity():return len({json.dumps(kit(w),sort_keys=True) for w in worlds()})==1


def office(w):
    return next((a for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.Actor) if not a.actor_has_tag('FoundrylinePlacementPreview') and abs(a.get_actor_location().x-1000)<1 and abs(a.get_actor_location().y-4000)<1 and len(a.get_components_by_class(unreal.RectLightComponent))>0),None)
def stats(w):
    a=office(w)
    if not a:return {}
    lights=[];fixtures=[];skylights=[]
    for c in a.get_components_by_class(unreal.SceneComponent):
        if c.component_has_tag('FoundrylineRecessedOfficeLight') and isinstance(c,unreal.RectLightComponent):
            p=c.get_editor_property('relative_location');lights.append([round(p.x,2),round(p.y,2),round(p.z,2),c.get_editor_property('intensity'),c.get_editor_property('temperature'),c.get_editor_property('source_width'),c.get_editor_property('source_height')])
        if isinstance(c,unreal.InstancedStaticMeshComponent):
            target=fixtures if c.component_has_tag('FoundrylineCeilingFixture') else skylights if c.component_has_tag('FoundrylineSkylight') else None
            if target is not None:
                for i in range(c.get_instance_count()):
                    t=c.get_instance_transform(i,False);p=t.translation;s=t.scale3d
                    if target is fixtures and abs(s.z*100-3)>.01:continue
                    target.append([round(v,2) for v in (p.x,p.y,p.z,s.x*100,s.y*100,s.z*100)])
    return {'lights':sorted(lights),'fixtures':sorted(fixtures),'skylights':sorted(skylights)}
def parity_office():return len({json.dumps(stats(w),sort_keys=True) for w in worlds()})==1

def tick(dt):
    global stage,last,pending_aim
    try:
        now=time.monotonic()
        if pending_aim and now-last_shot>.5:
            at,target=pending_aim;pending_aim=None;aim(at,target)
        if now-start>230:raise RuntimeError('Office review timed out at '+str(stage))
        if stage==0 and now-start>3:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1;last=now
        elif stage==1 and now-last>14:
            if len(worlds())<4:return
            check('first_person_starts_with_roof_shown',all(pc(w).are_roofs_shown() for w in worlds()))
            for i,w in enumerate(worlds()):N.place_player_collision_review(pc(w),unreal.Vector(-2940+i*45,710,112),-90);N.queue_commerce_action(pc(w),'Office')
            stage=2;last=now
        elif stage==2 and now-last>2:
            for w in worlds():N.queue_commerce_action(pc(w),'PropertyView')
            stage=3;last=now
        elif stage==3 and now-last>4:
            tool(peer(),'OfficeArea',(1000,4000),(2200,5000));stage=4;last=now
        elif stage==4 and now-last>3:
            check('client_office_build_replicates',agree() and parity_office() and bool(stats(host().get_world())))
            tool(peer(1),'OfficeRoom',(1100,4100),(1500,4500));stage=5;last=now
        elif stage==5 and now-last>3:
            tool(peer(2),'ChooseOfficeGlass',(0,0));aim((2800,3300,1500),(1500,4500,220));stage=6;last=now
        elif stage==6 and now-last>3:
            tool(peer(2),'PaveMove',(2100,4000));stage=6.5;last=now
        elif stage==6.5 and now-last>3:
            check('glazing_preview_has_no_collision',report(peer(2))['ghost_actors']>0 and report(peer(2))['ghost_blocking']==0);tool(peer(2),'PaveDown',(2100,4000));tool(peer(2),'PaveUp',(2100,4000));tool(peer(2),'SelectMode',(0,0));stage=7;last=now
        elif stage==7 and now-last>3:
            tool(peer(1),'OfficeGlass',(2200,4500));stage=8;last=now
        elif stage==8 and now-last>3:
            check('glass_and_room_changes_replicate',agree() and parity_office() and len(report().get('office_glass',[]))==2);q['office_by_world']=[stats(w) for w in worlds()];s=stats(host().get_world())
            check('actual_recessed_lights_present',len(s.get('lights',[]))>=4 and all(l[3]==5000 and l[4]==4000 and l[2]<320 for l in s.get('lights',[])))
            check('standard_two_by_four_foot_fixtures',len(s.get('fixtures',[]))>0 and all(abs(min(f[3:5])-60.96)<.02 and abs(max(f[3:5])-121.92)<.02 and abs(f[5]-3)<.01 for f in s['fixtures']))
            check('compact_office_skylights',len(s.get('skylights',[]))>0 and all(abs(min(f[3:5])-91.44)<.02 and abs(max(f[3:5])-121.92)<.02 for f in s['skylights']))
            tool(host(),'SelectMode',(0,0));tool(host(),'WheelClick',(250,100))
            for w in worlds():
                if not pc(w).are_roofs_shown():pc(w).toggle_roofs()
            aim((2800,3300,1500),(1500,4500,220));stage=9;last=now
        elif stage==9 and now-last>4:
            shot('Office_Roof_Skylights');aim((2080,4650,180),(1350,4350,245));stage=10;last=now
        elif stage==10 and now-last>4:
            shot('Office_Recessed_Ceiling');aim((3000,3400,500),(1900,4500,180));stage=11;last=now
        elif stage==11 and now-last>4:
            shot('Office_Glass_Wall');N.request_late_join();stage=12;last=now
        elif stage==12 and now-last>10:
            check('fifth_join_receives_office_geometry_and_lights',len(worlds())==5 and agree() and parity_office());
            if peer().are_roofs_shown():peer().toggle_roofs()
            check('roof_preference_is_local',host().are_roofs_shown() and not peer().are_roofs_shown());finish()
    except Exception:q['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
