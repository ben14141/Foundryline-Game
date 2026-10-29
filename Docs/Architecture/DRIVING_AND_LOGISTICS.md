# Driving surfaces and logistics overlay

Game-owned source: Public/Pathing, Private/Pathing, Private/Logistics and Private/UI.
Units are centimetres. This is the driving-layout foundation and route-check preview;
it does not spawn deliveries or implement a live traffic simulation.

## Guided trip workflow

The local logistics view now shows a numbered five-step sidebar and translucent
map fills instead of the old developer line overlay. Truck access is green, car
parking is blue, and loading/staging markers are amber. Entrances are derived from
real driveway/Truck Access contacts with the public street, with correctly oriented
street-lane approach and departure poses. Numbered door labels sit at the actual
loading openings; choosing a destination also highlights its parking/staging bay.

1. Select a numbered property entrance.
2. Paint/erase the connected paving on which trucks are allowed. Cars skip painting.
3. Choose a loading door, staging bay or (in Car Parking mode) parking space.
4. Select an exit gate. A two-way gate may serve both directions.
5. Test the whole trip. Arrival AND departure must pass before the preview plays.

The sidebar shows missing/selected steps and separate arrival/exit results. The
map draws directional arrival/departure routes only after the respective path
checks pass. A failed leg highlights its destination/exit with a widening/access
message; it does not claim to identify the exact geometric obstruction when the
bounded planner exhausts its budget. Solid cab/trailer silhouettes follow the
checked articulated poses, pause at the destination and leave. Pause/restart
controls and independent Truck Access / Parking layer toggles are provided.
Departure begins at the exact final arrival pose, so nothing teleports at loading.

Trip assignments, checks, playback and layers are local planning drafts. They do
not schedule deliveries and are not saved as operational fleet routes. The footer
states that boundary explicitly. Truck permission zones remain shared property
metadata and can be saved by the host using Save Property & Truck Access. Changing
shared paving/permissions invalidates a checked trip; missing gates/targets are
cleared. Changing one player's draft never changes another player's view or trip.

Painting continues through the existing validated construction RPC. Road, parking
and loading data come from the same immutable map used by the planner. HUD clicks
(including the top-right controls) are excluded from world selection/brush input.
Gate geometry is cached per placement revision rather than checked every frame.

## Geometry and paths

The parking renderer and navigation use the same stall/aisle/entrance layout.
Real driveway contacts remove stalls across the entrance. Car stalls are 2.5 x
5.5 m; central rows require room for circulation and end access. Raised planting
islands are blocked. Freight bays are 4 x 22 m, with separate manoeuvring apron.
The starter site now has a 56 x 36 m freight yard and a 24 m truck approach.

Car profile: 4.5 x 1.8 m, rear-axle turning radius 4 m, 15 cm clearance.
Semi profile: 2.55 m width, approximately 19.7 m total aligned length, 10 m
tractor rear-axle turning radius, articulated trailer and 20 cm clearance.
The prototype limits articulation to 70 degrees. These are configurable C++
profiles; vehicle-specific steering/wheelbase data will be needed for real assets.

Hybrid A* tracks tractor heading, trailer heading and gear, with forward/reverse
motion and sampled swept-body checks. Tangent/staged backing connections reduce
search for freight approaches. Both vehicle rectangles and offtracking must stay
on permitted pavement; final parking must fit its bay and loading must align the
trailer rear with the actual door. The road model uses right-hand lanes, with
junction envelopes permitting wide turns. Reversing is limited to parking/yards.

Queries use immutable layout snapshots on a worker thread, at most one pending
query per player. Each core search is bounded to 40,000 expansions/3 seconds;
multiphase queries have a 12-second overall deadline. Results report total elapsed
time, including unsuccessful approach searches. The coarse navigation guide has
40,000 cells; this is a starter-site implementation, not a whole-town scalability
or 60 FPS acceptance result. Spatial partitioning and shared route caches remain
necessary before a large fleet uses this planner.

## Authority, persistence and moving-object boundary

Truck rectangles are additive optional placement-record metadata (save schema 2),
replicated through existing server-owned property records. SetTruckZones is a
validated action: paved definition, owner/plant checks, contained positive bounds,
1 m snapping and at most 64 rectangles per object. An atomic zone brush may affect
up to 128 records; ordinary construction retains its narrow request limits.

Metadata-only zone edits publish shared records without rebuilding construction
meshes, lighting or grass. Save/load remains host-owned. Local overlay/camera
preferences are not replicated or stored as shared property settings.

Stable parking/loading target IDs and authority-owned target claims prevent two
future vehicle agents taking the same target. Claims are currently transient;
they are not a moving-car obstacle system. Empty bays remain usable for manoeuvres.
There are no live parked-car collision footprints, dynamic avoidance, intersection
queues, delivery scheduling, continuous vehicle replication or route repair after
layout edits yet. Those belong to a separate bounded moving-object simulation,
not construction rebuilds. The route preview proves static surface geometry only.

## Local visual refinements

A five-segment progress strip and explicit step titles indicate the current task.
Matching gates and destinations brighten under the cursor with a pulsing ring and
click prompt; the selected destination retains a halo. Hover thresholds match the
existing gate/destination picker. Both Paths / Arrival / Departure filters reduce
route overlap without rerunning paths, changing geometry or restarting playback.
The map title states which paths are visible. These preferences are local controller
fields and never enter the server construction request or save data.


## Shared node routes and traffic coordination (2026-10-26)

The logistics sidebar now has Routes / Trips / Traffic tabs. Routes draws directed
polylines on owned paving. Click existing nodes to branch or reconnect. Select,
move, unlink, delete, forward/backing and one-way/two-way tools are available.
Node settings include turn radius, Fit Turn (10 m semi minimum) and a waiting-area
marker. START / FINISH choose a local check; newly drawn chains set endpoints for
convenience. Right-click finishes drawing. Local vehicle choice and swept-clearance
visibility do not change another player's view. Trips retains the earlier guided
entrance / access / destination / exit check. Node checks are independent of that
automatic guided trip; they do not silently replace its selected arrival/exit.

RouteNodes are optional save-schema-2 metadata on owned paving, with stable GUIDs,
local 1 m coordinates and outgoing directed links. SetRouteNodes uses the same
owning-controller RPC, trusted company/plant context, revision and atomic batching
as construction. Limits are 64 nodes per surface, 8 outgoing links per node and
4096 nodes in the property snapshot. Validation rejects foreign references,
duplicate IDs, non-paved hosts, out-of-bounds coordinates and excessive payloads.
Deleting a node removes incident links; demolishing paving also prunes incoming
links on surviving paving records. Duplicating paving starts without route nodes.
Moving paving carries its local nodes; moving an individual node preserves its
GUID and connections. Metadata edits publish records without rebuilding meshes.
Host property saves include the graph; client previews are never saved as jobs.

Checks follow the chosen directed path with circular tangent fillets and the
vehicle's physical minimum rear-axle radius. Adjacent fillets must fit their link;
invalid turns never shrink the vehicle or silently bypass the authored nodes.
Every 25 cm sample validates cab and articulated trailer against allowed pavement,
truck permissions, islands, right-hand road lanes and articulation limits. Backing
stays on yards/parking. Gear changes require an aligned reversal. A second 50 cm
margin check distinguishes clear and tight routes. Playback uses checked poses,
4 m/s forward and 1 m/s backing, and shows the cab/trailer swept envelope. A route
starts with an aligned trailer at its first node; it is not proof of a transition
from an arbitrary preceding moving-vehicle pose. Full-trip docking is still the
separate guided-trip check. Failed routes show their checked prefix and a reason.

FFoundrylineTrafficLedger is a bounded, non-ticking coordinator consumed by the
native server-only world subsystem. A future fleet scheduler supplies an already
checked movement From -> Via -> To and measured downstream free space. A grant
atomically claims the junction and adjoining links. Complete vehicle length plus
3 m headway must fit downstream. Opposing movements, shared nodes, nearby swept
corridors and crossing links conflict, including crossings lacking a common node.
Conflicting ready arrivals use FIFO priority; a waiter whose exit is blocked does
not prevent another ready movement. Waiting requests expire after 30 s without a
refresh. Active claims NEVER expire under a vehicle: the scheduler releases them
only after the rear clears. Graph revision changes block new entries until old
claims clear. Clients cannot grant or release right-of-way. The coordinator does
not accept client RPCs and is separate from construction and property replication.

Dijkstra accepts immutable node wait estimates (active reservations and queued
requests) to prefer less congested directed branches. Geometry still has to pass;
a rejected geometric route is not automatically repaired into another layout.
The Traffic tab explains the rules and offers an explicitly local synthetic
rehearsal. This is a tested coordination foundation, not an operational fleet:
there are no spawned delivery agents, actual dynamic downstream occupancy sensors,
automated staging queues, continuous vehicle replication, live congestion heatmap
or delivery scheduling yet. Waiting markers are authored planning metadata. Fleet
integration must measure occupancy, check actual initial articulation, preserve
rear-clear safety through stops/disconnects, and replicate relevant vehicle state.
Large-town throughput and 60 FPS remain unmeasured.
