"""Unsaved four-player playtest acceptance; no authored map or save mutation."""
import unreal,time,json,traceback
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve();out=root/'Saved/Screenshots/Reviews';out.mkdir(parents=True,exist_ok=True)
N=unreal.FoundrylineNetworkTestTools;levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
levels.load_level('/Game/Foundryline/Maps/L_FirstShopStarter');unreal.EditorPythonScripting.set_keep_python_script_alive(True)
started=time.monotonic();last=started;stage=0;production_rounds=0;result={'checks':{},'errors':[]};driver=0;order=0

def worlds():return list(unreal.EditorLevelLibrary.get_pie_worlds(False))
def pc(w):return next(p for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller())
def host():return next(pc(w) for w in worlds() if unreal.GameplayStatics.get_game_mode(w))
def peers():return [pc(w) for w in worlds()]
def plant(p):return unreal.GameplayStatics.get_all_actors_of_class(p,unreal.FoundrylineFactoryPlant)[0]
def snap(p):return report(p)
def report(p):return json.loads(N.commerce_report(p))
def ui(p):return json.loads(N.operations_ui_report(p))
def check(k,v):
 result['checks'][k]=bool(v)
 if not v:result['errors'].append(k)
def action(p,s):N.queue_commerce_action(p,s)
def factory(p,s):N.queue_factory_action(p,unreal.Name(s))
def shot(n):unreal.SystemLibrary.execute_console_command(host(),'Shot filename="'+str(out/(n+'.png'))+'"')
def finish():
 result['completed']=True;(root/'Saved/Logs/PlaytestBacklogNativeQA.json').write_text(json.dumps(result,indent=2));unreal.unregister_slate_post_tick_callback(handle)
 unreal.SystemLibrary.execute_console_command(host() if worlds() else None,'QUIT_EDITOR')
def tick(dt):
 global stage,last,driver,order,pallet,client,production_rounds
 try:
  now=time.monotonic()
  if now-started>300:raise RuntimeError('Backlog acceptance timeout at '+str(stage))
  if stage==0 and now-started>3:levels.editor_request_begin_play();stage=1;last=now
  elif stage==1 and now-last>18:
   if len(worlds())!=4:return
   check('four_player_session',len(peers())==4)
   for i,p in enumerate(peers()):
    state=snap(p);check('peer%d_four_plasma_output_positions'%i,len([s for s in state["slots"] if s["machine"]==1 and s["output"]])==4)
    N.setup_operations_review(p,1)
   driver=report(host())['drivers'][0]['id'];factory(host(),'Computer1');stage=2;last=now
  elif stage==2 and now-last>2:
   check('office_cannot_open_remote_plasma',ui(host())['machine']==0)
   for p in peers():action(p,'Office')
   stage=3;last=now
  elif stage==3 and now-last>2:
   for p in peers():action(p,'Page7')
   stage=4;last=now
  elif stage==4 and now-last>1:
   for p in peers():action(p,'Workspace1')
   stage=5;last=now
  elif stage==5 and now-last>1:
   action(host(),'PrioritySet%d_0_0'%driver);stage=6;last=now
  elif stage==6 and now-last>2:
   check('driver_checkbox_disable_replicates',all(report(p)["drivers"][0]["priorities"]==[-1,1,2] for p in peers()))
   other=next(p for p in peers() if p!=host());action(other,'PrioritySet%d_0_0'%driver);stage=7;last=now
  elif stage==7 and now-last>2:
   check('client_can_enable_all_three_tasks',all(report(p)["drivers"][0]["priorities"]==[0,1,2] for p in peers()));shot('Playtest_DriverPriorities');stage=7.1;last=now
  elif stage==7.1 and now-last>1:
   other=next(p for p in peers() if p!=host());before=report(host());result['pre_client_purchase']=before;N.place_player_collision_review(other,unreal.GameplayStatics.get_player_pawn(host(),0).get_actor_location(),host().get_control_rotation().yaw);action(other,'BuyEquipment4');stage=7.2;last=now
  elif stage==7.2 and now-last>2:
   check('client_can_buy_unplaced_forklift_at_office',all(report(p)['cash']==result['pre_client_purchase']['cash']-120000 for p in peers()));check('office_purchase_does_not_spawn_active_vehicle',len(report(host())['drivers'])==len(result['pre_client_purchase']['drivers']));stage=7.5;last=now
  elif stage==7.5 and now-last>1:action(host(),'Close');N.queue_factory_console(host(),1);stage=8;last=now
  elif stage==8 and now-last>2:factory(host(),'Interact');stage=9;last=now
  elif stage==9 and now-last>2:
   check('physical_plasma_terminal_opens',ui(host())['machine']==1);check('other_players_stay_in_independent_office_ui',all(ui(p)['screen']==1 for p in peers() if p!=host()));shot('Playtest_PlasmaControl');stage=9.5;last=now
  elif stage==9.5 and now-last>1:factory(host(),'CloseComputer');N.setup_operations_review(host(),1);stage=10;last=now
  elif stage==10 and now-last>2:action(host(),'Tablet');stage=11;last=now
  elif stage==11 and now-last>2:
   action(host(),'Page0');stage=12;last=now
  elif stage==12 and now-last>1:
   check('dispatch_excludes_sales_offers',ui(host())['page']==2);shot('Playtest_Dispatch');stage=12.5;last=now
  elif stage==12.5 and now-last>1:action(host(),'Close');N.setup_operations_review(host(),0);stage=13;last=now
  elif stage==13 and now-last>3:
   check('planner_opened',not ui(host())['walking']);shot('Playtest_WorkspaceOverview');stage=14;last=now
  elif stage==14 and now-last>2:
   N.setup_operations_review(host(),1);stage=15;last=now
  elif stage==15 and now-last>1:action(host(),'Office');stage=16;last=now
  elif stage==16 and now-last>1:
   action(host(),'Page0');order=next(o['id'] for o in report(host())['orders'] if o['part']==1 and o['status']==0);action(host(),'Order%d'%order);stage=17;last=now
  elif stage==17 and now-last>1:action(host(),'Accept');stage=18;last=now
  elif stage==18 and now-last>2:
   check('accepted_contract_replicates',all(any(o['id']==order and o['status']==1 for o in report(p)['orders']) for p in peers()));action(host(),'Assign');stage=18.1;last=now
  elif stage==18.1 and now-last>1:
   check('cannot_move_contract_before_parts_exist',report(host())['pickup_order']==0 and not any(p['place']==6 for p in report(host())['pallets']));action(host(),'BuySteel');client=next(p for p in peers() if p!=host());action(client,'Close')
   m=report(host())['machines'][0];N.place_player_collision_review(client,unreal.Vector(m['x']+499,m['y']+197,m['z']+88),180);stage=19;last=now
  elif stage==19 and now-last>2:factory(client,'Interact');stage=20;last=now
  elif stage==20 and now-last>2:
   check('client_physical_machine_opens',ui(client)['machine']==1);factory(client,'QueueJob%d'%order);stage=21;last=now
  elif stage==21 and now-last>2:
   check('client_job_queue_replicates',all(any(m['queue_count']>0 or m['batch_remaining']>0 or m['remaining']>0 for m in report(p)['machines']) for p in peers()));N.advance_commerce_test(host(),2000);stage=22;last=now
  elif stage==22 and now-last>2:N.advance_commerce_test(host(),2000);stage=23;last=now
  elif stage==23 and now-last>2:
   stock=[p for p in report(host())['pallets'] if p['part']==1 and p['place']==1 and not p['booking']];need=next(o['quantity'] for o in report(host())['orders'] if o['id']==order)
   result['production_report']=report(host())
   if sum(p['quantity'] for p in stock)<need and production_rounds<12:
    production_rounds+=1;N.advance_commerce_test(host(),2000);last=now;return
   check('real_delivery_and_client_queued_plasma_produce_contract',sum(p['quantity'] for p in stock)>=need)
   if not stock:raise RuntimeError('No completed stock for staging review')
   pallet=stock[0]['id'];N.setup_operations_review(host(),0);stage=24;last=now
  elif stage==24 and now-last>2:
   stock=next(p for p in report(host())['pallets'] if p['id']==pallet);N.queue_construction_tool(host(),'PlannerSelect',unreal.IntVector(round(stock['x']),round(stock['y']),0),unreal.IntVector(),0,unreal.LinearColor(1,1,1,1));stage=25;last=now
  elif stage==25 and now-last>1:N.queue_construction_tool(host(),'HUD:PlannerInspect',unreal.IntVector(),unreal.IntVector(),0,unreal.LinearColor(1,1,1,1));stage=26;last=now
  elif stage==26 and now-last>2:
   check('stock_pallet_inspector_opens',ui(host())['screen']==3);shot('Playtest_PalletContract');action(host(),'StageStock%d_%d'%(pallet,order));stage=27;last=now
  elif stage==27 and now-last>2:N.advance_commerce_test(host(),2000);stage=28;last=now
  elif stage==28 and now-last>2:
   staged=[p for p in report(host())['pallets'] if p['place']==6 and p['order']==order];check('selected_stock_reaches_loading_bay_by_forklift',sum(p['quantity'] for p in staged)==next(o['quantity'] for o in report(host())['orders'] if o['id']==order))
   if not staged:raise RuntimeError('No staged pallets after forklift request')
   pallet=staged[0]['id'];action(host(),'Tablet');stage=29;last=now
  elif stage==29 and now-last>1:action(host(),'Carrier0');stage=30;last=now
  elif stage==30 and now-last>1:action(host(),'Pallet%d'%pallet);stage=31;last=now
  elif stage==31 and now-last>1:action(host(),'Book');stage=32;last=now
  elif stage==32 and now-last>2:
   check('selected_partial_pallet_has_a_booking',any(p['id']==pallet and p['booking']>0 for p in report(host())['pallets']));N.advance_commerce_test(host(),2000);stage=33;last=now
  elif stage==33 and now-last>2:N.advance_commerce_test(host(),2000);stage=34;last=now
  elif stage==34 and now-last>2:
   result['final_report']=report(host());order_state=next(o for o in report(host())['orders'] if o['id']==order);check('carrier_delivers_complete_contract_and_pays',order_state['delivered']==order_state['quantity'] and order_state['paid']>0)
   for i,p in enumerate(peers()):check('peer%d_final_ledger_valid'%i,report(p).get('authoritative_valid',True))
   finish()

 except Exception:result['errors'].append(traceback.format_exc());finish()
handle=unreal.register_slate_post_tick_callback(tick)
