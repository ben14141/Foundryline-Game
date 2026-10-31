"""Unsaved four-peer pavement acceptance, input exercise, physics, late join and rendered review."""
import unreal,time,json,traceback
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
    (root/'Saved/Logs/PavementUXQuickQA.json').write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(host() if worlds() else None,'QUIT_EDITOR')
def tick(dt):
    global stage,last,pending_aim
    try:
        now=time.monotonic()
        if pending_aim and now-last_shot>.5:
            at,target=pending_aim;pending_aim=None;aim(at,target)
        if now-start>160:raise RuntimeError('Quick pavement UX review timed out')
        if stage==0 and now-start>3:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1;last=now
        elif stage==1 and now-last>14:
            if len(worlds())<4:return
            for i,w in enumerate(worlds()):N.place_player_collision_review(pc(w),unreal.Vector(-2940+i*45,710,112),-90);N.queue_commerce_action(pc(w),'Office')
            stage=2;last=now
        elif stage==2 and now-last>2:
            for w in worlds():N.queue_commerce_action(pc(w),'PropertyView')
            stage=3;last=now
        elif stage==3 and now-last>4:
            shape(host(),'Rectangle',[(-3000,3000),(-1500,4200)]);stage=4;last=now
        elif stage==4 and now-last>3:
            shape(peer(),'Road',[(-3600,1800),(-3300,4500),(-800,4600)],731.52,2,False,True);stage=5;last=now
        elif stage==5 and now-last>3:
            check('default_road_has_four_paint_groups',any(a['lanes']==2 and a['markings']==4 for a in report()['areas']) and agree());tool(host(),'Select',(-2200,3600));N.show_pavement_tool(host(),'EditMenu');aim((-500,5200,2200),(-2200,3600,0));stage=6;last=now
        elif stage==6 and now-last>3:
            check('eight_area_handles',len(report()['handles'])==8);check('compact_edit_panel',0<report()['panel_height']<350);check('placed_area_surface_is_20cm',abs(N.pavement_height_at(host(),unreal.Vector(-2200,3600,0))-20)<.1);shot('Pavement_UX_Close_Edit');q['before']=report()['digest'];q['area_before']=sum(a['area_sqft'] for a in report()['areas']);stage=7;last=now
        elif stage==7 and now-last>2:
            tool(host(),'PaveDown',(-1500,3600));stage=8;last=now
        elif stage==8 and now-last>1:
            check('direct_handle_drag_starts',report()['direct_drag']);tool(host(),'PaveMove',(-1250,3600));stage=9;last=now
        elif stage==9 and now-last>2:
            shot('Pavement_UX_Close_Draft');stage=10;last=now
        elif stage==10 and now-last>2:
            tool(host(),'PaveUp',(-1250,3600));stage=11;last=now
        elif stage==11 and now-last>3:
            check('direct_edit_shared_with_four_players',report()['digest']!=q['before'] and agree());check('edge_moves_exactly_25cm_grid_increment',abs(sum(a['area_sqft'] for a in report()['areas'])-q['area_before']-300000/929.0304)<.1);N.show_pavement_tool(host(),'Road');stage=12;last=now
        elif stage==12 and now-last>2:
            check('road_panel_compact',not report()['advanced'] and 0<report()['panel_height']<450);shot('Pavement_UX_Close_Road');stage=13;last=now
        elif stage==13 and now-last>2:
            N.show_pavement_tool(host(),'Advanced');stage=14;last=now
        elif stage==14 and now-last>2:
            check('advanced_options_expand',report()['advanced'] and report()['panel_height']>300);shot('Pavement_UX_Close_Advanced');stage=15;last=now
        elif stage==15 and now-last>2:
            N.show_pavement_tool(host(),'Advanced');N.show_pavement_tool(host(),'PaintMenu');stage=16;last=now
        elif stage==16 and now-last>2:
            shot('Pavement_UX_Close_Paint');stage=17;last=now
        elif stage==17 and now-last>2:
            tool(host(),'Select',(-2200,3600));N.show_pavement_tool(host(),'EditMenu');stage=18;last=now
        elif stage==18 and now-last>2:
            shot('Pavement_UX_Close_Final');stage=19;last=now
        elif stage==19 and now-last>2:finish()
    except Exception:q['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
