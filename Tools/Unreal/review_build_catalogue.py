"""Unsaved four-peer pavement acceptance, input exercise, physics, late join and rendered review."""
import unreal,time,json,traceback,math
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve();out=root/'Saved/Screenshots/Reviews';out.mkdir(parents=True,exist_ok=True)
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
N=unreal.FoundrylineNetworkTestTools;start=time.monotonic();stage=0;last=0;cameras={};actions=[];index=0
q={'checks':{},'errors':[],'screenshots':[]};last_shot=-100;pending_aim=None
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/Foundryline/Maps/L_BasicShopStarter')
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
    (root/'Saved/Logs/BuildCatalogQA.json').write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(host() if worlds() else None,'QUIT_EDITOR')
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


def cat(p=None):return json.loads(report(p)['catalog'])
def click(p,key,item=False):
    hit=next(v for v in cat(p)['buttons'] if v.get('item' if item else 'action')==key)
    tool(p,'WheelClick',(round(hit['x']),round(hit['y'])))
def all_count(key,value):return all(report(pc(w)).get(key)==value for w in worlds())
def tick(dt):
    global stage,last
    try:
        now=time.monotonic()
        if now-start>250:raise RuntimeError('Catalogue review timed out at '+str(stage))
        if stage==0 and now-start>3:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1;last=now
        elif stage==1 and now-last>14:
            if len(worlds())<4:return
            for w in worlds():N.queue_commerce_action(pc(w),'Office')
            stage=2;last=now
        elif stage==2 and now-last>2:
            for w in worlds():N.queue_commerce_action(pc(w),'PropertyView')
            stage=3;last=now
        elif stage==3 and now-last>3:
            check('compact_toolbar_replaces_sidebar',all(any(b['action']=='CatalogOpen' for b in cat(pc(w))['buttons']) for w in worlds()))
            for w in worlds():tool(pc(w),'Key:Q',(0,0))
            stage=4;last=now
        elif stage==4 and now-last>2:
            check('keyboard_opens_local_catalogue',all(cat(pc(w))['open'] for w in worlds()));shot('Build_Catalogue_Expansion');q['expansion_menu']=cat()
            tool(peer(),'HUD:CatalogGroupDecorations',(0,0));stage=4.5;last=now
        elif stage==4.5 and now-last>2:
            click(peer(),'Company sign',True);stage=5;last=now
        elif stage==5 and now-last>2:
            check('actual_sign_card_selects_tool_and_closes_popup',report(peer())['tool']=='Sign' and not cat(peer())['open'])
            for w in worlds():
                if cat(pc(w))['open']:tool(pc(w),'Key:Q',(0,0))
            aim((4500,-6000,4500),(-500,0,0));stage=6;last=now
        elif stage==6 and now-last>2:
            tool(peer(),'PaveMove',(-700,-1700));stage=7;last=now
        elif stage==7 and now-last>2:
            check('sign_hologram_is_nonblocking',report(peer())['ghost_actors']>0 and report(peer())['ghost_blocking']==0);q['before_sign']=report()['digest'];tool(peer(),'PaveDown',(-700,-1700));tool(peer(),'PaveUp',(-700,-1700));stage=8;last=now
        elif stage==8 and now-last>3:
            check('sign_places_from_real_world_click',agree() and report()['digest']!=q['before_sign'] and peer().get_last_construction_error()==0)
            tool(host(),'HUD:CatalogRun',(0,0));stage=9;last=now
        elif stage==9 and now-last>2:
            check('business_running_before_equipment_selection',report()['factory_running']);tool(host(),'Key:Q',(0,0));stage=10;last=now
        elif stage==10 and now-last>2:
            click(host(),'CatalogGroupProduction');stage=11;last=now
        elif stage==11 and now-last>2:
            shot('Build_Catalogue_Production');stage=11.5;last=now
        elif stage==11.5 and now-last>1:
            click(host(),'Fabrication workstation',True);stage=12;last=now
        elif stage==12 and now-last>3:
            check('equipment_selection_pauses_only_business',not report()['factory_running'] and report()['factory_tool']=='Station' and not cat()['open']);tool(host(),'PaveMove',(-1300,900));stage=13;last=now
        elif stage==13 and now-last>2:
            q['workstation_hint']=report()['factory_hint'];check('workstation_has_valid_preview',report()['factory_preview_valid']);tool(host(),'PaveDown',(-1300,900));tool(host(),'PaveUp',(-1300,900));stage=14;last=now
        elif stage==14 and now-last>3:
            check('workstation_purchase_replicates',all_count('factory_stations',1));tool(peer(1),'Key:Q',(0,0));stage=15;last=now
        elif stage==15 and now-last>2:
            click(peer(1),'CatalogGroupStorage');tool(host(),'SelectMode',(0,0));stage=16;last=now
        elif stage==16 and now-last>2:
            click(peer(1),'CatalogLevel2');stage=17;last=now
        elif stage==17 and now-last>2:
            check('rack_levels_configurable_in_catalogue',cat(peer(1))['levels']==2);click(peer(1),'Parts rack',True);stage=18;last=now
        elif stage==18 and now-last>2:
            tool(peer(1),'PaveMove',(-600,500));stage=19;last=now
        elif stage==19 and now-last>2:
            q['rack_hint']=report(peer(1))['factory_hint'];check('rack_has_valid_preview',report(peer(1))['factory_preview_valid']);tool(peer(1),'PaveDown',(-600,500));tool(peer(1),'PaveUp',(-600,500));stage=20;last=now
        elif stage==20 and now-last>3:
            check('rack_purchase_replicates',all_count('factory_racks',1));tool(peer(1),'SelectMode',(0,0));tool(peer(1),'WheelClick',(150,400));stage=21;last=now
        elif stage==21 and now-last>2:
            check('old_sidebar_no_longer_blocks_world_input',not report(peer(1))['over_hud']);tool(host(),'Key:Q',(0,0));stage=22;last=now
        elif stage==22 and now-last>2:
            for key in 'SIGN':tool(host(),'Key:'+key,(0,0))
            stage=23;last=now
        elif stage==23 and now-last>2:
            check('search_filters_across_categories',cat()['search']=='sign' and cat()['matches']==1);shot('Build_Catalogue_Search');tool(host(),'Key:Enter',(0,0));stage=24;last=now
        elif stage==24 and now-last>2:
            check('search_enter_activates_sign',report()['tool']=='Sign' and not cat()['open']);tool(host(),'SelectMode',(0,0));N.request_late_join();stage=25;last=now
        elif stage==25 and now-last>10:
            check('fifth_join_receives_builds_and_purchases',len(worlds())==5 and agree() and all_count('factory_stations',1) and all_count('factory_racks',1));finish()
    except Exception:q['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
