"""Office computer screenshots across the starter loop (fresh -> accepted -> steel -> cutting -> staged -> booked -> paid).

Set builtins.foundryline_desktop_prefix (e.g. 'Before'/'After') before running. Images go to Saved/UIRedesign/<prefix>.
Uses the normal validated player actions; leaves a fresh first-person PIE session at the office computer.
"""
import unreal,time,json,traceback,builtins
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
T=unreal.FoundrylineNetworkTestTools
prefix=getattr(builtins,'foundryline_desktop_prefix','After')
base=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()))
out=base/'UIRedesign'/prefix;out.mkdir(parents=True,exist_ok=True)
log=out/'capture_report.json'
q={'prefix':prefix,'checks':{},'shots':[],'errors':[]}
start=time.monotonic();stage=0;last=0;order=0;queue=[];after_queue=0
def worlds():return unreal.EditorLevelLibrary.get_pie_worlds(False)
def pc():return next(p for w in worlds() for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller())
def r():return json.loads(T.commerce_report(pc()))
def ui():return json.loads(T.operations_ui_report(pc()))
def action(s):return T.queue_commerce_action(pc(),s)
def factory(s):return T.queue_factory_action(pc(),s)
def check(n,v):q['checks'][n]=bool(v)
def shot(n):
 unreal.SystemLibrary.execute_console_command(pc().get_world(),'Shot filename="'+str(out/(n+'.png'))+'"');q['shots'].append(n)
def tour(items,then):
 # items: (actions, name). Each item navigates, waits a second for the HUD, then captures.
 global queue,after_queue,stage,last
 queue=list(items);after_queue=then;stage=900;last=time.monotonic()-start
FRESH=[(['Page9'],'01_Fresh_Home'),(['Page0'],'02_Fresh_OrdersOffers'),(['Page1'],'03_Fresh_CustomerMail'),(['Page2'],'04_Fresh_YourOrders'),
 (['Page3'],'05_Fresh_BuySteel'),(['Page3','Workspace1'],'06_Fresh_SteelDeliveries'),(['Page10'],'07_Fresh_Production'),(['Page6'],'08_Fresh_Equipment'),
 (['Page11'],'09_Fresh_Stock'),(['Page4'],'10_Fresh_Shipping'),(['Page5'],'11_Fresh_Money'),(['Page5','Workspace1'],'12_Fresh_Statements'),
 (['Page7'],'13_Fresh_Team'),(['Page7','Workspace2'],'14_Fresh_Hire'),(['Page7','Workspace1'],'15_Fresh_ForkliftPriorities')]
def tick(dt):
 global stage,last,order,queue
 try:
  t=time.monotonic()-start
  if t>900:raise RuntimeError('Desktop loop capture timeout at stage '+str(stage))
  if stage==0 and t>4:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1;last=t
  elif stage==1 and t-last>15:
   unreal.SystemLibrary.execute_console_command(pc().get_world(),'LevelEditor.ToggleImmersive');check('fresh_cash',r()['cash']==400000 and not r()['pallets']);T.setup_operations_review(pc(),1);stage=2;last=t
  elif stage==2 and t-last>2:factory('Interact');stage=3;last=t
  elif stage==3 and t-last>2:
   check('office_open',ui()['screen']==1);order=r()['orders'][0]['id'];q['order']=order;tour(FRESH,10)
  elif stage==10 and t-last>1:
   action('Page0');action('Order'+str(order));stage=11;last=t
  elif stage==11 and t-last>1:shot('16_Offer_Selected');stage=12;last=t
  elif stage==12 and t-last>1:action('Accept');stage=13;last=t
  elif stage==13 and t-last>2:
   check('accepted',any(o['id']==order and o['status']==1 for o in r()['orders']) if 'status' in r()['orders'][0] else True)
   tour([(['Page9'],'17_Accepted_Home'),(['Page2','Order'+str(order)],'18_Accepted_YourOrder')],14)
  elif stage==14 and t-last>1:action('Assign');stage=15;last=t
  elif stage==15 and t-last>2:tour([(['Page9'],'19_Assigned_Home'),(['Page3','SheetsMinus'],'20_BuySteel_OneSheet')],16)
  elif stage==16 and t-last>1:action('BuySteel');stage=17;last=t
  elif stage==17 and t-last>2:
   check('steel_booked',len(r()['bookings'])==1 and r()['cash']<400000)
   tour([(['Page9'],'21_SteelOrdered_Home'),(['Page4'],'22_SteelOrdered_Shipping'),(['Page3','Workspace1'],'23_SteelOrdered_Deliveries')],18)
  elif stage==18 and t-last>1:action('Page9');action('ProgramOrder');stage=19;last=t
  elif stage==19 and t-last>2:shot('24_PlasmaProgrammed');stage=20;last=t
  elif stage==20 and t-last>1:factory('Apply');stage=21;last=t
  elif stage==21 and t-last>.3:
   check('batch_started_runs_plant',r()['running']);T.advance_commerce_test(pc(),10)
   if any(p['place']==1 for p in r()['pallets']):factory('Run');stage=22;last=t
   else:last=t
  elif stage==22 and t-last>1:T.setup_operations_review(pc(),1);stage=23;last=t
  elif stage==23 and t-last>1:factory('Interact');stage=24;last=t
  elif stage==24 and t-last>1:tour([(['Page9'],'25_SteelRacked_Home'),(['Page10'],'26_Cutting_Production'),(['Page11'],'27_SteelRacked_Stock')],25)
  elif stage==25 and t-last>1:action('Close');factory('Run');stage=26;last=t
  elif stage==26 and t-last>.2:
   if sum(p['quantity'] for p in r()['pallets'] if p['place']==6)>=40:factory('Run');stage=27;last=t
   else:T.advance_commerce_test(pc(),20);last=t
  elif stage==27 and t-last>2:T.setup_operations_review(pc(),1);stage=28;last=t
  elif stage==28 and t-last>1:factory('Interact');stage=29;last=t
  elif stage==29 and t-last>1:tour([(['Page9'],'28_PartsStaged_Home'),(['Page11'],'29_PartsStaged_Stock'),(['Page10'],'30_PartsStaged_Production')],30)
  elif stage==30 and t-last>1:action('Close');T.setup_operations_review(pc(),2);stage=31;last=t
  elif stage==31 and t-last>2:factory('Interact');stage=32;last=t
  elif stage==32 and t-last>2:
   action('Page8');action('Carrier1')
   for p in r()['pallets']:
    if p['place']==6:action('Pallet'+str(p['id']))
   stage=33;last=t
  elif stage==33 and t-last>2:shot('31_Tablet_PickupSelected');stage=34;last=t
  elif stage==34 and t-last>1:action('Book');stage=35;last=t
  elif stage==35 and t-last>2:check('pickup_booked',sum(not b['inbound'] for b in r()['bookings'])==1);action('Close');T.setup_operations_review(pc(),1);stage=36;last=t
  elif stage==36 and t-last>2:factory('Interact');stage=37;last=t
  elif stage==37 and t-last>1:tour([(['Page9'],'32_PickupBooked_Home'),(['Page4'],'33_PickupBooked_Shipping')],38)
  elif stage==38 and t-last>1:action('Close');factory('Run');stage=39;last=t
  elif stage==39 and t-last>.3:
   if next(o for o in r()['orders'] if o['id']==order)['delivered']>=40:factory('Run');stage=40;last=t
   else:T.advance_commerce_test(pc(),500);last=t
  elif stage==40 and t-last>2:
   o=next(o for o in r()['orders'] if o['id']==order);check('delivered_and_paid',o['paid']==44000);T.setup_operations_review(pc(),1);stage=41;last=t
  elif stage==41 and t-last>2:factory('Interact');stage=42;last=t
  elif stage==42 and t-last>1:tour([(['Page9'],'34_Paid_Home'),(['Page5'],'35_Paid_Money'),(['Page5','Workspace1'],'36_Paid_Statements'),(['Page2','Order'+str(order)],'37_Paid_YourOrders'),(['Page7'],'38_Paid_Team')],43)
  elif stage==43 and t-last>1:
   q['final']=r()['cash'];action('Close');unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_end_play();stage=44;last=t
  elif stage==44 and t-last>4:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=45;last=t
  elif stage==45 and t-last>15:
   check('left_fresh_session',r()['cash']==400000 and not r()['running']);q['completed']=True;log.write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.EditorPythonScripting.set_keep_python_script_alive(False);return
  if stage==900 and t-last>1:
   for a in queue[0][0]:action(a)
   stage=901;last=t
  elif stage==901 and t-last>1.2:
   shot(queue.pop(0)[1]);stage=900 if queue else after_queue;last=t
  q['stage']=stage;log.write_text(json.dumps(q,indent=2))
 except Exception:
  q['errors'].append(traceback.format_exc());log.write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.EditorPythonScripting.set_keep_python_script_alive(False)
handle=unreal.register_slate_post_tick_callback(tick)
