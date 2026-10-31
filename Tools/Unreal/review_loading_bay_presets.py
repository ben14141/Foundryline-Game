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
    (root/'Saved/Logs/LoadingBayPresetQA.json').write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(host() if worlds() else None,'QUIT_EDITOR')
def click(p,at):tool(p,'PaveDown',at);tool(p,'PaveUp',at)
def bays(p=None):return report(p)['loading_bays']
def tick(dt):
    global stage,last,pending_aim
    try:
        now=time.monotonic()
        if pending_aim and now-last_shot>.5:
            at,target=pending_aim;pending_aim=None;aim(at,target)
        if now-start>240:raise RuntimeError('Loading bay review timed out at '+str(stage))
        if stage==0 and now-start>3:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1;last=now
        elif stage==1 and now-last>14:
            if len(worlds())<4:return
            for i,w in enumerate(worlds()):N.place_player_collision_review(pc(w),unreal.Vector(-2940+i*45,710,112),-90);N.queue_commerce_action(pc(w),'Office')
            stage=2;last=now
        elif stage==2 and now-last>2:
            for w in worlds():N.queue_commerce_action(pc(w),'PropertyView')
            stage=3;last=now
        elif stage==3 and now-last>4:
            aim((6200,5600,4100),(500,1600,100));N.show_pavement_tool(host(),'AreaMenu');N.show_pavement_tool(host(),'WheelLoadingBay');stage=4;last=now
        elif stage==4 and now-last>2:
            check('wheel_selects_complete_bay_not_custom_paint',report()['tool']=='LoadingBay');N.show_pavement_tool(host(),'Mark5');check('custom_loading_paint_is_unavailable',report()['tool']=='LoadingBay');q['before']=report()['digest'];q['targets_before']=report()['truck_targets'];tool(host(),'PaveMove',(400,1700));stage=5;last=now
        elif stage==5 and now-last>3:
            check('hologram_has_no_collision',report()['ghost_actors']>0 and report()['ghost_blocking']==0);check('hologram_does_not_place_records',report()['digest']==q['before'] and not bays());shot('Loading_Bay_Small_Hologram');stage=6;last=now
        elif stage==6 and now-last>2:
            click(host(),(400,1700));stage=7;last=now
        elif stage==7 and now-last>3:
            check('small_bay_shared_by_four_players',len(bays())==1 and all(len(bays(pc(w)))==1 for w in worlds()) and agree());check('starter_vehicle_fits',len(bays())==1 and bays()[0]['tier']==1 and bays()[0]['small_clear']);check('existing_truck_door_converted_without_duplicate_target',report()['truck_targets']==q['targets_before']);q['id']=bays()[0]['id'];tool(host(),'SelectMode',(0,0));stage=8;last=now
        elif stage==8 and now-last>1:
            click(host(),(1000,1700));stage=9;last=now
        elif stage==9 and now-last>2:
            shot('Loading_Bay_Small_Placed');q['before_upgrade']=report()['digest'];N.show_pavement_tool(host(),'BayUpgrade');stage=10;last=now
        elif stage==10 and now-last>3:
            check('upgrade_previews_before_commit',report()['tool']=='UpgradeBay' and report()['ghost_actors']>0 and report()['digest']==q['before_upgrade']);shot('Loading_Bay_Box_Upgrade');stage=11;last=now
        elif stage==11 and now-last>2:
            N.show_pavement_tool(host(),'BayConfirm');stage=12;last=now
        elif stage==12 and now-last>3:
            check('box_upgrade_keeps_identity_and_replication',len(bays())==1 and bays()[0]['id']==q['id'] and bays()[0]['tier']==2 and agree());tool(peer(),'Select',(1000,1700));stage=13;last=now
        elif stage==13 and now-last>2:
            N.show_pavement_tool(peer(),'BayUpgrade');stage=14;last=now
        elif stage==14 and now-last>2:
            check('client_upgrade_preview_is_local',report(peer())['tool']=='UpgradeBay' and report()['tool']=='Select' and bays()[0]['tier']==2);N.show_pavement_tool(peer(),'BayConfirm');stage=15;last=now
        elif stage==15 and now-last>3:
            q['client_upgrade_reply']=report(peer());check('client_can_upgrade_shared_bay_to_semi',len(bays())==1 and bays()[0]['id']==q['id'] and bays()[0]['tier']==3 and bays()[0]['layout_clear'] and agree());shot('Loading_Bay_Semi_Placed');tool(peer(1),'Select',(1000,1700));stage=16;last=now
        elif stage==16 and now-last>1:
            tool(peer(1),'MoveDoor',(1000,1700),(-1000,2200));stage=17;last=now
        elif stage==17 and now-last>3:
            check('moving_moves_whole_bay_and_retains_id',len(bays())==1 and bays()[0]['id']==q['id'] and bays()[0]['tier']==3 and bays()[0]['min_y']>=2200 and agree());check('moved_apron_has_physical_floor',all(abs(N.pavement_height_at(pc(w),unreal.Vector(-1000,3000,0))-19.98)<.15 for w in worlds()));q['before_block']=report()['digest'];N.show_pavement_tool(peer(2),'BayPlace');stage=18;last=now
        elif stage==18 and now-last>1:
            click(peer(2),(-1600,2200));stage=19;last=now
        elif stage==19 and now-last>3:
            check('overlapping_bay_rejected_atomically',report()['digest']==q['before_block'] and len(bays())==1 and agree());tool(peer(2),'SelectMode',(0,0));check('late_join_requested',N.request_late_join());stage=20;last=now
        elif stage==20 and now-last>9:
            check('fifth_player_receives_bay_tier_and_identity',len(worlds())==5 and agree() and all(len(bays(pc(w)))==1 and bays(pc(w))[0]['id']==q['id'] and bays(pc(w))[0]['tier']==3 for w in worlds()));tool(peer(1),'Demolish',(-1000,3000));stage=21;last=now
        elif stage==21 and now-last>3:
            check('deletion_removes_door_paint_apron_and_target_for_all',not bays() and agree() and report()['truck_targets']==q['targets_before']-1);check('deletion_removes_apron_collision',all(N.pavement_height_at(pc(w),unreal.Vector(-1000,3000,0))==-10000 for w in worlds()));finish()
    except Exception:q['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
