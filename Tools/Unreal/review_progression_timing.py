"""Continue a genuinely earned starter business; no grants or user save writes.
Reports game-time phases, cash and stock-adjusted profit. One order uses wall-clock
8x simulation without AdvanceCommerceTest; fixture travel is reported separately.
"""
from pathlib import Path
import unreal
base=Path(unreal.Paths.project_dir()).resolve()/'Tools/Unreal/review_starter_business_playthrough.py'
source=base.read_text().split('unreal.EditorPythonScripting.set_keep_python_script_alive(True)')[0]
exec(compile(source.replace('START > 600','START > 1800').replace('yield 1','yield .5'),str(base),'exec'))
OUT=ROOT/'Saved/Logs/ProgressionTimingNativeQA.json'
STARTER_FLOW=flow
BASE_CHECK=check
CHECKPOINT_JSON=ROOT/'Saved/Logs/ProgressionEarnedCheckpoint.json'
CHECKPOINT_SLOT='Foundryline_QA_ProgressionEarned'
def check(name,value):
    if not value and player():
        RESULT['failure_pavement']=json.loads(N.pavement_report(player()))
        RESULT['failure_offices']=json.loads(N.office_storey_report(player()))
    BASE_CHECK(name,value)
RESULT['timing_notes']=['One plant second is one game minute. Accelerated ticks are not measured human playtime.', 'Terminal fixture travel is not walking time.', 'Current daily payroll is included when earned, even before midnight; sale prices are unchanged.']
RESULT['extended_jobs']=[]

def employees():return json.loads(N.employee_report(player()))

def checkpoint(label):
    r=report();RESULT[label]=r;OUT.write_text(json.dumps(RESULT,indent=2));return r

def pause():
    if report()['running']:
        factory('Run');yield .5

def phase(job,label,before,wall):
    after=report();job.setdefault('phases',[]).append({'phase':label,'game_minutes':after['clock']-before['clock'],'wall_seconds':time.monotonic()-wall,'cash_delta_cents':after['cash']-before['cash'],'wages_cents':after['wages_paid']-before['wages_paid']});return after

def tick_job(job,real_time=False):
    if real_time:yield .25
    else:run_sim();yield .1

def complete_order(offer,real_time=False,parallel_forming=False):
    oid=offer['id'];part=offer['part'];quantity=offer['quantity'];lb=5 if part in (1,9) else 2
    job={'order':offer,'real_time_8x':real_time,'parallel_forming':parallel_forming,'before':report(),'wall_start':time.monotonic(),'phases':[]};RESULT['extended_jobs'].append(job)
    N.setup_operations_review(player(),1);yield .5;action('Office');yield .5;action('Order%d'%oid);action('Accept');yield .5
    check('extended_%d_accepted'%oid,any(o['id']==oid and o['status']==1 for o in report()['orders']))
    required=max(0,math.ceil((quantity*lb-steel_asset(report())-.0001)/SHEET_LB))
    if required:
        action('BuySheets%d'%required);yield .5;action('BuySteel');yield .5
    job['sheets_bought']=required
    action('Close');N.queue_factory_console(player(),1);yield .5;factory('Interact');yield .5;factory('QueueJob%d'%oid);yield .5
    check('extended_%d_physical_plasma_queued'%oid,ui()['machine']==1 and any(m['queue_count'] or m['remaining'] or m['batch_remaining'] for m in report()['machines'] if m['id']==1))
    if part==9 and parallel_forming:
        factory('CloseComputer');yield .5
        bm=next(m for m in report()['machines'] if m['id']==BRAKE_ID)
        check('parallel_brake_terminal_fixture',N.place_player_collision_review(player(),unreal.Vector(bm['x']+180,bm['y']+445,112),-90))
        yield .5;factory('Interact');yield .5
        check('parallel_brake_physical_terminal_open',ui()['machine']==BRAKE_ID)
        factory('QueueJob%d'%oid);yield .5
    if real_time:factory('Speed8');yield .5
    if not report()['running']:factory('Run');yield .5
    before=report();wall=time.monotonic();deadline=wall+420
    for _ in range(2400):
        stock=[p for p in report()['pallets'] if p['part']==(9 if parallel_forming else 1) if not p['booking'] and not p['reserved']] if part==9 else [p for p in report()['pallets'] if p['part']==part and not p['booking'] and not p['reserved']]
        if sum(p['quantity'] for p in stock)>=quantity:break
        if time.monotonic()>deadline:raise RuntimeError('Production timed out '+str(report()))
        yield from tick_job(job,real_time)
    check('extended_%d_cut_complete'%oid,sum(p['quantity'] for p in stock)>=quantity);phase(job,'pipelined_cutting_and_forming' if parallel_forming else 'steel_delivery_and_cutting',before,wall)
    if part==9 and not parallel_forming:
        factory('CloseComputer');yield .5
        bm=next(m for m in report()['machines'] if m['id']==BRAKE_ID)
        check('brake_terminal_fixture',N.place_player_collision_review(player(),unreal.Vector(bm['x']+180,bm['y']+445,112),-90))
        yield .5;factory('Interact');yield .5
        check('brake_physical_terminal_open',ui()['machine']==BRAKE_ID)
        factory('QueueJob%d'%oid);yield .5
        before=report();wall=time.monotonic()
        for _ in range(180):
            stock=[p for p in report()['pallets'] if p['part']==9 and not p['booking'] and not p['reserved']]
            if sum(p['quantity'] for p in stock)>=quantity:break
            run_sim();yield .1
        check('extended_%d_formed_complete'%oid,sum(p['quantity'] for p in stock)>=quantity);phase(job,'forming_and_blank_transport',before,wall)
    factory('CloseComputer');yield .5;N.setup_operations_review(player(),2);yield .5;action('Tablet');yield .5;action('Order%d'%oid);yield .5
    for pallet in stock:
        action('StageStock%d_%d'%(pallet['id'],oid));yield .5
    before=report();wall=time.monotonic()
    for _ in range(1600 if real_time else 100):
        state=report();ids={s['id'] for s in state['slots'] if s['machine']==0 and s['id']!=100}
        ready=[p for p in state['pallets'] if p['order']==oid and p['place']==1 and p['owner'] in ids and not p['reserved']]
        if sum(p['quantity'] for p in ready)==quantity:break
        yield from tick_job(job,real_time)
    check('extended_%d_finished_goods_ready'%oid,sum(p['quantity'] for p in ready)==quantity);phase(job,'finished_goods_transfer',before,wall)
    yield from pause();action('Page8');yield .5
    for pallet in ready:action('Pallet%d'%pallet['id']);yield .3
    carrier=0 if len(ready)==1 else 1 if len(ready)<=2 else 2
    fee=(2500,4000,9000)[carrier];job['carrier']=carrier;job['pickup_fee_cents']=fee
    action('Carrier%d'%carrier);yield .3;cash=report()['cash'];action('Book');yield .5
    check('extended_%d_pickup_paid_once'%oid,report()['cash']==cash-fee)
    factory('Run');yield .5;before=report();wall=time.monotonic()
    for _ in range(1600 if real_time else 100):
        done=next(o for o in report()['orders'] if o['id']==oid)
        if done['status']==2 and done['delivered']==quantity:break
        yield from tick_job(job,real_time)
    check('extended_%d_delivered'%oid,done['status']==2 and done['delivered']==quantity);phase(job,'carrier_arrival_loading_and_delivery',before,wall);yield from pause()
    after=report();job['after']=after;job['delta']=financial_delta(job['before'],after);job['wall_total_seconds']=time.monotonic()-job['wall_start'];job['stock_adjusted_profit_cents']=job['delta']['cash']-job['delta']['wages_accrued']+round(100*(steel_asset(after)-steel_asset(job['before'])));job['equivalent_sim_seconds_1x']=job['delta']['clock'];job['equivalent_sim_seconds_8x']=job['delta']['clock']/8
    check('extended_%d_valid'%oid,after['authoritative_valid'] and after['cash_matches_registry'] and abs(after['mass_delta'])<.2);checkpoint('latest_extended')

def flow():
    global BRAKE_ID
    if CHECKPOINT_JSON.exists() and unreal.GameplayStatics.does_save_game_exist(CHECKPOINT_SLOT,0):
        LEVELS.load_level('/Game/Foundryline/Maps/L_FirstShopStarter');LEVELS.editor_request_begin_play()
        for _ in range(40):
            yield .5
            if player() and report().get('enabled'):break
        check('earned_checkpoint_restored',N.test_factory_checkpoint(player(),CHECKPOINT_SLOT,True));yield 1
        previous=json.loads(CHECKPOINT_JSON.read_text());RESULT.clear();RESULT.update(previous);RESULT['completed']=False;RESULT['errors']=[];RESULT['payroll_policy']='Daily $120 per employee, paid at midnight; profits deduct accrued wages.'
        N.setup_operations_review(player(),1);yield .5;action('Office');yield .5
    else:
        yield from STARTER_FLOW();RESULT['completed']=False;RESULT['starter_checkpoint_wall_seconds']=time.monotonic()-START
        check('earned_checkpoint_saved',N.test_factory_checkpoint(player(),CHECKPOINT_SLOT,False))
        CHECKPOINT_JSON.write_text(json.dumps(RESULT,indent=2))
    BRAKE_ID=next(m['id'] for m in report()['machines'] if m['kind']==2)
    action('PropertyView');yield 1
    cash=report()['cash'];chosen=None;attempts=[]
    # Use the current owned-equipment planner path, not the retired buy-and-place tool.
    for x,y in [(-1700,1750),(-1800,700),(-1300,1800)]:
        N.queue_construction_tool(player(),unreal.Name('PlannerPlace'),unreal.IntVector(BRAKE_ID,0,0),unreal.IntVector(x,y,0),1,unreal.LinearColor(1,1,1,1));yield 1
        installed=any(s['machine']==BRAKE_ID for s in report()['slots']);attempts.append({'x':x,'y':y,'installed':installed,'ui':ui()})
        if installed:chosen=(x,y);break
    RESULT['owned_brake_placement_attempts']=attempts
    if chosen is None:
        N.queue_construction_tool(player(),unreal.Name('HUD:PlannerCancel'),unreal.IntVector(),unreal.IntVector(),0,unreal.LinearColor(1,1,1,1));yield .5
        N.queue_construction_tool(player(),unreal.Name('Building'),unreal.IntVector(-2400,2200,0),unreal.IntVector(400,3600,0),0,unreal.LinearColor(1,1,1,1));yield 2
        RESULT['expansion_status']=json.loads(N.pavement_report(player()))['status'];chosen=(-1800,2800)
        N.queue_construction_tool(player(),unreal.Name('PlannerPlace'),unreal.IntVector(BRAKE_ID,0,0),unreal.IntVector(*chosen,0),1,unreal.LinearColor(1,1,1,1));yield 1
        RESULT['expansion_cash_cost_cents']=cash-report()['cash']
    check('brake_installed_without_second_purchase',report()['cash']==cash and any(s['machine']==BRAKE_ID for s in report()['slots']))
    N.queue_construction_tool(player(),unreal.Name('HUD:PlannerCancel'),unreal.IntVector(),unreal.IntVector(),0,unreal.LinearColor(1,1,1,1));yield .5
    if chosen==(-1800,2800):
        for a,b in [((-500,1950),(-100,3500)),((-1150,2250),(-100,2650)),((-1150,2900),(-100,3300))]:
            x=unreal.Vector(*a,20);y=unreal.Vector(*b,20);v=json.loads(N.preview_factory_brush(player(),unreal.Name('Forklift'),x,y,True,True));RESULT.setdefault('new_forklift_areas',[]).append(v);yield 1;check('forklift_area_%s'%str(a),v['valid'])
    N.setup_operations_review(player(),1);yield .5;action('Office');yield .5;action('HirePerson');yield 1
    person=next(e for e in employees()['employees'] if e['role']==3);action('AssignEmployee%d_0_%d'%(person['id'],BRAKE_ID));yield 1
    check('brake_staffed',any(e['assignment']==BRAKE_ID and e['role']==0 for e in employees()['employees']));checkpoint('staffed_upgrade')
    # Observe normal 8x wall time, with no accelerated tick calls for this entire order.
    offer=next(o for o in report()['orders'] if o['status']==0 and o['part'] in (1,10) and o['quantity']<=30 and o['payment']!=2)
    yield from complete_order(offer,True)
    factory('Speed1');yield .5
    # Keep selling with the third employee on payroll; test actual generated offers.
    for i in range(3):
        offers=[o for o in report()['orders'] if o['status']==0 and o['part'] in (1,9,10) and o['payment']!=2]
        offer=next((o for o in offers if o['part']==9),offers[0]);yield from complete_order(offer,False)
    # Controlled comparison: identical earned business, offer, inventory and payroll.
    comparison_slot='Foundryline_QA_BrakeComparison'
    comparison_offer=next(o for o in report()['orders'] if o['status']==0 and o['part']==9 and o['quantity']==75 and o['payment']!=2)
    check('comparison_checkpoint_saved',N.test_factory_checkpoint(player(),comparison_slot,False))
    comparison_before=report();yield from complete_order(comparison_offer,False,False)
    sequential=RESULT['extended_jobs'][-1]
    check('comparison_checkpoint_restored',N.test_factory_checkpoint(player(),comparison_slot,True));yield 1
    check('comparison_identical_cash_and_clock',report()['cash']==comparison_before['cash'] and abs(report()['clock']-comparison_before['clock'])<.01)
    yield from complete_order(comparison_offer,False,True)
    parallel=RESULT['extended_jobs'][-1]
    RESULT['brake_comparison']={'same_offer':comparison_offer,'sequential':sequential,'parallel':parallel,'sequential_branch_rolled_back':True}
    check('played_checkpoint_saved',N.test_factory_checkpoint(player(),'Foundryline_QA_ProgressionPlayed',False))
    RESULT['completed']=True;checkpoint('final_extended');
    N.setup_operations_review(player(),1);yield .5;action('Office');yield .5;action('Page7');yield .5;shot('DailyPayroll_Team');yield 1
    action('Page5');yield .5;shot('DailyPayroll_Money');yield 1

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
FLOW=flow();HANDLE=unreal.register_slate_post_tick_callback(tick)
