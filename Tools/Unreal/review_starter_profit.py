"""Observe ordinary starter play: never grants cash/stock, advances time or issues actions.

Run in an existing PIE session. Finish with builtins.foundryline_stop_profit_audit().
The report distinguishes cash contribution from profit: retained steel/parts are assets,
and observed window costs include idle payroll and any unrelated business spending.
"""
import builtins
import json
import time
import traceback
from pathlib import Path
import unreal

N = unreal.FoundrylineNetworkTestTools
OUT = Path(unreal.Paths.project_saved_dir()).resolve() / 'Logs/StarterProfitNativeQA.json'
DATA = {'completed': False, 'read_only': True,
        'units': {'money': 'integer cents', 'clock': 'game minutes'},
        'players': {}, 'errors': []}
START = time.monotonic()
LAST = 0.0


def delta(before, after):
    keys = ('cash', 'cost', 'revenue', 'clock', 'wages_paid')
    return {key: after.get(key, 0) - before.get(key, 0) for key in keys}


def save():
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(DATA, indent=2), encoding='utf-8')


def stop():
    DATA['completed'] = True
    save()
    unreal.unregister_slate_post_tick_callback(HANDLE)


def tick(_dt):
    global LAST
    now = time.monotonic() - START
    if now - LAST < 2:
        return
    LAST = now
    try:
        for world in unreal.EditorLevelLibrary.get_pie_worlds(False):
            for player in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.PlayerController):
                if not player.is_local_controller():
                    continue
                key = player.get_path_name()
                report = json.loads(N.commerce_report(player))
                entry = DATA['players'].setdefault(key, {
                    'initial': report, 'events': [], 'completed_order_ids': [],
                    'preexisting_completed_ids': [o['id'] for o in report.get('orders', [])
                        if o.get('delivered', 0) >= o.get('quantity', 1)]})
                previous = entry.get('latest', entry['initial'])
                paid_before = {o['id']: o.get('paid', 0) for o in previous.get('orders', [])}
                for order in report.get('orders', []):
                    paid = order.get('paid', 0) - paid_before.get(order['id'], 0)
                    complete = order.get('delivered', 0) >= order.get('quantity', 1)
                    first_complete = (complete and order['id'] not in entry['completed_order_ids']
                        and order['id'] not in entry['preexisting_completed_ids'])
                    if first_complete:
                        entry['completed_order_ids'].append(order['id'])
                    if paid or first_complete:
                        entry['events'].append({'real_seconds': now, 'order': order,
                            'payment_since_previous_sample': paid,
                            'window_financial_delta': delta(entry['initial'], report)})
                entry['latest'] = report
                entry['window_financial_delta'] = delta(entry['initial'], report)
                entry['two_orders_completed'] = len(entry['completed_order_ids']) >= 2
        save()
    except Exception:
        DATA['errors'].append(traceback.format_exc())
        stop()


HANDLE = unreal.register_slate_post_tick_callback(tick)
builtins.foundryline_stop_profit_audit = stop
