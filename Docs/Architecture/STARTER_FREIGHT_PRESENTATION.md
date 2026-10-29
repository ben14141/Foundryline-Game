# Starter freight presentation

The first-order shop uses the purchased Megapoly.Art Trucks `Truck_sidewall`
flatbed for Foundry Lane Haulage. The source Unity package is preserved outside
Content; the derived Unreal assets live under
`Content/Foundryline/Logistics/StarterHauler`. Nothing in Spellscape or the
original `Content/Factory` package is changed.

## Units and conversion

`Tools/Blender/convert_starter_flatbed.py` reconstructs the author prefab's local
transforms rather than using the raw FBX hierarchy's inconsistent unit transforms.
The original complete vehicle bounds are 1055.83 x 317.21 x 405.67 cm. Meshes import
with scene-unit conversion and scale 1. `StarterFlatbed_AssemblyReview.blend` preserves a complete assembled source fitting
view with the separate wheels, open gate, pallet supports and three visible sheets.
The six wheels export around their real
axle centres, with an approximately 47.35 cm rolling radius. A thin game-owned cab
shell, light paint and separate glass material complete the cab behind the source
trim pieces without increasing those bounds. The loading-side gate is separated
from the bed for opening at the bay.

A Blender geometry ray cast over eighteen points measured the bed surface at
112.951 cm above the tyre ground plane (`Saved/Logs/TruckDeckSamples.json`). The
loading anchor is rounded to 113 cm above ground. With road surface Z=20, pallet
bases are Z=133. The forklift root remains at Z=20; its mast lifts the load rather
than driving the forklift onto the truck deck.

The stationary carrier origin is (1260,1700,20) in plant coordinates. The two
pallet bases are (1200,1635,133) and (1200,1765,133), matching the simulation's
loading anchors. The first anchor also carries the incoming sheet pallet. The
sheet mesh is already 152.4 x 304.8 cm along X/Y, so no additional rotation is
needed. Its long axis runs along the flatbed. Two wood supports under the long
sheet stack represent its two occupied freight spaces. Visible sheets and cut
parts are bounded samples; the ledger retains exact quantities and weights.

## Authority and movement

Bookings, company funds, bay FIFO, docking time, reservations and cargo transfers
are authoritative simulation state. The carrier actor only presents the booking.
It replicates vehicle kind, visible part/quantity samples, bay-ready status and
movement, without customer contract or financial terms. Relevance is bounded by
300 m instead of forcing every carrier to every player. Each plant displays at
most eight active freight actors and reuses them.

The front axle steers from displayed path curvature with a 35-degree bound; rear
axles remain fixed. All wheels spin from distance and the measured rolling radius.
The arrival, bay manoeuvre and departure use rounded fixed paths through the
starter road/driveway/apron. Twelve game minutes are reserved for entering the bay
before cargo handling begins. The side gate opens once docked; the wheels spin
from the distance actually displayed. Departure occupies the bay for twenty game
minutes while the vehicle clears the site. The starter apron extends north to
Y=3600 so the flatbed can turn forward through the yard.

These starter presentation paths are separate from the configurable general
logistics node network. Changes to the property layout do not yet regenerate
these carrier paths. Other carrier capacities and service terms exist in the
booking model, but their dedicated van/box/semi presentation is still represented
by simple temporary meshes; this conversion supplies the purchased starter
flatbed. Four-player acceptance and frame-rate performance must be reported by
the native acceptance run, not inferred from the mesh conversion.

## Editable Blender control rig

`SourceAssets/AssemblyReviews/StarterHauler/Foundryline_StarterHauler_Rig.blend`
uses the same rigid-part workflow as the earlier tanker/tractor source. Select
`StarterHauler_ROOT` to use `travel_metres`, `steering_degrees` and
`loading_gate_open`. Six axle-centred wheel objects, two steering pivots and the
loading gate use nine control drivers. The named action
`Hauler_Drive_Steer_LoadingGate_Demo` spans frames 1-420, with timeline markers for
approaching, turning, parking, loading, closing the gate and departing. Textures
are packed into the Blender file.

`Tools/Blender/build_starter_hauler_rig.py` rebuilds and evaluates the actual rig at
its key poses. Ten checks are recorded in
`Saved/Logs/StarterHaulerBlenderRigQA.json`, including tyre contact, unit scale,
front-only steering, distance-driven rotation and gate open/close. Unreal uses the
independent rest meshes and booking state rather than playing a canned driving
clip. `Foundryline.Logistics.StarterHaulerAssemblyAndAnimation` verifies that
same mesh assembly and animation behavior in an engine world.

## Playtest pavement review

The cut-parts profile's truck access now ends at the freight-apron edge (Y=-200),
so placement accepts it instead of silently rejecting its former overlapping run.
The protected street extends to X=10000 and the carrier leaves at X=9300, keeping
its full length on the street before hiding in transit. The 14 m driveway mouth
allows the measured tyres to sweep through the junction. Arriving traffic uses
the eastbound lane at Y=-3000; departing traffic completes its wide junction turn
then smoothly returns to that lane over X=6500..8700.

`Tools/Blender/review_carrier_pavement_sweep.py` checks the authored pavement union
against the measured six wheel pivots and tyre widths. The static review samples
2418 arrival poses, 1054 docking poses and 2728 departure poses; all sampled tyre
contacts are paved (`Saved/Logs/CarrierTyrePavementReview.json`). This geometry
review does not replace native motion/physics acceptance. The cut-shop layout
engine test separately asserts that the access is actually placed and connects
the public street to the apron.
