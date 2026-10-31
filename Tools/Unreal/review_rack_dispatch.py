"""Fresh four-player rack-to-truck review using real purchases, production and RPCs. No save writes."""
import unreal,time,json,traceback
from pathlib import Path
R=Path(unreal.Paths.project_dir()).resolve();OUT=R/'Saved/Logs/RackDispatchNativeQA.json';SHOTS=R/'Saved/Screenshots/Reviews';SHOTS.mkdir(parents=True,exist_ok=True)
N=unreal.FoundrylineNetworkTestTools;L=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
L.load_level('/Game/Foundryline/Maps/L_FirstShopStarter');unreal.EditorPythonScripting.set_keep_python_script_alive(True)
q={'checks':{},'errors':[],'completed':False};start=time.monotonic();next_at=start+3
def players():return [p for w in unreal.EditorLevelLibrary.get_pie_worlds(False) for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller()]
def host():return next(p for p in players() if p.has_authority())
def state(p=None):return json.loads(N.commerce_report(p or host()))
def ui(p=None):return json.loads(N.operations_ui_report(p or host()))
def action(p,s):N.queue_commerce_action(p,s)
def factory(p,s):N.queue_factory_action(p,unreal.Name(s))
def check(n,v):
 q['checks'][n]=bool(v)
 if not v:q['errors'].append(n)
def shot(name):unreal.SystemLibrary.execute_console_command(host(),'Shot filename="'+str(SHOTS/('RackDispatch_'+name+'.png'))+'"')
def advance():
 N.advance_commerce_test(host(),700)
 if any(p['place']==6 for p in state()['pallets']):raise AssertionError('New production entered retired floor staging')
def shared():
 rs=[state(p) for p in players()];return all(all(r[k]==rs[0][k] for k in ['cash','orders','bookings','pallets']) for r in rs)
def flow():
 L.editor_request_begin_play();yield 18
 ps=players();check('four_players',len(ps)==4);h=host();c=next(p for p in ps if p!=h)
 for p in ps:N.setup_operations_review(p,1)
 yield 1
 order=next(o for o in state()['orders'] if o['part']==1 and o['status']==0);oid=order['id'];quantity=order['quantity'];q['order']=order
 action(c,'Office');yield 1;action(c,'Order'+str(oid));action(c,'Accept');yield 1
 check('client_accepts_order_authoritatively',all(any(o['id']==oid and o['status']==1 for o in state(p)['orders']) for p in ps))
 action(c,'Assign');yield 1;check('unmade_stock_cannot_be_prepared','No parts ready' in ui(c)['status'])
 action(c,'SheetsMinus');yield .5;action(c,'BuySteel');yield 1
 check('real_steel_purchase',len(state()['bookings'])==1 and state()['cash']<400000)
 action(c,'Close');N.queue_factory_console(h,1);yield 1;factory(h,'Interact');yield 1;
 q['physical_focus']=ui(h);
 if ui(h)['machine']!=1:raise RuntimeError('Physical plasma did not open: '+str(ui(h)))
 factory(h,'QueueJob'+str(oid));yield 1
 check('physical_plasma_queue',ui(h)['machine']==1 and any(m['queue_count'] or m['batch_remaining'] for m in state()['machines']))
 if not state()['running']:factory(h,'Run');yield .5
 for _ in range(90):
  if sum(p['quantity'] for p in state()['pallets'] if p['part']==1)>=quantity:break
  advance();yield .15
 check('real_brackets_cut',sum(p['quantity'] for p in state()['pallets'] if p['part']==1)==quantity)
 factory(h,'CloseComputer');yield .5;N.setup_operations_review(c,2);yield .5;action(c,'Tablet');yield .5;action(c,'Order'+str(oid));action(c,'Assign');yield 1
 fgslots={s['id'] for s in state()['slots'] if s['machine']==0 and s['id']!=100}
 for _ in range(80):
  ready=[p for p in state()['pallets'] if p['order']==oid and p['place']==1 and p['owner'] in fgslots and not p['reserved']]
  if sum(p['quantity'] for p in ready)==quantity:break
  advance();yield .15
 check('exact_stock_in_finished_rack',sum(p['quantity'] for p in ready)==quantity)
 if not ready:raise RuntimeError('No finished rack pallet: '+str(state()))
 for p in ready:
  s=next(s for s in state()['slots'] if s['id']==p['owner']);check('pallet_'+str(p['id'])+'_on_shelf',all(abs(p[k]-s[k])<.1 for k in ['x','y','z']))
 if state()['running']:factory(h,'Run');yield 1
 N.setup_operations_review(h,2);yield .5;action(h,'Tablet');yield .5;action(h,'Order'+str(oid));action(h,'Page8');yield .5;shot('ChoosePallets');yield .5
 action(c,'Page8');yield .5;action(c,'Pallet'+str(ready[0]['id']));yield .5
 check('client_selects_rack_pallet',ui(c)['selected_pallets']==1)
 action(c,'Carrier0');yield .5;check('truck_comparison_preserves_selection',ui(c)['selected_pallets']==1)
 action(h,'Pallet'+str(ready[0]['id']));action(h,'Carrier0');yield .5;shot('ReadyToBook');yield .5
 before=state()['cash'];action(c,'Book');yield 1
 freight=next(b for b in state()['bookings'] if not b['inbound'] and b['order']==oid);q['booking']=freight
 check('booking_charges_once',state()['cash']==before-freight['fee']);check('all_peers_booked_pallet_locked',all(any(p['id']==ready[0]['id'] and p['booking']==freight['id'] for p in state(peer)['pallets']) for peer in ps))
 yield .5;shot('AlreadyBooked');yield .5
 action(c,'Pallet'+str(ready[0]['id']));yield .5;check('booked_pallet_not_reselectable',ui(c)['selected_pallets']==0)
 action(c,'Book');yield 1;check('duplicate_has_no_fee',state()['cash']==before-freight['fee'])
 action(h,'Close');action(c,'Close');factory(h,'Run');yield .5
 for _ in range(90):
  if next(o for o in state()['orders'] if o['id']==oid)['delivered']==quantity:break
  advance();yield .15
 if state()['running']:factory(h,'Run');yield 1
 check('truck_loaded_from_rack_and_customer_paid',next(o for o in state()['orders'] if o['id']==oid)['delivered']==quantity and state()['revenue']>0)
 check('final_valid_ledger',state().get('authoritative_valid') and abs(state().get('mass_delta',999))<.2 and state().get('cash_matches_registry'))
 yield 1;check('four_peer_final_agreement',shared());q['final']=[state(p) for p in ps]
 # Inspect actual named, open-sided starter rack and real collision next to it.
 front=json.loads(N.probe_player_collision(h,unreal.Vector(-1200,1880,112),unreal.Vector(-500,1880,112)));q['front_aisle_collision']=front;check('rack_front_aisle_has_no_invisible_boundary',front.get('valid') and not front.get('blocked') and not front.get('penetrating'))
 N.setup_operations_review(h,4);yield 1;camera=N.create_review_camera(h);target=unreal.Vector(-850,2050,100);eye=target+unreal.Vector(520,-650,450);camera.set_actor_location_and_rotation(eye,unreal.MathLibrary.find_look_at_rotation(eye,target),False,True);h.set_view_target_with_blend(camera,0);yield 1;shot('OpenRackLabels');yield 1
 q['completed']=True
FLOW=flow()
def finish():
 OUT.write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(host() if players() else None,'QUIT_EDITOR')
def tick(dt):
 global next_at
 if time.monotonic()<next_at:return
 try:
  if time.monotonic()-start>300:raise TimeoutError('Rack dispatch review timeout')
  next_at=time.monotonic()+next(FLOW);OUT.write_text(json.dumps(q,indent=2))
 except StopIteration:finish()
 except Exception:q['errors'].append(traceback.format_exc());q['failure']=state() if players() else {};finish()
handle=unreal.register_slate_post_tick_callback(tick)
