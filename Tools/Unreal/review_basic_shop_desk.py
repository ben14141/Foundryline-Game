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
    (root/'Saved/Logs/BasicShopDeskQA.json').write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(host() if worlds() else None,'QUIT_EDITOR')
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


def ui(p):return json.loads(N.operations_ui_report(p))
def empty_shop(w):
 plants=list(unreal.GameplayStatics.get_all_actors_of_class(w,unreal.FoundrylineFactoryPlant))
 if len(plants)!=1:return False
 r=json.loads(plants[0].get_factory_report())
 forbidden=['FoundrylinePlasmaTable','FoundrylinePressBrakeDisplay','FoundrylineMaterialHandlingCell','FoundrylineFreightRig','FoundrylineWeldingStation']
 return r['pallets']==0 and r['jobs']==0 and plants[0].get_zone_count()==0 and not any(a.get_class().get_name() in forbidden for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.Actor))

def tick(dt):
 global stage,last
 try:
  now=time.monotonic()
  if now-start>180:raise RuntimeError('Desk planning review timed out at '+str(stage))
  if stage==0 and now-start>3:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1;last=now
  elif stage==1 and now-last>14:
   if len(worlds())<4:return
   check('four_players_start_first_person_at_office',all(ui(pc(w))['walking'] for w in worlds()))
   check('shop_still_has_no_production_equipment_or_staff',all(empty_shop(w) for w in worlds()))
   check('working_desk_replicated_to_all_players',all(len(ui(pc(w))['terminals'])==1 and ui(pc(w))['terminals'][0]['mesh'] and ui(pc(w))['terminals'][0]['collision'] for w in worlds()))
   for w in worlds():N.queue_commerce_action(pc(w),'Office')
   stage=2;last=now
  elif stage==2 and now-last>3:
   check('office_computer_opens_erp',all(ui(pc(w))['screen']==1 for w in worlds()))
   for w in worlds():N.queue_commerce_action(pc(w),'PropertyView')
   stage=3;last=now
  elif stage==3 and now-last>3:
   check('computer_grants_planning_access',all(not ui(pc(w))['walking'] and report(pc(w))['wheel_available'] for w in worlds()))
   q['before_build']=report()['digest'];tool(peer(),'Building',(4000,3000),(4800,3800));stage=4;last=now
  elif stage==4 and now-last>3:
   check('client_can_place_after_using_computer',agree() and report()['digest']!=q['before_build'] and peer().get_last_construction_error()==0)
   tool(peer(1),'Demolish',(4400,3400));stage=5;last=now
  elif stage==5 and now-last>3:
   check('client_can_remove_test_building',agree() and peer(1).get_last_construction_error()==0)
   for w in worlds():N.queue_commerce_action(pc(w),'ReturnOffice')
   stage=6;last=now
  elif stage==6 and now-last>3:
   check('return_from_planning_restores_first_person',all(ui(pc(w))['walking'] for w in worlds()));finish()
 except Exception:q['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
