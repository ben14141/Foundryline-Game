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
    (root/'Saved/Logs/ExteriorKitQA.json').write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(host() if worlds() else None,'QUIT_EDITOR')
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

def tick(dt):
    global stage,last,pending_aim
    try:
        now=time.monotonic()
        if pending_aim and now-last_shot>.5:
            at,target=pending_aim;pending_aim=None;aim(at,target)
        if now-start>240:raise RuntimeError('Exterior kit review timed out at '+str(stage))
        if stage==0 and now-start>3:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1;last=now
        elif stage==1 and now-last>14:
            if len(worlds())<4:return
            for i,w in enumerate(worlds()):N.place_player_collision_review(pc(w),unreal.Vector(-2940+i*45,710,112),-90);N.queue_commerce_action(pc(w),'Office')
            stage=2;last=now
        elif stage==2 and now-last>2:
            for w in worlds():N.queue_commerce_action(pc(w),'PropertyView')
            stage=3;last=now
        elif stage==3 and now-last>4:
            check('initial_kit_matches_four_players',parity() and kit(host().get_world())['instances']>100);tool(host(),'Demolish',(3000,3000));stage=3.2;last=now
        elif stage==3.2 and now-last>2:
            shape(host(),'Rectangle',[(400,-200),(5600,3600)],curbs=False);stage=3.5;last=now
        elif stage==3.5 and now-last>2:
            q['before_preview']=report()['digest'];aim((4000,2800,1400),(300,1700,400));N.show_pavement_tool(host(),'AreaMenu');N.show_pavement_tool(host(),'BayPlace');tool(host(),'PaveMove',(400,1700));stage=4;last=now
        elif stage==4 and now-last>3:
            tool(host(),'PaveMove',(400,1700));stage=4.5;last=now
        elif stage==4.5 and now-last>2:
            check('hologram_includes_exterior_kit',kit(host().get_world(),True)['instances']>0);check('hologram_kit_has_no_collision_or_mutation',kit(host().get_world(),True)['colliding']==0 and report()['digest']==q['before_preview']);shot('Exterior_Kit_Hologram');stage=5;last=now
        elif stage==5 and now-last>2:
            tool(host(),'PaveDown',(400,1700));tool(host(),'PaveUp',(400,1700));tool(host(),'SelectMode',(0,0));tool(host(),'WheelClick',(250,100));
            for w in worlds():pc(w).toggle_roofs()
            aim((4200,-4000,2300),(-700,600,300));stage=6;last=now
        elif stage==6 and now-last>3:
            q['kit_by_world']=[kit(w) for w in worlds()];check('committed_details_match_all_players',agree() and parity());check('valid_materials_and_decorative_collision',all(not kit(w)['bad_materials'] and not kit(w)['colliding'] for w in worlds()));shot('Exterior_Kit_Whole_Shop');stage=7;last=now
        elif stage==7 and now-last>2:
            aim((2350,2900,880),(400,1700,330));stage=8;last=now
        elif stage==8 and now-last>3:
            shot('Exterior_Kit_Loading_Door');aim((-4800,-1800,1050),(-2700,700,180));stage=9;last=now
        elif stage==9 and now-last>3:
            shot('Exterior_Kit_Office');aim((-1300,-900,330),(-2100,0,190));stage=10;last=now
        elif stage==10 and now-last>3:
            shot('Exterior_Kit_Personnel_Entry');tool(peer(),'Building',(1000,4000),(1900,5300));stage=11;last=now
        elif stage==11 and now-last>3:
            check('client_build_creates_shared_kit',agree() and parity());tool(peer(1),'Shop2',(2000,4100),(2800,5200));stage=12;last=now
        elif stage==12 and now-last>3:
            check('tier2_join_keeps_kit_in_sync',agree() and parity() and all(pc(w).get_last_construction_error()==0 for w in worlds()));tool(peer(2),'OfficeArea',(100,4100),(900,5200));stage=13;last=now
        elif stage==13 and now-last>3:
            check('joined_office_kit_is_shared',agree() and parity());q['expanded_kit']=[kit(w) for w in worlds()];aim((6000,8500,3200),(1650,4500,600));stage=14;last=now
        elif stage==14 and now-last>3:
            shot('Exterior_Kit_Tier2_Expansion');peer().toggle_roofs();check('late_join_requested',N.request_late_join());stage=15;last=now
        elif stage==15 and now-last>9:
            check('fifth_player_receives_matching_details',len(worlds())==5 and agree() and parity());check('roof_preference_remains_local',host().are_roofs_shown() and not peer().are_roofs_shown());tool(peer(1),'Demolish',(2300,4500));stage=16;last=now
        elif stage==16 and now-last>3:
            check('removing_expansion_restores_shared_perimeter',agree() and parity());q['final_kit']=[kit(w) for w in worlds()];finish()
    except Exception:q['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
