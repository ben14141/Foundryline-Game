"""Fresh unsaved starter business: four real jobs, paid steel, physical terminal, freight.

No free cash, inventory, edited orders, save writes or simulation replacement. Uses
the ordinary queued UI commands and AdvanceCommerceTest (real plant ticks/account
sync). Configure single-player PIE before launch. Leaves the final Inventory open.
"""
import json
import math
import time
import traceback
from pathlib import Path
import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve()
OUT = ROOT / 'Saved/Logs/StarterBusinessNativeQA.json'
SHOTS = ROOT / 'Saved/Screenshots/Reviews'
SHOTS.mkdir(parents=True, exist_ok=True)
N = unreal.FoundrylineNetworkTestTools
LEVELS = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
RESULT = {'completed': False, 'checks': {}, 'jobs': [], 'errors': [],
          'units': {'money': 'integer cents', 'clock': 'game minutes'},
          'grants': 'none', 'advance_steps_per_callback': 200}
START = time.monotonic()
NEXT = START + 3
SHEET_LB = 152.4 * 304.8 * .3175 * .000001 * 7850 / .45359237


def player():
    for world in unreal.EditorLevelLibrary.get_pie_worlds(False):
        if unreal.GameplayStatics.get_game_mode(world):
            return next(p for p in unreal.GameplayStatics.get_all_actors_of_class(
                world, unreal.PlayerController) if p.is_local_controller())
    return None


def report():
    return json.loads(N.commerce_report(player()))


def ui():
    return json.loads(N.operations_ui_report(player()))


def check(name, value):
    RESULT['checks'][name] = bool(value)
    if not value:
        raise RuntimeError(name)


def snapshot(label):
    value = report()
    RESULT[label] = value
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(RESULT, indent=2), encoding='utf-8')
    return value


def action(name):
    check('queued_' + name, N.queue_commerce_action(player(), name))


def factory(name):
    check('queued_factory_' + name, N.queue_factory_action(player(), unreal.Name(name)))


def shot(name):
    unreal.SystemLibrary.execute_console_command(player(),
        'Shot filename="' + str(SHOTS / (name + '.png')) + '"')


def stable_cash():
    return report()['cash'], report()['cost'], len(report()['bookings'])


def steel_asset(state):
    return sum(m['sheet_lb'] for m in state['machines']) + sum(
        p['unit_lb'] * p['quantity'] for p in state['pallets'] if p['part'] == 0)


def financial_delta(before, after):
    return {key: after[key] - before[key] for key in
            ('cash', 'cost', 'revenue', 'wages_paid', 'wages_accrued', 'clock')}


def run_sim():
    check('real_simulation_tick', N.advance_commerce_test(player(), 200))


def flow():
    LEVELS.load_level('/Game/Foundryline/Maps/L_FirstShopStarter')
    LEVELS.editor_request_begin_play()
    for _ in range(40):
        if player() and report().get('enabled'):
            break
        yield 1
    check('single_player_pie', len(unreal.EditorLevelLibrary.get_pie_worlds(False)) == 1)
    initial = snapshot('initial')
    check('fresh_start_4000', initial['cash'] == 400000)
    check('fresh_no_free_inventory', not initial['pallets'] and initial['revenue'] == 0)
    if initial['running']:
        factory('Run')
        yield 1
    check('paused_for_atomic_human_error_checks', not report()['running'])
    check('office_position', N.setup_operations_review(player(), 1))
    yield 1
    action('Office')
    yield 1
    before = stable_cash()
    action('BuyEquipment1')  # $8500 plasma against real $4000 balance.
    yield 1
    check('unaffordable_purchase_no_charge', stable_cash() == before)
    check('unaffordable_purchase_visible_error', bool(ui().get('status')))
    shot('Business_UnaffordablePurchase')
    for index, (part, quantity, pounds) in enumerate(((1, 40, 5), (10, 60, 2), (10, 60, 2), (1, 50, 5))):
        job = {'index': index + 1, 'part': part, 'expected_quantity': quantity}
        RESULT['jobs'].append(job)
        N.setup_operations_review(player(), 1)
        yield 1
        action('Office')
        yield 1
        action('Page0')
        yield 1
        state = report()
        offer = next(o for o in state['orders'] if o['status'] == 0 and o['part'] == part and o['quantity'] == quantity and (index < 2 or o['payment'] != 2))
        check('job%d_actual_offer_quantity' % index, offer['quantity'] == quantity)
        order_id = offer['id']
        job['order'] = offer
        job['before'] = state
        action('Order%d' % order_id)
        yield .5
        action('Accept')
        yield 1
        check('job%d_acceptance_authoritative' % index, next(
            o for o in report()['orders'] if o['id'] == order_id)['status'] == 1)
        shot('Business_Job%d_Contract' % (index + 1))
        before = stable_cash()
        previous_pickup_order = report()['pickup_order']
        action('Assign')
        yield 1
        check('job%d_cannot_stage_unmade_parts' % index,
              report()['pickup_order'] == previous_pickup_order and stable_cash() == before and
              not any(p['place'] == 6 for p in report()['pallets']))
        check('job%d_no_parts_feedback' % index, 'No parts ready' in ui().get('status', ''))
        action('Close')
        check('physical_machine_position', N.queue_factory_console(player(), 1))
        yield 1
        factory('Interact')  # Actual E input, not remote office programming.
        yield 1
        check('job%d_physical_terminal_open' % index, ui()['machine'] == 1)
        factory('QueueJob%d' % order_id)
        yield 1
        check('job%d_program_queued' % index, any(
            m['queue_count'] or m['batch_remaining'] or m['remaining'] for m in report()['machines']))
        shot('Business_Job%d_PlasmaQueue' % (index + 1))
        if index == 0:
            if not report()['running']:
                factory('Run')
                yield 1
            no_steel_before = report()
            run_sim()
            yield .5
            check('empty_steel_wait_does_not_create_parts', not report()['pallets'] and
                  report()['machines'][0]['remaining'] == 0)
            check('empty_steel_wait_actually_runs_and_pays_wages',
                  report()['clock'] > no_steel_before['clock'] and report()['wages_paid'] + report()['wages_accrued'] > no_steel_before['wages_paid'] + no_steel_before['wages_accrued'])
            job['no_steel_wait'] = report()
            factory('Run')
            yield 1
        available = steel_asset(report())
        sheets = max(0, math.ceil((quantity * pounds - available - .0001) / SHEET_LB))
        job['sheets_purchased'] = sheets
        factory('CloseComputer')
        N.setup_operations_review(player(), 1)
        yield 1
        action('Office')
        yield 1
        if sheets:
            action('BuySheets%d' % sheets)
            yield .5
            before = report()
            action('BuySteel')
            yield 1
            after = report()
            check('job%d_real_paid_sheet_delivery' % index,
                  after['cash'] < before['cash'] and len(after['bookings']) == len(before['bookings']) + 1)
            job['steel_receipt_delta'] = financial_delta(before, after)
            shot('Business_Job%d_SteelReceipt' % (index + 1))
        action('Close')
        N.queue_factory_console(player(), 1)
        yield 1
        factory('Interact')
        yield 1
        if not report()['running']:
            factory('Run')
            yield 1
        for attempt in range(120):
            state = report()
            stock = [p for p in state['pallets'] if p['part'] == part and p['place'] == 1
                     and not p['booking'] and not p['reserved']]
            if sum(p['quantity'] for p in stock) >= quantity:
                break
            run_sim()
            yield .2
        check('job%d_actual_parts_produced' % index, sum(p['quantity'] for p in stock) >= quantity)
        check('job%d_output_is_designated_machine_rack' % index, all(any(
            s['id'] == p['owner'] and s['machine'] == 1 and s['output'] for s in state['slots']) for p in stock))
        job['production'] = state
        factory('CloseComputer')
        N.setup_operations_review(player(), 2)
        yield 1
        action('Tablet')
        yield 1
        for pallet in stock:
            action('StageStock%d_%d' % (pallet['id'], order_id))
            yield 1
        for _ in range(80):
            state = report()
            staged = [p for p in state['pallets'] if p['place'] == 1 and p['order'] == order_id and any(s['id'] == p['owner'] and s['machine'] == 0 and s['id'] != 100 for s in state['slots'])]
            if sum(p['quantity'] for p in staged) == quantity:
                break
            run_sim()
            yield .2
        check('job%d_forklift_stored_exact_order_in_finished_goods' % index,
              sum(p['quantity'] for p in staged) == quantity)
        check('job%d_smallest_carrier_van_fits' % index,
              len(staged) == 1 and sum(p['quantity'] * p['unit_lb'] + 60 for p in staged) <= 900)
        factory('Run')  # Pause so failed/duplicate booking cash checks exclude wages.
        yield 1
        action('Carrier0')
        yield .5
        before = stable_cash()
        action('Book')
        yield 1
        check('job%d_empty_manifest_rejected_no_charge' % index, stable_cash() == before)
        check('job%d_empty_manifest_visible_error' % index, bool(ui().get('status')))
        action('Pallet%d' % staged[0]['id'])
        yield .5
        action('Book')
        yield 1
        booked = report()
        check('job%d_real_van_booking_fee' % index, booked['cash'] == before[0] - 2500)
        check('job%d_real_manifest_quantity' % index, any(
            not b['inbound'] and b['order'] == order_id and
            sum(m['quantity'] for m in b['manifest']) == quantity for b in booked['bookings']))
        action('Pallet%d' % staged[0]['id'])  # Booked stock cannot be selected again.
        yield .5
        before = stable_cash()
        action('Book')
        yield 1
        check('job%d_duplicate_booking_rejected_no_charge' % index, stable_cash() == before)
        factory('Run')
        yield 1
        for _ in range(80):
            state = report()
            done = next(o for o in state['orders'] if o['id'] == order_id)
            if done['delivered'] == quantity and done['status'] == 2 and done['paid'] > 0:
                break
            run_sim()
            yield .2
        check('job%d_delivery_and_customer_payment' % index,
              done['delivered'] == quantity and done['status'] == 2 and done['paid'] > 0)
        factory('Run')
        yield 1
        job['after'] = report()
        job['financial_delta'] = financial_delta(job['before'], job['after'])
        job['remaining_raw_steel_lb'] = steel_asset(job['after'])
        job['remaining_raw_steel_asset_cents_at_purchase_rate'] = round(
            job['remaining_raw_steel_lb'] * 100)
        check('job%d_ledger_valid' % index, job['after'].get('authoritative_valid') is True)
        check('job%d_mass_conserved' % index, abs(job['after'].get('mass_delta', 999)) < .2)
        check('job%d_cash_matches_company' % index, job['after'].get('cash_matches_registry') is True)
        snapshot('after_job%d' % (index + 1))
    N.setup_operations_review(player(), 1)
    yield 1
    action('Office')
    yield 1
    action('Page11')
    yield 1
    shot('Business_FourJobs_FinalInventory')
    final = snapshot('final')
    RESULT['whole_run_delta'] = financial_delta(initial, final)
    check('four_jobs_done_more_orders_available', len(RESULT['jobs']) == 4 and
          any(o['status'] == 0 and o['part'] in (1, 10) for o in final['orders']))
    check('business_positive_cash_survives', final['cash'] > 0)
    RESULT['cash_gain_positive'] = final['cash'] > initial['cash']
    check('four_jobs_fund_brake_with_1000_reserve', final['cash'] - final['wages_accrued'] >= 550000)
    action('Page10')
    yield 1
    action('Workspace1')
    yield 1
    before_upgrade = report()
    action('BuyEquipment2')
    yield 1
    upgraded = snapshot('after_brake_purchase')
    check('earned_cash_buys_press_brake_once', upgraded['cash'] == before_upgrade['cash'] - 450000)
    check('purchased_brake_waits_for_planning_placement',
          sum(m['kind'] == 2 for m in upgraded['machines']) == 1 and
          not any(s['machine'] == 2 for s in upgraded['slots']))
    check('upgrade_preserves_ledger_and_cash', upgraded['authoritative_valid'] and upgraded['cash_matches_registry'])
    shot('Business_EarnedPressBrakePurchase')
    yield 1  # Let the queued screenshot finish before an unattended editor exits.
    RESULT['completed'] = True


def tick(_dt):
    global NEXT
    if time.monotonic() < NEXT:
        return
    try:
        if time.monotonic() - START > 600:
            raise TimeoutError('Starter business playthrough exceeded 600 real seconds')
        NEXT = time.monotonic() + next(FLOW)
    except StopIteration:
        OUT.write_text(json.dumps(RESULT, indent=2), encoding='utf-8')
        unreal.unregister_slate_post_tick_callback(HANDLE)
        if '-StarterReviewQuit' in unreal.SystemLibrary.get_command_line():
            unreal.SystemLibrary.execute_console_command(player(), 'QUIT_EDITOR')
    except Exception:
        RESULT['errors'].append(traceback.format_exc())
        if player():
            RESULT['failure_report'] = report()
            RESULT['failure_ui'] = ui()
        OUT.parent.mkdir(parents=True, exist_ok=True)
        OUT.write_text(json.dumps(RESULT, indent=2), encoding='utf-8')
        unreal.unregister_slate_post_tick_callback(HANDLE)
        if '-StarterReviewQuit' in unreal.SystemLibrary.get_command_line():
            unreal.SystemLibrary.execute_console_command(player(), 'QUIT_EDITOR')


unreal.EditorPythonScripting.set_keep_python_script_alive(True)
FLOW = flow()
HANDLE = unreal.register_slate_post_tick_callback(tick)
