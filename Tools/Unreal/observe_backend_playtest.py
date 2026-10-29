"""Read-only observer for a separately driven native starter-shop workflow.

Execute before review_first_order.py. It changes no gameplay, camera, input, save,
settings or screenshots. At the end call _backend_playtest_observer.stop().
"""
import unreal as _backend_unreal
import time as _backend_time
import json as _backend_json
import math as _backend_math
from pathlib import Path as _BackendPath

class BackendPlaytestObserver:
    def __init__(self):
        self.start = _backend_time.monotonic()
        self.last = -1
        self.samples = 0
        self.failures = []
        self.places = set()
        self.phases = set()
        self.moving = set()
        self.processing = set()
        self.last_driver = {}
        self.torch_positions = set()
        self.received_lb = False
        self.last_report = {}
        self.motion_samples = []
        self.path = _BackendPath(_backend_unreal.Paths.convert_relative_path_to_full(_backend_unreal.Paths.project_saved_dir())) / 'Logs' / 'BackendPlaytestObserver.json'
        self.handle = _backend_unreal.register_slate_post_tick_callback(self.tick)
        self.write(False)

    def tick(self, dt):
        now = _backend_time.monotonic() - self.start
        if now - self.last < .2:
            return
        self.last = now
        try:
            worlds = _backend_unreal.EditorLevelLibrary.get_pie_worlds(False)
            for world in worlds:
                for pc in _backend_unreal.GameplayStatics.get_all_actors_of_class(world, _backend_unreal.PlayerController):
                    if not pc.is_local_controller() or not pc.has_authority():
                        continue
                    report = _backend_json.loads(_backend_unreal.FoundrylineNetworkTestTools.commerce_report(pc))
                    if not report.get('enabled'):
                        continue
                    self.samples += 1
                    self.last_report = report
                    if not report.get('authoritative_valid', False):
                        self.record('Invalid authoritative state: ' + report.get('validation_error', 'missing validation'))
                    if not report.get('cash_matches_registry', False):
                        self.record('Plant cash differs from its authoritative company registry')
                    if abs(report.get('mass_delta', 0)) > .25:
                        self.record('Material conservation difference exceeded 0.25 lb')
                    for pallet in report.get('pallets', []):
                        self.places.add(pallet['place'])
                        if not (0 <= pallet.get('reserved', 0) <= pallet['quantity']):
                            self.record('Invalid pallet reservation')
                    for driver in report.get('drivers', []):
                        if not all(_backend_math.isfinite(driver[k]) for k in ('x', 'y', 'z')):
                            self.record('Non-finite forklift position')
                        if abs(driver['z'] - 20) > 2:
                            self.record('Forklift root left the raised shop/road surface')
                        key = (report['plant'], driver['id'])
                        at = (driver['x'], driver['y'], driver['z'])
                        before = self.last_driver.get(key)
                        if before and sum((a-b)**2 for a,b in zip(at, before)) > 4:
                            self.moving.add(driver['id'])
                        self.last_driver[key] = at
                    for machine in report.get('machines', []):
                        if machine['remaining'] < 0 or machine['remaining'] > machine['duration'] + .001:
                            self.record('Machine progress outside its recipe duration')
                        if machine['remaining'] > 0 and machine['enabled'] and report.get('running'):
                            self.processing.add(machine['id'])
                    for booking in report.get('bookings', []):
                        self.phases.add(booking['status'])
                    # Existing native presentation probe is unambiguous in a one-plant review.
                    plants = _backend_unreal.GameplayStatics.get_all_actors_of_class(world, _backend_unreal.FoundrylineFactoryPlant)
                    if len(plants) == 1:
                        motion = _backend_json.loads(_backend_unreal.FoundrylineNetworkTestTools.equipment_motion_report(pc))
                        if motion.get('cutting') and motion.get('torch'):
                            self.torch_positions.add(tuple(round(v, 1) for v in motion['torch']))
                        if motion.get('cutting') and len(self.motion_samples) < 8:
                            self.motion_samples.append(motion)
            if int(now) % 2 == 0:
                self.write(False)
        except Exception as exc:
            self.record(type(exc).__name__ + ': ' + str(exc))
            self.write(False)

    def record(self, error):
        if error not in self.failures:
            self.failures.append(error)

    def write(self, completed):
        self.path.write_text(_backend_json.dumps({
            'completed': completed, 'read_only': True, 'samples': self.samples,
            'checks': {
                'observed_authoritative_factory': self.samples > 0,
                'sampled_invariants_hold': self.samples > 0 and not self.failures,
                'forklift_motion_observed': bool(self.moving),
                'machine_processing_observed': bool(self.processing),
                'plasma_torch_motion_observed': len(self.torch_positions) > 1,
            },
            'failures': self.failures, 'pallet_places_seen': sorted(self.places),
            'booking_phases_seen': sorted(self.phases), 'drivers_seen_moving': sorted(self.moving),
            'machines_seen_processing': sorted(self.processing),
            'torch_positions_seen': len(self.torch_positions), 'motion_samples': self.motion_samples,
            'last_report': self.last_report,
        }, indent=2), encoding='utf-8')

    def stop(self):
        if self.handle is not None:
            _backend_unreal.unregister_slate_post_tick_callback(self.handle)
            self.handle = None
        self.write(True)

if '_backend_playtest_observer' in globals():
    _backend_playtest_observer.stop()
_backend_playtest_observer = BackendPlaytestObserver()
