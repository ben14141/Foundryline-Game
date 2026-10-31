"""Unsaved PIE review: real office forklift purchase, owned placement and catalogue.
No cash grants, authored map writes, content imports or saved games. Station is a
preview only. Storing/restoring the idle starter plasma exercises paid ownership.
"""
import unreal,time,json,traceback
from pathlib import Path
ROOT=Path(unreal.Paths.project_dir()).resolve();OUT=ROOT/'Saved/Screenshots/Reviews';OUT.mkdir(parents=True,exist_ok=True)
N=unreal.FoundrylineNetworkTestTools;LEVELS=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
LEVELS.load_level('/Game/Foundryline/Maps/L_FirstShopStarter');unreal.EditorPythonScripting.set_keep_python_script_alive(True)
start=time.monotonic();last=start;index=-1;camera=None;before={};fork_at=None;plasma_at=None
q={'checks':{},'errors':[],'screenshots':[],'notes':['Fresh unsaved PIE, no artificial cash or inventory. Station is a noncommitted preview.']}
def worlds():return list(unreal.EditorLevelLibrary.get_pie_worlds(False))
def host():return next(p for w in worlds() if unreal.GameplayStatics.get_game_mode(w) for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller())
def commerce():return json.loads(N.commerce_report(host()))
def ui():return json.loads(N.operations_ui_report(host()))
def pavement():return json.loads(N.pavement_report(host()))
def cat():return json.loads(pavement()['catalog'])
def action(name):N.queue_commerce_action(host(),name)
def factory(name):N.queue_factory_action(host(),unreal.Name(name))
def tool(name,a=(0,0),b=None,channel=0):
 b=b or a;return N.queue_construction_tool(host(),unreal.Name(name),unreal.IntVector(int(a[0]),int(a[1]),0),unreal.IntVector(int(b[0]),int(b[1]),0),channel,unreal.LinearColor(1,1,1,1))
def check(name,value):
 q['checks'][name]=bool(value)
 if not value:q['errors'].append(name)
def shot(name):
 name='OwnedUI_'+name;q['screenshots'].append(name);unreal.SystemLibrary.execute_console_command(host(),'Shot filename="'+str(OUT/(name+'.png'))+'"')
def aim(target=(-1300,800,20)):
 global camera
 if not camera:camera=N.create_review_camera(host());camera.get_component_by_class(unreal.CameraComponent).set_field_of_view(55)
 target=unreal.Vector(*target);at=target+unreal.Vector(1600,-2200,2200)
 camera.set_actor_location_and_rotation(at,unreal.MathLibrary.find_look_at_rotation(at,target),False,True);host().set_view_target_with_blend(camera,0)
def click_item(name):
 hit=next(b for b in cat()['buttons'] if b.get('item')==name);tool('WheelClick',(round(hit['x']),round(hit['y'])))
def open_catalog(group):
 if not cat()['open']:tool('HUD:CatalogOpen')
 tool('HUD:CatalogGroup'+group)
def pause():
 if commerce()['running']:factory('Run')
def before_buy():
 before['cash']=commerce()['cash'];before['drivers']=len(commerce()['drivers']);action('BuyEquipment4')
def bought():
 check('office_purchase_charges_exact_forklift_price',commerce()['cash']==before['cash']-120000)
 check('office_purchase_does_not_spawn_forklift',len(commerce()['drivers'])==before['drivers'])
 q['office_purchase_status']=ui()['status'];shot('OfficePurchaseReceipt')
def choose_fork_position():
 global fork_at
 aim((-1300,800,20));candidates=[(-1300,300),(-1000,300),(-1600,300),(-800,350),(-1600,1850),(0,300)]
 for x,y in candidates:
  p=unreal.Vector(x,y,20);result=json.loads(N.preview_factory_brush(host(),unreal.Name('Equipment'),p,p,False,False))
  if result['valid']:fork_at=(x,y);N.aim_factory_brush(host(),p);q['forklift_position']=fork_at;return
 raise RuntimeError('No clear paid-forklift position: '+pavement().get('factory_hint',''))
def place_fork():
 before['placement_cash']=commerce()['cash'];tool('PaveDown',fork_at);tool('PaveUp',fork_at)
def fork_placed():
 check('planner_placed_exactly_one_owned_forklift',len(commerce()['drivers'])==before['drivers']+1)
 check('owned_forklift_placement_has_no_second_charge',commerce()['cash']==before['placement_cash']);tool('SelectMode')
def store_plasma():
 global plasma_at
 machine=next(m for m in commerce()['machines'] if m['id']==1);plasma_at=(machine['x'],machine['y']);tool('PlannerStore',(1,0),channel=1)
def stored():
 check('idle_starter_plasma_can_return_to_owned_storage',not any(s['machine']==1 for s in commerce()['slots']));open_catalog('Production')
def markers(label):
 rows=[]
 for actor in unreal.GameplayStatics.get_all_actors_of_class(host(),unreal.Actor):
  for part in actor.get_components_by_class(unreal.SceneComponent):
   if not part.component_has_tag('FoundrylinePlacementFlow') or not part.is_visible():continue
   row={'component':part.get_name(),'tags':[str(t) for t in part.get_editor_property('component_tags')],'position':str(part.get_world_location())}
   if isinstance(part,unreal.TextRenderComponent):row['text']=str(part.get_editor_property('text'))
   if isinstance(part,unreal.PrimitiveComponent):row['no_collision']=part.get_collision_enabled()==unreal.CollisionEnabled.NO_COLLISION
   rows.append(row)
 q[label+'_markers']=rows
 check(label+'_clear_in_out_labels',sorted(r['text'] for r in rows if 'text' in r)==['IN','OUT'])
 check(label+'_solid_nonblocking_arrow_geometry',sum('text' not in r for r in rows)==6 and all(r.get('no_collision',True) for r in rows))
def preview_plasma():
 aim((plasma_at[0],plasma_at[1],20));N.aim_factory_brush(host(),unreal.Vector(plasma_at[0],plasma_at[1],20))
def plasma_shot():markers('plasma');q['plasma_preview_ui']=ui();shot('OwnedPlasmaHologram')
def restore_plasma():tool('PlannerPlace',(1,0),plasma_at,1)
def restored():check('owned_plasma_restores_without_purchase',any(s['machine']==1 for s in commerce()['slots']));tool('SelectMode')
def station_preview():
 aim((plasma_at[0],plasma_at[1],20));N.aim_factory_brush(host(),unreal.Vector(plasma_at[0],plasma_at[1],20))
def station_shot():markers('station');q['station_preview_hint']=pavement().get('factory_hint');shot('StationInOutHologram')
def validate_assets():
 for name in ['Plasma','Brake','Roller','ToolForklift']:check('real_model_texture_'+name,bool(unreal.load_asset('/Game/Foundryline/UI/BuildCatalogue/T_Catalog_'+name)))
 check('office_equipment_page_opens',ui()['page']==6);shot('OfficeBuyEquipmentFourCards')
def finish():
 q['completed']=True;q['elapsed_seconds']=round(time.monotonic()-start,2);q['last_step']=index
 if worlds():q['final_commerce']=commerce();check('final_authoritative_ledger_valid',q['final_commerce'].get('authoritative_valid',False))
 q['screenshots']=[str(max(OUT.glob(n+'*.png'),key=lambda p:p.stat().st_mtime,default=OUT/(n+'.png'))) for n in q['screenshots']]
 (ROOT/'Saved/Logs/OwnedEquipmentUIQA.json').write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle)
 unreal.SystemLibrary.execute_console_command(host() if worlds() else None,'QUIT_EDITOR')
steps=[
 (20,lambda:N.setup_operations_review(host(),1)),
 (2,lambda:action('Office')),(2,lambda:action('Page6')),(2,validate_assets),
 (1,before_buy),(2,bought),(1,lambda:action('PropertyView')),
 (3,pause),(1,lambda:open_catalog('Production')),(2,lambda:shot('ProductionCatalogue')),
 (1,lambda:open_catalog('Logistics')),(2,lambda:click_item('Forklift')),
 (2,choose_fork_position),(2,lambda:shot('PaidForkliftPlacement')),(1,place_fork),(2,fork_placed),
 (1,store_plasma),(2,stored),(2,lambda:shot('OwnedPlasmaCatalogue')),
 (1,lambda:click_item('Plasma table')),(2,preview_plasma),(2,plasma_shot),
 (1,lambda:tool('PaveCancel',plasma_at)),(1,lambda:open_catalog('Production')),(2,lambda:click_item('Fabrication workstation')),
 (2,station_preview),(2,station_shot),(1,lambda:tool('SelectMode')),(1,restore_plasma),(2,restored),
 (1,lambda:N.setup_operations_review(host(),1)),(2,lambda:action('Close')),(2,lambda:shot('FirstPersonHeader')),(1,finish)
]
def tick(dt):
 global index,last
 try:
  now=time.monotonic()
  if now-start>240:raise RuntimeError('Owned equipment UI timeout at step '+str(index))
  if index<0:
   if now-start>3:LEVELS.editor_request_begin_play();index=0;last=now
   return
  delay,operation=steps[index]
  if now-last<delay:return
  operation();index+=1;last=now
 except Exception:q['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
