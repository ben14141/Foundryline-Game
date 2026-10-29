# Factory loop verification - 2026-10-27

Development editor build passed on Unreal Engine 5.8. The full Foundryline
automation run passed 57 tests, including fixed recipe timing, queued programs,
material conservation, four-driver completion, in-flight serialization,
part-sized bay layout and imperial display conversion.

Evidence: `Saved/Logs/FactoryIntegrationBuild.log` and `FactoryFullSuite.log`.

The live network review passed all recorded checks with four players, then a
fifth late joiner. It exercised client orders, shared pause and completion,
host-only load, exact restoration of saved in-flight stock and cargo, collection,
and simultaneous first-person and top-down views. Evidence:
`Saved/Logs/FactoryNetworkQA.json` and `FactoryNetworkEditor.log`.

The rendered review captures machine computers, requirements, partial and finished
tankers, first-person E interaction, and colored floor zones under
`Saved/Screenshots/Reviews`. Its machine/menu assertions are recorded in
`Saved/Logs/FactoryVisualQA.json`. Null-RHI test results alone are not visual evidence.

Prototype limits: workers use temporary proxies. The starter shared forklift
corridor allows one loaded trip and its return at a time, rotating waiting drivers.
Independent simultaneous corridor reservations, commercial purchasing/payment,
production-scale player limits and 60 FPS acceptance remain future work.

## Work-zone preset follow-up

`WorkZonePresetBuild.log` records a successful development build. The full suite
now passes 59 tests (`WorkZonePresetTests.log`), including all 24 combinations of
three preset sizes, four orientations and two delivery sides. These check exact
physical footprints, non-overlapping attached bays, automatic supply-route linking,
rejected edits without mutation and save/load. A separate rotated Spacious zone
with opposite-side deliveries completes the full tanker recipe with conserved stock.

`WorkZonePresetNetworkQA.json` records passing four-player and fifth late-join
checks, including a client creating a preset, the shared automatic supply link and
rejection of a second overlapping client placement. Mixed camera views, production,
in-flight host save/load and collection also pass with the new Compact starter.

The disposable placement UI review never writes the saved map or plant save slot.
Its output is `WorkZonePresetVisualQA.json`; screenshots are under
`Saved/Screenshots/Reviews/Factory_WorkZone_Presets.png`.

## Organized workshop and machine racking

The 52 x 50 metre test shop replaces the cramped 36 x 30 metre layout. It uses
the Spacious tanker zone, two loading doors, an attached office, three spaced
machine cells and seven physical racks with twenty-one shelves. Machine racks
stand inboard of the equipment so the tall exterior walls do not hide them.

`WorkshopRebuildBuild.log` records the development build. `WorkshopFullSuite.log`
records 63 passing automation tests. New tests check every rack/delivery position
for actual navigation access, all fixed storage clear of the assembly footprint,
the shop slab beneath every rack, and real inventory through all thirteen assigned
positions during a complete tanker build. Full production, four-driver completion,
material conservation, fixed recipes, queued jobs and in-flight save/load pass.

`WorkshopNetworkQA.json` records four live peers and a fifth late joiner, with
owned client commands, preset placement/overlap rejection, shared rack stock,
production, collection, mixed camera views and exact in-flight save/load recovery.
`WorkshopVisualQA.json` records rendered machine-computer interaction, seven rack
frames/twenty-one shelves, finished tanker and the reset starter saved ready for use.
Images use the `Workshop_` prefix under `Saved/Screenshots/Reviews`.

The pre-redesign save is retained under `Saved/Backups/BeforeWorkshopRebuild`.
The reorganized layout uses factory save version 2. Forklift travel still uses
the conservative shared-trip lease; larger independent traffic reservations and
production-scale performance measurements are not established by these checks.

Follow-up acceptance includes connected forklift junction paint, automatically
derived pedestrian crossings and people yielding before entering. New geometry
and simulation tests cover boundary clipping, retaining outside edges, duplicate
crossing suppression, lane-edit invalidation and crossing after a forklift clears.
Scene checks also verify physical rack collision, the four-metre tanker exit free
of equipment/racks, and the full exit/steel-sheet envelope fitting its loading door.

Parked forklifts now use four marked bays off the main aisle and clear of the
tanker exit. Path endpoint selection skips occupied lane nodes rather than
rejecting a reachable rack. The four-driver complete-production regression
passes with both changes; a dedicated endpoint test retains the original
parked-position case.


## Stocked practice shop

The practice actor seeds real inventory in all thirteen assigned shelf positions,
all six linked tanker supply bays and each machine output pallet. The ordinary
raw-stock simulation seed remains available for recipe/production regression
checks. Starter parts use fixed catalog steel weights and participate in normal
reservations, forklift jobs, consumption and save/load; loading a save does not
replenish them. Long stock now follows the shelf length instead of crossing it.

WorkshopStockBuild.log records the compiled update and WorkshopStockTests.log
records 64 passing tests, including complete stocked production with conserved
steel mass. WorkshopStockNetworkQA.json and WorkshopStockVisualQA.json provide
co-op and rendered acceptance for the starter stock. Stocked_ screenshots show
racks, output pallets and work-zone staging before any stock is consumed.

## Dock-connected tanker bay and internal parts areas

The largest starter zone now touches the east loading-door threshold, with the
actual tanker mesh directly inside and a tractor-only pickup vehicle centred
outside. Machine cells/racks move to the west bank; the receiving door remains
separate. All preset Min/Max bounds now enclose their parts strip, so the orange
work-zone footprint includes all six blue delivery bays. Product placement,
hologram preview, worker position, physical collision and path exclusion share
the same product centre, including rotation and supply-side changes.

DocksideZoneBuild.log and DocksideZoneTests.log record compilation and 65 passing
tests. Coverage includes actual product mesh bounds near the opening, wall contact,
full pallet-bay containment for every preset/rotation/side, complete production,
four-driver traffic, and tractor-only collision/driving without a phantom trailer.
DocksideZoneNetworkQA.json and DocksideZoneVisualQA.json record co-op and rendering
acceptance. The layout uses save version 3; the version 2 stocked save is preserved
under Saved/Backups/BeforeDocksideZone. Physical product-to-truck transfer remains
separate from these verified layout and access changes.
Machine-cell orange outlines and fills also enclose their input/output racks and output pallet pads; containment is checked alongside the tanker supply bays.

## Resized former large preset

Preset 2 is now labelled Tanker bay and measures 60 ft x 30 ft overall, replacing
80 ft x 68 ft. Its product area is 60 ft x 16 ft with a 14 ft internal parts strip.
The actual trailer mesh, collision dimensions and loading-door alignment remain
unchanged. The welder stands on the opposite side of the tank from supplies,
with the full body radius inside the smaller perimeter. A connected forklift
branch reaches the closer parts bays. Other presets retain their dimensions.

TankerPresetResizeBuild.log records compilation; TankerPresetResizeTests.log
records 65 passing tests, including full production, four-driver completion,
rotated layouts, bay containment, unchanged actual product position and welder
clearance. TankerPresetResizeNetworkQA.json records four peers plus a fifth late
joiner. TankerPresetResizeVisualQA.json and TankerResize_ screenshots record
rendered acceptance and a fresh stocked starter saved ready for testing. The
previous version 3 save is backed up under Saved/Backups/BeforeTankerPresetResize;
the revised geometry uses save version 4.

## Standard MIG equipment and two tanker bays

TwinWorkZonesBuild.log records the compiled update. TwinWorkZonesTests.log records
66 passing tests including both products completing from independent zone buffers,
conserved stock/steel mass, mirrored drop-off access, all four requested loading-wall
openings, standard welder containment for all preset rotations/sides, physical
machine collision and exact gun-tip effect attachment. Idle-zone stock is excluded
from the active-zone global demand calculation, preventing a two-zone supply stall.

TwinWorkZonesNetworkQA.json covers four live peers plus a fifth late joiner, two
standard machines per peer, shared starter stock, validated client placement/order,
save/load and mixed camera views. TwinWorkZonesVisualQA.json and TwinZones_ images
cover rendered carts, gun/lead presentation, active welding, both completed products,
the loading wall, and a fresh stocked layout saved for playtesting. Cable components
use movable attachment consistently with the plant root. Welding retains MIG spark
motion and light intensity while deriving local activity from replicated production;
distant overview views still cull the effect. The old version 4 save is preserved
under Saved/Backups/BeforeTwinWorkZones and the revised layout uses version 5.


## Surface floor paint and clear forklift parking - 2026-10-27

Replaced raised cube markings and floating TextRender stencils with one shallow,
matte deferred decal at the actual slab top (20 cm). The world-coordinate atlas
contains the existing lane junction clipping, crossings, work-zone hatching,
part-sized supply bays, labels and parking markings. It rebuilds only after layout
changes and uses mipmaps. GPU resources are omitted on dedicated/null-RHI runs.
The optional HUD retains labels and its legend; its depth-ignorant floor triangles
and raised selection debug boxes were removed. Removed the duplicate legacy
receiving-guide actor from the workshop startup.

Four 200 x 500 cm parking stalls now occupy clear portions of the strip between
the main forklift aisle and personnel walkway. Their painted bounds include the
forks, rather than just the body origin. Parked trucks align along the stall.
Loading an old authored parking position relocates that home, preserving work
orders and inventory; idle trucks move with their stall. Existing version-5 saves
remain compatible, with a pre-change backup in Saved/Backups/BeforeFloorPaint.

Validation: native editor build succeeded; 67/67 Foundryline tests passed,
including parking stall separation, slab containment, lane/equipment/zone
clearance, and access to the main aisle with all four forklifts present.
Rendered and multiplayer evidence is recorded separately in
Saved/Logs/FloorPaintVisualQA.json and FloorPaintNetworkQA.json.

Four simultaneous rendered peers passed the shared-state factory exercise,
including a surface-paint component on each peer, client placement, overlap
rejection, production, in-flight save/load, mixed walking/top-down views, and an
exact-state fifth late join. These checks do not constitute an FPS benchmark.


## Accurate floor brush and placement grid - 2026-10-27

Factory painting now intersects the actual slab at Z=20 cm, independently of the
construction grid. Removed the additional 25 cm rounding on authoritative lane
placement, which previously moved the committed paint away from the preview.

A local, shallow decal grid uses the same world origin as brush snapping. Filled
blue/red footprint previews respect depth and show the actual dimensions of the
work zone, lane or fixed-size crossing stamp. The crossing preview now matches
its actual 200 x 500 cm footprint. Zone previews still include the product and
welder holograms. No guides or preview actors replicate or reserve shared space.

Controls: grid button or Tab cycles 6 inches / 1 foot / 2 feet; Shift temporarily
uses 1 inch. G toggles the grid. Edge snapping aligns neighboring zones, walls,
lane centers and endpoints; zone spacing cycles touching / 6 inches / 1 foot.
Alt + mouse wheel adjusts lane width. Drag and release paints lanes; crossings
are single-click stamps. Right-click or Escape cancels. Live footprint dimensions
and a validity message stay in the sidebar. Route administration is shown when
no paint brush is active. The server still validates every command, including
simultaneous overlaps. Touching zone boundaries are legal; real overlaps remain
invalid. Removed the invisible right-side HUD exclusion when the overlay is off.

Native build and all 68 tests passed, including endpoint alignment against
non-grid edges. Test and native visual evidence live in
Saved/Logs/FloorGridTests.log and FloorGridVisualQA.json. Four-player co-op including client painting, save/load,
mixed views and a fifth late join passed in FloorGridNetworkQA.json.

The decal material generator asserts every graph connection, including its UV
transform and component masks. Grid lines repeat at the selected physical snap
spacing. Footprint borders use a separate 1024-pixel texture for close placement.
The final visual review runs standalone; four-player test settings are restored
before reopening the normal scene. Initial review attempts exposed a leftover
multi-client configuration and a screenshot-resize cursor shift; the final
review uses normal viewport screenshots and checks actual visible guide bounds.

Final acceptance: FloorGridVisualQA.json completed with every check true and no
errors. Native checks verified visible preview alignment at the neighboring
zone edge, physical grid scaling, exact committed adjacency, lane validity,
crossing size, and restoration of the normal two-bay scene. The material graph
inspection confirms connected UV-transform and mask inputs. Final screenshots
use the normal Shot SHOWUI path, avoiding viewport resize during cursor-driven
preview capture. FloorGrid_Zone_Edge_Snap00000.png shows the final grid and filled
preview; FloorPaintVisualQA.json also verifies all full forklift meshes fit their
separate parking paint. No FPS gain or packaged-build acceptance is claimed.


## Solid personnel lanes and exclusive work zones - 2026-10-27

Personnel routes now have solid green painted boundaries. They use the same
outside-probe clipping as forklift routes: internal edges disappear at straight
extensions, corners and T/cross junctions. Both colors open at personnel/forklift
crossing mouths, and white stripes derive automatically from the intersection.
Adjacent crossing fragments merge when their union remains rectangular, rather
than dropping half a crossing at a split road section. Personnel labels avoid
the crossing center.

Only the orange diagonal fill stripes inside work zones increased from 3 to
8 cm; the 12 cm outer border, machine-cell hatching and blue supply-bay paint
retain their widths. New work zones cannot cover any floor lane or crossing,
and lane/crossing painting cannot cover an existing work zone. Touching edges
remain legal. These rules run in the shared model used by client previews and
the authoritative server, and saved layouts cannot bypass them.

Starter forklift branches moved from Y=2500/3500 to 2550/3450 cm. The two short
personnel branches end at X=941.2 cm, the work-zone boundary. Employees can enter
clear work-zone floor from the walkway without requiring a painted route through
the reserved zone; product and welder collision restrictions remain in place.
Loading the four exact old authored starter lane records migrates their positions
without resetting inventory or jobs. Other overlapping saved layouts are rejected
without replacing the active property. Pre-change data is preserved in
Saved/Backups/BeforeSolidPersonnel.

Validation: native editor build and 70/70 automation tests passed. Two old
geometry/production fixtures were updated to use a clear test floor or routes
around the work-zone perimeter rather than illegal lanes across its interior.
Tests cover personnel T/cross/straight joins, merged zebra crossings, mutual
lane/work-zone exclusions, starter clearance, employee access and rotated
production. Four-player gameplay/network QA without a renderer passed all 23
checks, including server rejection of a client's work zone over a personnel
lane, full production, save/load, mixed views and a fifth late join. It is not
a rendering or FPS test. Evidence: PersonnelLanesBuild.log,
PersonnelLanesTests.log, PersonnelLanesNetworkQA.json, and the separate native
render review PersonnelLanesVisualQA.json under Saved/Logs.

Final native render acceptance: PersonnelLanesVisualQA.json completed with all
four checks true and no errors; both stocked work zones remain and the normal
layout saved successfully. Reviewed PersonnelLanes_Solid_People_And_Work_Fill00000.png
and PersonnelLanes_Automatic_Crossing00000.png: solid green T joins are open,
white crossing stripes interrupt both lane boundary colors, and the wider orange
hatching retains the original outer work-zone borders. Editor left open in the
normal standalone, paused two-bay practice scene.


## Forklift diagonal routes and rounded corners - 2026-10-27

Forklift routing uses eight-direction A* with distance-weighted diagonal steps
and an octile heuristic. A bounded visibility pass removes unnecessary grid
zigzags; clear bends receive checked quadratic arc segments. Every shortcut
respects continuous coverage by connected forklift lanes, clearance from static
equipment, reserved work zones and other trucks, and one-way direction rules.
Analytic segment/box tests catch thin obstacles and small pavement gaps. Endpoint
snapping uses the same full routing-footprint checks as subsequent path edges,
so docking connectors do not select an unusable concave-corner node. Docking and
parking spurs remain separate from the smoothed lane route.

Vehicles slow on approach to tight changes in heading. Rear wheels visibly steer
against the turn, matching the existing rear-steer forklift layout. Yield retries
now install a route instead of changing position by a 50 cm jump. Travel checks
the movement segment, rather than only its endpoint, for vehicle/person spacing.
Routes and progress remain authoritative and replicated; saved route arrays
retain their existing format. This remains the prototype fleet/lease model,
not a new vehicle-physics or unrestricted multi-vehicle dispatch system.

Validation: editor build and 73/73 tests passed. New tests cover diagonal distance,
rounded elbows, routing-footprint containment, machinery clearance, narrow gaps,
one-way direction including short connectors, parked trucks, and continuity
during yielding. Existing tests confirm all machine racks/drop bays remain
reachable and raw-material/stocked/rotated production still completes. An initial
docking-corner regression was caught and fixed before final acceptance.
Four-player network QA without a renderer passed all 25 checks, including
observed diagonal/rounded authoritative paths, full production, exact in-flight
save/load, mixed views and a fifth late join. Native render evidence is separate
in ForkliftRoutingVisualQA.json. No FPS gain is claimed. Saved pre-change state
and editor settings are preserved in Saved/Backups/BeforeForkliftRouting.

Final native render review completed with all four checks true: live diagonal
paths, live rounded paths, visible rear-wheel steering, and restoration of the
fresh paused two-bay scene with 29 pallets. Reviewed the native screenshot
ForkliftRouting_Rounded_Corner00000.png. ForkliftRoutingVisualQA.json reports no
errors. The normal standalone editor remains open for play testing, with the
fresh layout saved. Docking connectors intentionally remain precise terminal
approaches rather than unrestricted shortcuts through reserved work zones.

## Equipment production motion - 2026-10-27

Plasma now traverses, lowers, follows the selected part outline, retracts and
parks with its gantry, carriage, torch and hoses moving. Cutting effects follow
the actual cutting phase and stop on pause. The table hides steel when empty.
The press brake now cycles its ram, punch, piston rods and back gauge, with an
in-process blank and formed part. The roller rotates its cylinders and feeds a
physically sized shell blank through a bounded 32-strip bend presentation;
legacy raw-sheet demo visuals no longer overwrite factory stock visibility.
The welding gun follows a small bead motion while a real assembly stage is active.

Presentation predicts between authoritative snapshots, without changing recipe
weights, durations, inventory or save version. Disabling a machine now pauses
its existing part as well as preventing a new part. Pausing preserves equipment
pose. Clients reconnect presentation when replicated equipment arrives after
the plant, including late joins. Particle state is local, not per-spark RPCs.

Build succeeded. EquipmentMotionTests.log records 76/76 passing automation tests,
including actual imported-component motion, stock visibility, pause/resume and
material thickness. EquipmentMotionNetworkQA.json confirms actual plasma, press
and roller movement on all four peers, common pause, real-cycle completion,
exact machine result save/load and a fifth late join. This focused equipment
run did not observe a rounded forklift route; that unrelated navigation case
was validated separately in ForkliftRoutingNetworkQA.json. The first equipment
network run caught the client discovery bug and was not accepted. Its extended
legacy raw-production sequence was stopped after stalling; the focused rerun
covers equipment instead of claiming a fresh full raw-to-tanker network pass.

Native render review additionally checks real sparks, full press stroke, curved
roller stock, completed cycles and welding activity. See EquipmentMotionVisualQA.json
for its final result, and EquipmentMotion_* screenshots under Saved/Screenshots/Reviews.
These are stylized production animations, not a metal deformation physics solver.
No performance measurement or FPS improvement is claimed. Pre-change save and
editor settings are retained in Saved/Backups/BeforeEquipmentMotion.

Final native review completed with all nine checks true and no errors: real-stock
exercise, stable paused pose, live plasma sparks, full ram travel, curved roller
stock, completed cycles, live welding sparks, moving welding gun, and restoration
of the fresh paused two-zone/29-pallet scene. Saved that fresh scene and left the
standalone Foundryline editor open. Latest native evidence uses the 00001 images.
