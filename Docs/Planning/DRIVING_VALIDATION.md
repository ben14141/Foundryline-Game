# Driving foundation validation - 2026-10-26

Project: C:/Users/curly/Documents/Foundryline Game. Unreal Engine 5.8.
No vendor assets, maps or existing user saves were changed by these tests.

## Compiled/runtime evidence

- FoundrylineEditor Win64 Development compiled successfully. Existing engine API
  deprecation and HUD float conversion warnings remain.
- Saved/Logs/DrivingFinalAutomation.log: all 27 Foundryline automation cases passed,
  exit 0, including existing construction, editing, save, facade, lighting and walk
  cases plus LayoutAndZones, KinematicRoutes and StarterRoutes.
- Starter-route checks use the actual seeded property and actual loading opening:
  street -> driveway -> car bay; reverse semi parking; aligned reverse loading;
  full semi street arrival -> backing to loading; loading -> street departure.
- Example total route-query times in that headless run: car arrival 1388 ms,
  semi arrival 3011 ms, departure 3425 ms. These are bounded asynchronous planning
  queries, not frame times or a fleet throughput/60 FPS benchmark.
- Synthetic checks reject disconnected paving, missing truck permission and
  jackknifing; demonstrate articulated offtracking and verify returned swept poses.
- Parking targets reject a second claimant and only the owner can release a bay.

## Network and persistence

Saved/Logs/DrivingNetwork.json and DrivingNetwork.log: completed, 21/21 checks
passed, zero script exceptions. Four initial local PIE players plus fifth late
join, NullRHI, one process. Client brush actions use native tick dispatch so they
exercise the actual owning-controller RPC rather than Python-local callspace.

Verified: hidden overlay default; only one player's overlay/edit state changes;
client paint and partial erase; matching shared property and driving inventories;
no mesh actor recreation for zone-only changes; host async save completes; host
load restores permissions and peers converge; public road access needs no zone;
client brush cannot modify public street; fifth player receives zones with their
own overlay hidden. One unique DrivingQA save was deleted after QA.

## Visual check

Rendered single PIE client with a background server checked the branded sidebar,
12 car bays / 7 freight/loading targets, parking entrance and local overlay toggle.
The first review found pavement-depth flicker in world overlay lines; the overlay
render layer was corrected and recompiled. Final rendered review verified readable hatching, parking-bay outlines and entrance
labels without pavement flicker. Overlay hide returned to the regular construction
view. A manually selected route exhausted the bounded search and displayed its
failure message; the planner does not guarantee a route for every entry/bay pair.
Scripts: Tools/Unreal/validate_driving_network.py and review_logistics_ui.py.

## Scope remaining

This validates static layout, route-check previews and routing metadata transport.
It does not validate real vehicle assets, continuous networked driving, dynamic
obstacles/parked cars, collision avoidance, junction queues, delivery scheduling,
route repair, dedicated/remote server latency or town/fleet performance. Those
systems must consume server-owned routes/targets without rebuilding construction.
