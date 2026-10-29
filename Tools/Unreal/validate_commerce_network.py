"""Four-player first-order acceptance through real owning-controller RPCs."""
import unreal,time,json,traceback
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
start=time.monotonic();stage=0;last=0;host=None;guests=[];order=0;before_cash=0;booked=0;q={'checks':{},'errors':[]};out=Path(unreal.Paths.project_saved_dir())/'Logs/CommerceNetworkQA.json'
T=unreal.FoundrylineNetworkTestTools

def make_guid(text):
 g=unreal.Guid()
 for n,i in zip('abcd',range(0,32,8)):g.set_editor_property(n,int(text.replace('-','')[i:i+8],16))
 return g

def worlds():return unreal.EditorLevelLibrary.get_pie_worlds(False)
def players():return [p for w in worlds() for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller()]
def report(p):return json.loads(T.commerce_report(p))
def action(p,s):return T.queue_commerce_action(p,s)
def factory(p,s):return T.queue_factory_action(p,s)
def check(n,v):q['checks'][n]=bool(v)
def shared():
 rs=[report(p) for p in players()];keys=['cash','cost','revenue','orders','bookings','pallets'];return all(all(r.get(k)==rs[0].get(k) for k in keys) for r in rs)
def finish():
 q['completed']=True;out.write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle)
 if worlds():unreal.SystemLibrary.execute_console_command(worlds()[0],'QUIT_EDITOR')

def tick(dt):
 global stage,last,host,guests,order,before_cash,booked
 try:
  t=time.monotonic()-start
  if t>420:raise RuntimeError('Commerce network timeout stage '+str(stage))
  if stage==0 and t>3:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1;last=t
  elif stage==1 and t-last>15:
   ps=players();host=next(p for p in ps if p.has_authority());guests=[p for p in ps if not p.has_authority()];rs=[report(p) for p in ps];check('four_connected_players',len(ps)==4);check('fresh_private_cash_and_no_free_steel',all(r.get('enabled') and r['cash']==400000 and not r['running'] and not r['pallets'] for r in rs));check('private_startup_state_matches',shared());q['initial_ui']=[json.loads(T.operations_ui_report(p)) for p in ps];order=rs[0]['orders'][0]['id']
   check('all_players_start_first_person',all(json.loads(T.operations_ui_report(p))['walking'] for p in ps));factory(guests[0],'Walk');stage=101;last=t
  elif stage==101 and t-last>2:
   check('free_topdown_toggle_blocked',json.loads(T.operations_ui_report(guests[0]))['walking']);action(guests[0],'Office');action(guests[0],'PropertyView');stage=102;last=t
  elif stage==102 and t-last>2:
   ui=json.loads(T.operations_ui_report(guests[0]));q['property_ui']=[json.loads(T.operations_ui_report(p)) for p in players()];check('office_computer_grants_only_requesting_player_property_view',not ui['walking'] and ui['property_access'] and guests[0].get_controlled_pawn() is not None and guests[0].get_controlled_pawn().get_class().get_name()=='FoundrylineCameraPawn' and all(json.loads(T.operations_ui_report(p))['walking'] for p in players() if p!=guests[0]));factory(guests[0],'Walk');stage=103;last=t
  elif stage==103 and t-last>2:
   ui=json.loads(T.operations_ui_report(guests[0]));check('return_to_office_first_person_with_roof',ui['walking'] and ui['near_office'] and ui['roofs'] and not ui['property_access']);action(guests[0],'Tablet');action(guests[0],'PropertyView');stage=104;last=t
  elif stage==104 and t-last>1:
   check('tablet_cannot_grant_property_view',json.loads(T.operations_ui_report(guests[0]))['walking']);action(guests[0],'Close')
   for p in guests[:2]:action(p,'Order'+str(order));action(p,'Accept')
   stage=2;last=t
  elif stage==2 and t-last>2:
   rs=[report(p) for p in players()];check('simultaneous_accept_has_one_accepted_order',all(sum(o['status']==1 for o in r['orders'])==1 for r in rs));check('second_accept_rejected',any('no longer' in report(p)['status'] for p in guests[:2]));action(guests[0],'Assign');action(guests[0],'SheetsMinus');stage=30;last=t
  elif stage==30 and t-last>2:action(guests[0],'BuySteel');stage=3;last=t
  elif stage==3 and t-last>2:
   check('remote_purchase_debits_once_and_books_delivery',len(report(host)['bookings'])==1 and report(host)['cash']<400000);factory(guests[1],'Computer1')
   for i in range(30):factory(guests[1],'BatchPlus')
   factory(guests[1],'Apply');stage=4;last=t
  elif stage==4 and t-last>.5:
   r=report(host)
   if sum(p['quantity'] for p in r['pallets'] if p['place']==6)>=40:factory(host,'Run');stage=5;last=t
   else:T.advance_commerce_test(host,2000);last=t
  elif stage==5 and t-last>2:
   check('remote_program_produces_and_stages_exact_order',sum(p['quantity'] for p in report(host)['pallets'] if p['place']==6)==40);check('production_inventory_cash_mirror',shared());pallets=[p for p in report(host)['pallets'] if p['place']==6]
   for p in guests[:2]:
    action(p,'Carrier1')
    for v in pallets:action(p,'Pallet'+str(v['id']))
    action(p,'Book')
   before_cash=report(host)['cash'];stage=6;last=t
  elif stage==6 and t-last>2:
   rs=[report(p) for p in players()];check('simultaneous_booking_single_fee_single_load',all(sum(not b['inbound'] for b in r['bookings'])==1 and r['cash']==before_cash-4000 for r in rs));check('duplicate_booking_rejected',any('already booked' in report(p)['status'] or 'changed' in report(p)['status'] for p in guests[:2]));check('no_revenue_before_delivery',all(r['revenue']==0 for r in rs));factory(host,'Run');stage=7;last=t
  elif stage==7 and t-last>.5:
   r=report(host);o=next(o for o in r['orders'] if o['id']==order)
   if o['delivered']==40:factory(host,'Run');stage=8;last=t
   else:T.advance_commerce_test(host,2000);last=t
  elif stage==8 and t-last>2:
   rs=[report(p) for p in players()];check('customer_delivery_and_completion_payment_on_all_peers',all(next(o for o in r['orders'] if o['id']==order)['paid']==44000 and r['revenue']==44000 for r in rs));check('final_private_state_matches',shared());setup=json.loads(T.exercise_business_foundation(host,0));setup.pop('create_error',None);q['checks'].update(setup);stage=9;last=t
  elif stage==9 and t-last>14:
   rs=[report(p) for p in players()];check('three_companies_have_independent_cash',len(set(r['company'] for r in rs))==3 and len(set(r['cash'] for r in rs))==3);check('clients_receive_only_their_company_operations',all(len(r['private_actors'])==1 and r['private_actors'][0]['company']==r['company'] for p,r in zip(players(),rs) if not p.has_authority()));foreign=next(r['plant'] for r in rs if r['company']!=report(guests[0])['company']);before_cash=report(guests[0])['cash'];T.queue_commerce_foreign_target(guests[0],make_guid(foreign));stage=10;last=t
  elif stage==10 and t-last>2:
   check('forged_foreign_target_denied_without_debit',report(guests[0])['cash']==before_cash and 'Join this company' in report(guests[0])['status']);check('late_join_requested',T.request_late_join());stage=11;last=t
  elif stage==11 and t-last>1:
   ps=players();rs=[report(p) for p in ps];clients=[r for p,r in zip(ps,rs) if not p.has_authority()]
   foreign=any(any(a['company']!=r['company'] for a in r.get('private_actors',[])) for r in clients)
   ready=len(ps)==5 and all(r.get('enabled') and len(r.get('private_actors',[]))==1 and r['private_actors'][0]['company']==r.get('company') for r in clients)
   q['late_join_wait_seconds']=t-last
   if foreign or ready or t-last>60:
    check('fifth_player_joins',len(ps)==5);check('late_join_receives_only_own_company_private_state',ready and not foreign);q['reports']=rs;finish();return
  q['stage']=stage;out.write_text(json.dumps(q,indent=2))
 except Exception:q['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
