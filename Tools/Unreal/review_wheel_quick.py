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
    (root/'Saved/Logs/WheelQuickQA.json').write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(host() if worlds() else None,'QUIT_EDITOR')
def schedule():
    global actions
    actions=[
      (lambda:option(peer(),(1000,1000),'Convert'),lambda:(check('legacy_yard_conversion_shared',len(report()['areas'])==1 and agree()),q.update(truck_converted=report()['truck_targets']))),
      (lambda:tool(peer(1),'Demolish',(4200,-1000)),lambda:check('legacy_drive_removed',agree())),
      (lambda:shape(peer(2),'Road',[(4200,-2400),(5200,-1000),(2600,1500)],914.4,2,True,True),lambda:check('curved_30ft_truck_drive_shared',any(a['lanes']==2 and abs(a['width_ft']-30)<.01 and a['vertices']>10 for a in report()['areas']) and agree())),
      (lambda:option(peer(),(4200,-1800),'RadiusMore'),lambda:check('entrance_radius_15ft_shared',any(abs(a['radius_ft']-15)<.01 and a['entrance_ends'] for a in report()['areas']) and agree())),
      (lambda:option(peer(1),(4200,-1800),'RadiusMore'),lambda:check('entrance_radius_20ft_shared',any(abs(a['radius_ft']-20)<.01 for a in report()['areas']) and agree())),
      (lambda:option(peer(2),(4200,-1800),'RadiusMore'),lambda:check('entrance_radius_25ft_shared',any(abs(a['radius_ft']-25)<.01 for a in report()['areas']) and agree())),
      (lambda:shape(peer(),'Rectangle',[(400,3600),(4000,5200)]),lambda:check('rectangular_expansion_merges',len(report()['areas'])==2 and agree())),
      (lambda:shape(peer(1),'Rectangle',[(4500,4000),(5600,5200)]),lambda:check('separate_custom_patch',len(report()['areas'])==3 and agree())),
      (lambda:shape(peer(2),'Rectangle',[(4000,4000),(4500,5200)]),lambda:check('compound_bridge_merges_shared',len(report()['areas'])==2 and agree())),
      (lambda:shape(peer(),'Erase',[(1500,4100),(2400,4700)]),lambda:check('paintable_island_cutout_shared',sum(a['holes'] for a in report()['areas'])==1 and agree())),
      (lambda:tool(peer(1),'PaveRound',(5600,5200),(5200,4800)),lambda:check('outer_corner_rounded_shared',any(a['lanes']==0 and a['vertices']>10 for a in report()['areas']) and agree())),
      (lambda:paint(peer(),0,[(800,4900),(800,3800)],548.64),lambda:check('custom_parking_targets_created',report()['parking_targets']>=q['parking_before']+4 and agree())),
      (lambda:paint(peer(1),6,[(800,2800),(2500,2800)],304.8),lambda:check('keep_clear_paint_shared',sum(a['markings'] for a in report()['areas'])>=2 and agree())),
      (lambda:paint(peer(2),5,[(2800,100),(5300,100)],365.76),lambda:check('loading_paint_has_destination',report()['truck_targets']>=q['truck_converted']+1 and agree())),
      (lambda:paint(peer(),4,[(3000,600),(3500,600)],10.16),lambda:check('direction_arrows_shared',agree())),
      (lambda:paint(peer(1),7,[(3750,-2200),(4650,-2200)],10.16),lambda:check('stop_bar_shared',agree())),
      (lambda:option(peer(2),(4200,-1800),'StripeRoad'),lambda:check('road_paint_follows_curve',any(a['lanes']==2 and a['markings']>=4 for a in report()['areas']) and agree())),
    ]
def wheel_button():
    r=report();tool(host(),'WheelClick',(round(r['wheel_button_x']),round(r['wheel_button_y'])))
def wheel_sector(index,count):
    r=report();angle=math.radians(-90+index*360/count);tool(host(),'WheelClick',(round(r['wheel_x']+math.cos(angle)*r['wheel_r']*.75),round(r['wheel_y']+math.sin(angle)*r['wheel_r']*.75)))
def tick(dt):
    global stage,last,pending_aim
    try:
        now=time.monotonic();elapsed=now-start
        if pending_aim and now-last_shot>.5:
            at,target=pending_aim;pending_aim=None;aim(at,target)
        if elapsed>180:raise RuntimeError('Quick wheel review timed out')
        if stage==0 and elapsed>3:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1;last=now
        elif stage==1 and now-last>14:
            if len(worlds())<4:return
            check('wheel_hidden_before_office_planning',all(not report(pc(w))['wheel_available'] for w in worlds()))
            for i,w in enumerate(worlds()):N.place_player_collision_review(pc(w),unreal.Vector(-2940+i*45,710,112),-90);N.queue_commerce_action(pc(w),'Office')
            stage=2;last=now
        elif stage==2 and now-last>2:
            for w in worlds():N.queue_commerce_action(pc(w),'PropertyView')
            stage=3;last=now
        elif stage==3 and now-last>4:
            shape(host(),'Rectangle',[(-3000,3000),(-1500,4200)]);N.show_pavement_tool(host(),'Road');aim((9500,9500,8000),(1500,1700,0));stage=22;last=now
        elif stage==22 and now-last>2:
            q['before_wheel']=report()['digest'];q['other_tools']=[report(peer(i))['tool'] for i in range(3)];wheel_button();stage=23;last=now
        elif stage==23 and now-last>2:
            check('wheel_button_opens_root',report()['wheel_open'] and report()['wheel_page']==0);shot('Planning_Wheel_Main');stage=24;last=now
        elif stage==24 and now-last>2:
            wheel_sector(0,3);stage=25;last=now
        elif stage==25 and now-last>2:
            check('pavement_sector_opens_tools',report()['wheel_page']==1);shot('Planning_Wheel_Pavement');stage=26;last=now
        elif stage==26 and now-last>2:
            wheel_sector(0,6);stage=27;last=now
        elif stage==27 and now-last>2:
            check('wheel_rectangle_selection_sets_active_tool',not report()['wheel_open'] and report()['tool']=='PaveRect' and report()['wheel_tool']=='PAVE RECTANGLE');wheel_button();stage=28;last=now
        elif stage==28 and now-last>1:
            wheel_sector(1,3);stage=29;last=now
        elif stage==29 and now-last>2:
            check('road_sector_opens_tools',report()['wheel_page']==2);shot('Planning_Wheel_Roads');stage=30;last=now
        elif stage==30 and now-last>2:
            wheel_sector(1,6);stage=31;last=now
        elif stage==31 and now-last>2:
            check('wheel_curved_road_selection',not report()['wheel_open'] and report()['tool']=='PaveSpline' and abs(report()['road_width_ft']-24)<.01);wheel_button();stage=32;last=now
        elif stage==32 and now-last>1:
            wheel_sector(2,3);stage=33;last=now
        elif stage==33 and now-last>2:
            check('logistics_sector_opens_tools',report()['wheel_page']==3);shot('Planning_Wheel_Logistics');stage=34;last=now
        elif stage==34 and now-last>2:
            wheel_sector(1,6);stage=35;last=now
        elif stage==35 and now-last>2:
            check('wheel_selects_route_drawing',report()['tool']=='NetDraw' and report()['logistics_visible']);wheel_button();stage=36;last=now
        elif stage==36 and now-last>1:
            wheel_sector(0,3);stage=37;last=now
        elif stage==37 and now-last>1:
            wheel_sector(2,6);stage=38;last=now
        elif stage==38 and now-last>2:
            check('paint_opens_markings_wheel',report()['wheel_page']==4);shot('Planning_Wheel_Markings');stage=39;last=now
        elif stage==39 and now-last>2:
            r=report();tool(host(),'WheelClick',(round(r['wheel_x']),round(r['wheel_y'])));stage=40;last=now
        elif stage==40 and now-last>2:
            check('wheel_center_returns_to_parent',report()['wheel_open'] and report()['wheel_page']==1);wheel_sector(2,6);stage=41;last=now
        elif stage==41 and now-last>1:
            wheel_sector(0,8);stage=42;last=now
        elif stage==42 and now-last>2:
            check('wheel_parking_tool_exits_logistics',report()['tool']=='PavePaint' and not report()['logistics_visible'] and report()['wheel_tool']=='PARKING');check('wheel_clicks_do_not_build_on_map',report()['digest']==q['before_wheel'] and agree());check('wheel_changes_only_local_players_tools',[report(peer(i))['tool'] for i in range(3)]==q['other_tools'] and all(not report(peer(i))['wheel_open'] for i in range(3)));wheel_button();stage=43;last=now
        elif stage==43 and now-last>1:
            tool(host(),'PaveCancel',(0,0));stage=44;last=now
        elif stage==44 and now-last>2:
            check('right_click_closes_wheel_without_changing_tool',not report()['wheel_open'] and report()['tool']=='PavePaint' and report()['digest']==q['before_wheel']);check('late_join_requested',N.request_late_join());stage=45;last=now
        elif stage==45 and now-last>8:
            check('fifth_player_has_geometry_and_paint',len(worlds())==5 and agree());finish()

    except Exception:q['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
