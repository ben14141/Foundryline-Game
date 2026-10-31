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
    (root/'Saved/Logs/RoadSplineQA.json').write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(host() if worlds() else None,'QUIT_EDITOR')
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

knots=[(-3500,3000),(-3300,4200),(-2000,5000),(0,5000),(1500,4500)]
def click(at):tool(host(),'PaveDown',at);tool(host(),'PaveUp',at)
def splines():return [a for a in report()['areas'] if a['spline']]
def tick(dt):
    global stage,last,index,pending_aim
    try:
        now=time.monotonic()
        if pending_aim and now-last_shot>.5:
            at,target=pending_aim;pending_aim=None;aim(at,target)
        if now-start>220:raise RuntimeError('Spline review timed out at '+str(stage))
        if stage==0 and now-start>3:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1;last=now
        elif stage==1 and now-last>14:
            if len(worlds())<4:return
            check('wheel_unavailable_before_office',all(not report(pc(w))['wheel_available'] for w in worlds()))
            for i,w in enumerate(worlds()):N.place_player_collision_review(pc(w),unreal.Vector(-2940+i*45,710,112),-90);N.queue_commerce_action(pc(w),'Office')
            stage=2;last=now
        elif stage==2 and now-last>2:
            for w in worlds():N.queue_commerce_action(pc(w),'PropertyView')
            stage=3;last=now
        elif stage==3 and now-last>4:
            shape(host(),'Rectangle',[(500,3800),(2500,4800)]);aim((8000,9500,9000),(-700,4200,0));N.show_pavement_tool(host(),'Spline');N.show_pavement_tool(host(),'StandardPreset');stage=4;last=now
        elif stage==4 and now-last>3:
            check('standard_preset_is_24ft_two_way',report()['road_lanes']==2 and abs(report()['road_width_ft']-24)<.01);q['before_draft']=report()['digest'];click(knots[0]);index=1;stage=5;last=now
        elif stage==5 and now-last>1.4:
            if index<len(knots):click(knots[index]);index+=1;last=now
            else:check('clicked_points_remain_uncommitted',report()['draft_points']==5 and report()['digest']==q['before_draft']);shot('Road_Spline_Hologram');N.show_pavement_tool(host(),'TurningCheck');stage=6;last=now
        elif stage==6 and now-last>2:
            check('spline_vehicle_sweep_is_available',bool(report()['clearance']));N.show_pavement_tool(host(),'TurningCheck');tool(host(),'PaveEnter',knots[-1]);stage=7;last=now
        elif stage==7 and now-last>3:
            check('five_point_spline_placed_and_replicated',len(splines())==1 and splines()[0]['spline_points']==5 and agree());check('spline_connects_to_paved_yard',len(splines())==1 and splines()[0]['entrance_ends']&2);check('spline_paint_valid',len(splines())==1 and splines()[0]['markings']==4 and splines()[0]['invalid_markings']==0);tool(host(),'Select',knots[2]);N.show_pavement_tool(host(),'EditMenu');stage=8;last=now
        elif stage==8 and now-last>2:
            check('all_five_points_have_edit_handles',len(report()['handles'])==5);shot('Road_Spline_Edit_Handles');stage=9;last=now
        elif stage==9 and now-last>2:
            q['before_move']=report()['digest'];tool(host(),'PaveDown',knots[2]);stage=10;last=now
        elif stage==10 and now-last>1:
            check('spline_point_drag_starts',report()['direct_drag']);tool(host(),'PaveMove',(-1800,4800));stage=11;last=now
        elif stage==11 and now-last>2:
            tool(host(),'PaveUp',(-1800,4800));stage=12;last=now
        elif stage==12 and now-last>3:
            check('point_edit_updates_shared_spline',report()['digest']!=q['before_move'] and len(splines())==1 and splines()[0]['spline_points']==5 and agree());check('edited_knot_matches_drag_target',any(abs(h['x']+1800)<1 and abs(h['y']-4800)<1 for h in report()['handles']));check('new_spline_point_has_collision',all(abs(N.pavement_height_at(pc(w),unreal.Vector(-1800,4800,0))-20)<.1 for w in worlds()));N.show_pavement_tool(host(),'InsertPoint');stage=13;last=now
        elif stage==13 and now-last>1:
            q['before_far_insert']=report()['digest'];click((1000,3000));stage=13.5;last=now
        elif stage==13.5 and now-last>2:
            check('far_click_does_not_insert_point',report()['digest']==q['before_far_insert'] and splines()[0]['spline_points']==5);click((-2600,4800));stage=14;last=now
        elif stage==14 and now-last>3:
            check('insert_point_updates_spline',len(splines())==1 and splines()[0]['spline_points']==6 and agree());N.show_pavement_tool(host(),'RemovePoint');stage=15;last=now
        elif stage==15 and now-last>1:
            q['before_far_remove']=report()['digest'];click((1000,3000));stage=15.5;last=now
        elif stage==15.5 and now-last>2:
            check('far_click_does_not_remove_point',report()['digest']==q['before_far_remove'] and splines()[0]['spline_points']==6);click((-2600,4800));stage=16;last=now
        elif stage==16 and now-last>3:
            check('remove_point_updates_spline',len(splines())==1 and splines()[0]['spline_points']==5 and agree());check('paint_still_valid_after_point_edits',len(splines())==1 and splines()[0]['invalid_markings']==0);N.show_pavement_tool(host(),'Spline');stage=17;last=now
        elif stage==17 and now-last>1:
            q['before_cancel']=report()['digest'];click(knots[0]);stage=18;last=now
        elif stage==18 and now-last>1:
            click(knots[1]);stage=19;last=now
        elif stage==19 and now-last>1:
            tool(host(),'PaveBack',knots[1]);stage=20;last=now
        elif stage==20 and now-last>1:
            check('backspace_removes_last_draft_point',report()['draft_points']==1);N.show_pavement_tool(host(),'WheelToggle');stage=21;last=now
        elif stage==21 and now-last>1:
            check('wheel_does_not_abandon_unfinished_spline',not report()['wheel_open'] and report()['draft_points']==1);tool(host(),'PaveCancel',knots[1]);stage=22;last=now
        elif stage==22 and now-last>2:
            check('cancel_discards_draft_without_world_change',report()['draft_points']==0 and report()['digest']==q['before_cancel'] and agree());N.show_pavement_tool(host(),'WheelTruck');stage=23;last=now
        elif stage==23 and now-last>2:
            check('truck_wheel_preset_uses_spline_and_30ft_two_way',report()['tool']=='PaveSpline' and report()['road_lanes']==2 and abs(report()['road_width_ft']-30)<.01 and abs(report()['draft_radius_ft']-30)<.01);wheel_button();stage=24;last=now
        elif stage==24 and now-last>1:
            wheel_sector(1,3);stage=25;last=now
        elif stage==25 and now-last>1:
            wheel_sector(5,6);stage=26;last=now
        elif stage==26 and now-last>1:
            check('road_markings_open_from_road_wheel',report()['wheel_page']==5);r=report();tool(host(),'WheelClick',(round(r['wheel_x']),round(r['wheel_y'])));stage=27;last=now
        elif stage==27 and now-last>1:
            check('wheel_back_returns_to_roads',report()['wheel_page']==2);shot('Road_Spline_Wheel');stage=28;last=now
        elif stage==28 and now-last>2:
            tool(host(),'PaveCancel',(0,0));N.show_pavement_tool(host(),'EditMenu');tool(host(),'Select',(-1800,4800));stage=29;last=now
        elif stage==29 and now-last>2:
            shot('Road_Spline_Final');stage=30;last=now
        elif stage==30 and now-last>2:
            check('late_join_requested',N.request_late_join());stage=31;last=now
        elif stage==31 and now-last>8:
            check('fifth_player_receives_spline_geometry_and_points',len(worlds())==5 and agree() and all(any(a['spline'] and a['spline_points']==5 for a in report(pc(w))['areas']) for w in worlds()));finish()
    except Exception:q['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
