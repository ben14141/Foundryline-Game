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
    (root/'Saved/Logs/PavementUXQA.json').write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(host() if worlds() else None,'QUIT_EDITOR')
def schedule():
    global actions
    actions=[
      (lambda:option(peer(),(1000,1000),'Convert'),lambda:(check('legacy_yard_conversion_shared',len(report()['areas'])==1 and agree()),q.update(truck_converted=report()['truck_targets']))),
      (lambda:tool(peer(1),'Demolish',(4200,-1000)),lambda:check('legacy_drive_removed',agree())),
      (lambda:shape(peer(2),'Road',[(4200,-2400),(5200,-1000),(2600,1500)],914.4,2,True,True),lambda:check('curved_30ft_truck_drive_shared',any(a['lanes']==2 and abs(a['width_ft']-30)<.01 and a['vertices']>10 for a in report()['areas']) and agree())),
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
def tick(dt):
    global stage,last,index,pending_aim
    try:
        now=time.monotonic();elapsed=now-start
        if pending_aim and now-last_shot>.5:
            at,target=pending_aim;pending_aim=None;aim(at,target)
        if elapsed>300:raise RuntimeError('Pavement review timeout at stage '+str(stage))
        if stage==0 and elapsed>3:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1;last=now
        elif stage==1 and now-last>14:
            if len(worlds())<4:return
            check('four_players',len(worlds())==4)
            for i,w in enumerate(worlds()):
                N.place_player_collision_review(pc(w),unreal.Vector(-2940+i*45,710,112),-90);N.queue_commerce_action(pc(w),'Office')
                unreal.SystemLibrary.execute_console_command(pc(w),'r.MotionBlurQuality 0');unreal.SystemLibrary.execute_console_command(pc(w),'r.DepthOfFieldQuality 0')
            stage=2;last=now
        elif stage==2 and now-last>2:
            for w in worlds():N.queue_commerce_action(pc(w),'PropertyView')
            stage=3;last=now
        elif stage==3 and now-last>4:
            check('office_planning_access',all(not json.loads(N.operations_ui_report(pc(w)))['walking'] for w in worlds()));q['parking_before']=report()['parking_targets'];q['truck_before']=report()['truck_targets'];schedule();actions[0][0]();stage=4;last=now
        elif stage==4 and now-last>2.5:
            q.setdefault('action_reports',[]).append([report(pc(w)) for w in worlds()]);actions[index][1]();index+=1
            if index<len(actions):actions[index][0]();last=now
            else:
                q['before_rejection']=report()['digest'];shape(peer(),'Road',[(1000,5400),(3000,5400)],300,2,False,False);stage=5;last=now
        elif stage==5 and now-last>3:
            check('narrow_two_way_drive_rejected',report()['digest']==q['before_rejection'] and agree());stage=6;last=now
        elif stage==6 and now-last>8:
            heights={}
            for label,point in [('asphalt',(1200,4000)),('cutout',(1900,4400)),('curb',(1490,4417)),('entrance',(4200,-2390))]:heights[label]=[N.pavement_height_at(pc(w),unreal.Vector(*point,0)) for w in worlds()]
            q['physical_heights_cm']=heights;check('asphalt_collision_is_20cm',all(abs(h-20)<.1 for h in heights['asphalt']));check('cutout_has_no_pavement_collision',all(h<-9000 for h in heights['cutout']));check('beveled_curb_has_6in_reveal',all(34<h<35.5 for h in heights['curb']));check('street_join_has_no_blocking_curb',all(abs(h-20)<.2 for h in heights['entrance']))
            check('all_finish_materials_used',all(m in report()['materials'] for m in ['M_PavementAsphalt','M_CurbConcrete','M_RoadPaintWhite','M_RoadPaintYellow']));host().toggle_roofs();N.show_pavement_tool(host(),'AreaMenu');aim((9500,9500,8000),(1500,1700,0));stage=7;last=now
        elif stage==7 and now-last>4:
            shot('Pavement_Overview');aim((5700,-3500,700),(4000,-1100,20));tool(host(),'Select',(4200,-1800));N.show_pavement_tool(host(),'EditMenu');stage=8;last=now
        elif stage==8 and now-last>4:
            shot('Pavement_Curved_Drive');aim((3500,5600,650),(1800,4400,20));stage=9;last=now
        elif stage==9 and now-last>4:
            shot('Pavement_Paint_And_Curbs');aim((8000,9000,9500),(700,3300,0));N.show_pavement_tool(host(),'Rect');stage=10;last=now
        elif stage==10 and now-last>3:
            q['before_input']=report()['digest'];tool(host(),'PaveDown',(-3000,3000));stage=11;last=now
        elif stage==11 and now-last>1:
            tool(host(),'PaveMove',(-1500,4200));stage=12;last=now
        elif stage==12 and now-last>2:
            shot('Pavement_Drag_Hologram');tool(host(),'PaveUp',(-1500,4200));stage=13;last=now
        elif stage==13 and now-last>3:
            check('actual_pointer_drag_places_area',report()['digest']!=q['before_input'] and agree());q['input_report']=report();N.show_pavement_tool(host(),'Road');stage=14;last=now
        elif stage==14 and now-last>3:
            shot('Pavement_Road_Tool_UI');stage=15;last=now
        elif stage==15 and now-last>2:
            N.show_pavement_tool(host(),'OneWay');N.show_pavement_tool(host(),'Truck');N.show_pavement_tool(host(),'Curve');N.show_pavement_tool(host(),'Vehicle3');aim((7000,8500,9000),(-1500,3000,0));q['before_curve_input']=report()['digest'];stage=16;last=now
        elif stage==16 and now-last>3:
            tool(host(),'PaveDown',(-3700,1800));stage=17;last=now
        elif stage==17 and now-last>1:
            tool(host(),'PaveMove',(-800,4600));stage=18;last=now
        elif stage==18 and now-last>2:
            tool(host(),'PaveUp',(-800,4600));stage=19;last=now
        elif stage==19 and now-last>2:
            check('curved_input_waits_for_bend_handle',report()['await_bend'] and report()['digest']==q['before_curve_input']);tool(host(),'PaveMove',(-3400,4500));stage=20;last=now
        elif stage==20 and now-last>3:
            shot('Pavement_Curve_Hologram_And_Clearance');tool(host(),'PaveDown',(-3400,4500));tool(host(),'PaveUp',(-3400,4500));stage=21;last=now
        elif stage==21 and now-last>3:
            check('actual_curve_handle_click_commits',not report()['await_bend'] and report()['digest']!=q['before_curve_input'] and agree());stage=22;last=now
        elif stage==22 and now-last>2:
            N.show_pavement_tool(host(),'EditMenu');aim((-500,5200,2200),(-2200,3600,0));stage=23;last=now
        elif stage==23 and now-last>3:
            tool(host(),'PaveDown',(-2250,3600));tool(host(),'PaveUp',(-2250,3600));stage=24;last=now
        elif stage==24 and now-last>2:
            r=report();check('edit_tab_exposes_eight_direct_area_handles',r['palette']==2 and len(r['handles'])==8);check('area_inspector_compact_by_default',not r['advanced'] and 0<r['panel_height']<350);q['before_edge']=r['digest'];shot('Pavement_UX_Direct_Edit');stage=25;last=now
        elif stage==25 and now-last>2:
            tool(host(),'PaveDown',(-1500,3600));stage=26;last=now
        elif stage==26 and now-last>1:
            check('edge_handle_press_begins_direct_drag',report()['direct_drag']);tool(host(),'PaveMove',(-1200,3600));stage=27;last=now
        elif stage==27 and now-last>2:
            tool(host(),'PaveUp',(-1200,3600));stage=28;last=now
        elif stage==28 and now-last>3:
            check('direct_edge_drag_changes_shared_geometry',not report()['direct_drag'] and report()['digest']!=q['before_edge'] and agree());paint(host(),2,[(-2800,3200),(-1600,3200)],10.16);stage=29;last=now
        elif stage==29 and now-last>2:
            tool(host(),'Select',(-2200,3800));N.show_pavement_tool(host(),'EditMenu');stage=30;last=now
        elif stage==30 and now-last>2:
            tool(host(),'PaveDown',(-2200,3200));tool(host(),'PaveUp',(-2200,3200));stage=31;last=now
        elif stage==31 and now-last>2:
            check('click_paint_selects_only_paint',report()['selected_paint']);q['before_paint_drag']=report()['digest'];q['area_before_paint']=sum(a['area_sqft'] for a in report()['areas']);tool(host(),'PaveDown',(-2200,3200));stage=32;last=now
        elif stage==32 and now-last>1:
            check('selected_paint_drag_starts',report()['direct_drag']);tool(host(),'PaveMove',(-2200,3450));stage=33;last=now
        elif stage==33 and now-last>2:
            tool(host(),'PaveUp',(-2200,3450));stage=34;last=now
        elif stage==34 and now-last>3:
            check('direct_paint_move_shared_without_resizing_asphalt',report()['digest']!=q['before_paint_drag'] and abs(sum(a['area_sqft'] for a in report()['areas'])-q['area_before_paint'])<.1 and agree());q['marks_before_delete']=sum(a['markings'] for a in report()['areas']);N.show_pavement_tool(host(),'DeleteSelection');stage=35;last=now
        elif stage==35 and now-last>3:
            check('delete_selected_paint_preserves_asphalt',sum(a['markings'] for a in report()['areas'])==q['marks_before_delete']-1 and abs(sum(a['area_sqft'] for a in report()['areas'])-q['area_before_paint'])<.1 and agree());N.show_pavement_tool(host(),'PaintMenu');stage=36;last=now
        elif stage==36 and now-last>2:
            shot('Pavement_UX_Paint_Menu');stage=37;last=now
        elif stage==37 and now-last>2:
            N.show_pavement_tool(host(),'Road');stage=38;last=now
        elif stage==38 and now-last>2:
            check('road_panel_compact_without_advanced',not report()['advanced'] and 0<report()['panel_height']<450);shot('Pavement_UX_Road_Menu');stage=39;last=now
        elif stage==39 and now-last>2:
            N.show_pavement_tool(host(),'Advanced');stage=40;last=now
        elif stage==40 and now-last>2:
            check('advanced_road_options_expand_only_when_requested',report()['advanced'] and report()['panel_height']>300);shot('Pavement_UX_Advanced');stage=41;last=now
        elif stage==41 and now-last>2:
            check('late_join_requested',N.request_late_join());stage=42;last=now
        elif stage==42 and now-last>8:
            check('fifth_player_has_geometry_and_paint',len(worlds())==5 and agree());finish()

    except Exception:q['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
