"""Native visual and first-person interaction acceptance; fresh first-person shop left open."""
import unreal,time,json,traceback,runpy,re
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
observer=runpy.run_path(str(Path(__file__).with_name('observe_backend_playtest.py')))['_backend_playtest_observer']
runpy.run_path(str(Path(__file__).with_name('observe_playtest_world.py')))
tour=[];tour_next=0
erp_index=0;erp_pages=[9,0,1,2,3,10,11,4,5,6,7]
start=time.monotonic();stage=0;last=0;order=0;run_seen=False;q={'checks':{},'errors':[]};base=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()));out=base/'Screenshots/Reviews';out.mkdir(parents=True,exist_ok=True);log=base/'Logs/CommerceNativeQA.json';T=unreal.FoundrylineNetworkTestTools

def worlds():return unreal.EditorLevelLibrary.get_pie_worlds(False)
def pc():return next(p for w in worlds() for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller())
def r():return json.loads(T.commerce_report(pc()))
def ui():return json.loads(T.operations_ui_report(pc()))
def action(s):return T.queue_commerce_action(pc(),s)
def factory(s):return T.queue_factory_action(pc(),s)
def shot(n):unreal.SystemLibrary.execute_console_command(pc().get_world(),'Shot filename="'+str(out/('FirstOrder_'+n+'.png'))+'"');q.setdefault('screenshots',[]).append(str(out/('FirstOrder_'+n+'.png')))
def aim_carrier():
 carriers=unreal.GameplayStatics.get_all_actors_of_class(pc().get_world(),unreal.FoundrylineCarrierSceneActor)
 if carriers:
  at=carriers[0].get_actor_location();eye=at+unreal.Vector(1800,1700,1700);target=at+unreal.Vector(0,0,110);cam=pc().get_view_target();cam.set_actor_location(eye,False,False);cam.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(eye,target),False)

def check(n,v):q['checks'][n]=bool(v)
def done():observer.stop();q['screenshots']=[str(max([f for f in out.iterdir() if re.fullmatch(re.escape(Path(p).stem)+r'\d{5}\.png',f.name)],key=lambda f:f.stat().st_mtime)) for p in q.get('screenshots',[]) if [f for f in out.iterdir() if re.fullmatch(re.escape(Path(p).stem)+r'\d{5}\.png',f.name)]];q['completed']=True;log.write_text(json.dumps(q,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.EditorPythonScripting.set_keep_python_script_alive(False);runpy.run_path(str(Path(__file__).with_name('review_playtest_world_views.py')))

def begin_tour(items, continuation):
 global tour,tour_next,stage,last
 tour=list(items);tour_next=continuation;stage=5000;last=time.monotonic()-start

def tick(dt):
 global stage,last,order,run_seen,erp_index,tour
 try:
  t=time.monotonic()-start
  if t>420:raise RuntimeError('Native commerce timeout stage '+str(stage))
  if stage==0 and t>4:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1;last=t
  elif stage==1 and t-last>15:
   unreal.SystemLibrary.execute_console_command(pc().get_world(),'LevelEditor.ToggleImmersive');check('fresh_first_person_at_office',ui()['walking'] and ui()['near_office'] and ui()['roofs']);check('fresh_cash_no_free_stock',r()['cash']==400000 and not r()['pallets'] and not r()['running']);check('physical_computer_and_tablet_imported',len(ui()['terminals'])==2 and all(v['mesh'] and v['collision'] for v in ui()['terminals']));shot('FirstPersonStartup');factory('Walk');stage=2;last=t
  elif stage==2 and t-last>1:
   check('free_view_toggle_blocked',ui()['walking']);T.setup_operations_review(pc(),1);stage=3;last=t
  elif stage==3 and t-last>2:shot('OfficeDesk');stage=31;last=t
  elif stage==31 and t-last>1:factory('Interact');stage=4;last=t
  elif stage==4 and t-last>2:
   check('physical_E_opens_office',ui()['screen']==1 and ui()['ignore_move'] and ui()['cursor']);order=r()['orders'][0]['id'];action('Page0');action('Order'+str(order));stage=5;last=t
  elif stage==5 and t-last>2:
   shot('OfficeOrders');begin_tour([
    (['DocTab1'],'BuyerQuotation',{'page':0,'document_tab':1}),
    (['Sort3'],'SalesValueAscending',{'sort':3,'sort_descending':False}),
    (['Sort3'],'SalesValueDescending',{'sort':3,'sort_descending':True}),
    (['DocTab2'],'OfferActivity',{'document_tab':2}),
    (['Page1'],'CustomerInbox',{'page':1}),
    (['Sort1'],'InboxSenderSort',{'sort':1}),
    (['Page0','Order'+str(order)],'SalesSummary',{'page':0,'document_tab':0})],51)
  elif stage==51 and t-last>1:action('PropertyView');stage=6;last=t
  elif stage==6 and t-last>2:
   check('office_property_view_granted',not ui()['walking'] and ui()['property_access'] and not ui()['roofs']);shot('PropertyView');stage=61;last=t
  elif stage==61 and t-last>1:factory('Walk');stage=7;last=t
  elif stage==7 and t-last>2:
   check('property_returns_to_office',ui()['walking'] and ui()['near_office'] and not ui()['property_access']);factory('Interact');action('Order'+str(order));action('Accept');stage=8;last=t
  elif stage==8 and t-last>2:
   action('Assign');begin_tour([
    (['Page2','Order'+str(order),'DocTab1'],'AcceptedSalesDocument',{'page':2,'document_tab':1}),
    (['DocTab2'],'AcceptedSalesActivity',{'document_tab':2}),
    (['Page3','SheetsMinus'],'PurchaseDraft',{'page':3,'workspace_tab':0})],9)
  elif stage==9 and t-last>2:shot('BuySteel');stage=91;last=t
  elif stage==91 and t-last>1:action('BuySteel');stage=10;last=t
  elif stage==10 and t-last>2:
   check('steel_purchase_scheduled',len(r()['bookings'])==1 and r()['cash']<400000);begin_tour([
    (['Workspace1','DocTab1'],'PurchaseOrderDocument',{'page':3,'workspace_tab':1,'document_tab':1}),
    (['DocTab2'],'PurchaseActivity',{'document_tab':2}),
    (['Page4','Sort2'],'DispatchArrivalSort',{'page':4,'sort':2}),
    (['DocTab1'],'CarrierBookingDocument',{'document_tab':1}),
    (['Page9'],'LiveDashboard',{'page':9})],101)
  elif stage==101 and t-last>1:
   action('Plasma')
   for i in range(30):factory('BatchPlus')
   stage=11;last=t
  elif stage==11 and t-last>2:shot('PlasmaComputer');stage=111;last=t
  elif stage==111 and t-last>1:factory('Apply');stage=12;last=t
  elif stage==12 and t-last>.3:
   check('start_batch_runs_plant',r()['running']);T.advance_commerce_test(pc(),10)
   if any(p['place']==7 for p in r()['pallets']):factory('Run');T.setup_operations_review(pc(),3);aim_carrier();stage=13;last=t
   else:last=t
  elif stage==13 and t-last>3:shot('IncomingSteelTruck');stage=131;last=t
  elif stage==131 and t-last>1:factory('Run');stage=14;last=t
  elif stage==14 and t-last>.2:
   if sum(p['quantity'] for p in r()['pallets'] if p['place']==6)==40:factory('Run');stage=15;last=t
   else:T.advance_commerce_test(pc(),20);last=t
  elif stage==15 and t-last>2:T.setup_operations_review(pc(),1);stage=158;last=t
  elif stage==158 and t-last>1:factory('Interact');action('Page11');stage=154;last=t
  elif stage==154 and t-last>1:check('populated_inventory_open',ui()['screen']==1 and ui()['page']==11);shot('ERP_StockLedger');stage=155;last=t
  elif stage==155 and t-last>1:action('Page10');stage=156;last=t
  elif stage==156 and t-last>1:check('production_status_open',ui()['screen']==1 and ui()['page']==10);shot('ERP_WorkCentres');stage=157;last=t
  elif stage==157 and t-last>1:action('Close');stage=150;last=t
  elif stage==150 and t-last>2:
   at=unreal.Vector(-1250,2000,112);target=unreal.Vector(-975,2000,55);facing=unreal.MathLibrary.find_look_at_rotation(at+unreal.Vector(0,0,80),target);pawn=pc().get_controlled_pawn();pawn.set_actor_location(at,False,False);pawn.set_actor_rotation(unreal.Rotator(0,facing.yaw,0),False);pc().set_control_rotation(facing);stage=151;last=t
  elif stage==151 and t-last>1:factory('Interact');stage=152;last=t
  elif stage==152 and t-last>1:check('aimed_E_inspects_staged_pallet',ui()['screen']==3);shot('PalletInspection');stage=153;last=t
  elif stage==153 and t-last>1:action('Close');T.setup_operations_review(pc(),2);stage=16;last=t
  elif stage==16 and t-last>2:shot('LoadingTabletStand');stage=161;last=t
  elif stage==161 and t-last>1:factory('Interact');stage=17;last=t
  elif stage==17 and t-last>2:
   check('physical_E_opens_loading_tablet',ui()['screen']==2 and ui()['ignore_move']);action('PropertyView');check('tablet_cannot_enter_topdown',ui()['walking']);action('Page8');action('Carrier1')
   for p in r()['pallets']:
    if p['place']==6:action('Pallet'+str(p['id']))
   stage=18;last=t
  elif stage==18 and t-last>2:shot('TabletPickup');stage=181;last=t
  elif stage==181 and t-last>1:action('Book');stage=19;last=t
  elif stage==19 and t-last>2:
   check('pickup_booked_exact_pallet',sum(not b['inbound'] for b in r()['bookings'])==1);action('Page4');stage=20;last=t
  elif stage==20 and t-last>2:shot('TabletCalendar');stage=201;last=t
  elif stage==201 and t-last>1:action('Close');T.setup_operations_review(pc(),3);factory('Run');stage=21;last=t
  elif stage==21 and t-last>.2:
   if any(p['place']==8 for p in r()['pallets']):factory('Run');stage=22;last=t
   else:T.advance_commerce_test(pc(),5);last=t
  elif stage==22 and t-last>1:aim_carrier();stage=222;last=t
  elif stage==222 and t-last>2:shot('LoadedPickupTruck');stage=221;last=t
  elif stage==221 and t-last>1:factory('Run');stage=23;last=t
  elif stage==23 and t-last>.3:
   if next(o for o in r()['orders'] if o['id']==order)['delivered']==40:factory('Run');stage=24;last=t
   else:T.advance_commerce_test(pc(),500);last=t
  elif stage==24 and t-last>2:
   o=next(o for o in r()['orders'] if o['id']==order);check('complete_order_delivered_paid_once',o['paid']==44000 and r()['revenue']==44000);T.setup_operations_review(pc(),1);stage=25;last=t
  elif stage==25 and t-last>2:factory('Interact');action('Page5');stage=26;last=t
  elif stage==26 and t-last>2:
   shot('OfficeFinances');begin_tour([
    (['Sort2'],'LedgerMovementSort',{'sort':2,'page':5}),
    (['Workspace1'],'CompletedSalesStatement',{'workspace_tab':1}),
    (['DocTab1'],'CustomerPaymentReceipts',{'document_tab':1}),
    (['DocTab2'],'CustomerPaymentHistory',{'document_tab':2}),
    (['Page9'],'CompletedDashboard',{'page':9})],261)
  elif stage==261 and t-last>1:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_end_play();stage=27;last=t
  elif stage==27 and t-last>4:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=28;last=t
  elif stage==28 and t-last>15:
   check('left_fresh_first_person_shop',ui()['walking'] and ui()['near_office'] and r()['cash']==400000 and not r()['running'] and not r()['pallets']);q['final_ui']=ui();q['final_operations']=r();shot('ReadyToPlay');stage=29;last=t
  elif stage==29 and t-last>1:T.setup_operations_review(pc(),1);factory('Interact');stage=30;last=t
  elif stage==30 and t-last>1:action('Page'+str(erp_pages[erp_index]));stage=32;last=t
  elif stage==32 and t-last>1:
   check('ERP_page_'+str(erp_pages[erp_index]),ui()['screen']==1 and ui()['page']==erp_pages[erp_index]);shot('ERP_App'+str(erp_pages[erp_index]));stage=33;last=t
  elif stage==33 and t-last>1:
   erp_index+=1
   if erp_index==len(erp_pages):action('Close');stage=34;last=t
   else:stage=30;last=t
  elif stage==34 and t-last>1:check('ERP_exit_returns_to_first_person',ui()['walking'] and ui()['screen']==0);done();return
  if stage==5000 and t-last>1:
   actions,name,expected=tour[0]
   for a in actions:action(a)
   stage=5001;last=t
  elif stage==5001 and t-last>1:
   actions,name,expected=tour.pop(0);u=ui();check('ERP_V2_'+name,all(u.get(k)==v for k,v in expected.items()));shot('V2_'+name);stage=5000 if tour else tour_next;last=t
  q['stage']=stage;log.write_text(json.dumps(q,indent=2))
 except Exception:q['errors'].append(traceback.format_exc());done()
handle=unreal.register_slate_post_tick_callback(tick)
