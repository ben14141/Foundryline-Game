# Straight tanker and tractor test

Source: user-owned Megapoly.Art Trucks & Trailers I, 3_axle_Tanker_2 and
Semi_Truck_2. Original vendor assets are unchanged.

## Blender

SourceAssets/AssemblyReviews/TankerTractor/Foundryline_Tanker_Tractor.blend

Separate tractor and tanker roots, ten separately pivoted road wheels, two
sliding landing legs and footplates, kingpin and fifth-wheel locators. Metres in
Blender convert to centimetres in Unreal. The 360-frame / 30 fps demonstration
backs in, couples, retracts legs, tows, lowers legs and drives the tractor away.
Frame markers identify the stages. Wheels rotate from signed travelled distance.

## Unreal test controls

A test pair starts on the starter property's paved yard. Click TRUCK TEST or
press V to control it; V returns to construction. W/S move forward/reverse,
A/D steer, C requests coupling, G raises/lowers legs, U detaches. Camera zoom
remains available. First-person mode requires leaving vehicle test mode.

Bring the rear of the semi within 2.2 m of the trailer hitch with up to 60
 degrees of yaw difference; the trailer snaps to the semi after a clearance check.
Coupling raises the legs automatically. U brakes, lowers the legs and releases
once the trailer is supported. Driving away does not immediately reconnect it.
There is no need to manually operate the landing gear for normal pickup/dropoff.

The authority validates driver ownership, input bounds and coupling commands.
Input timeout brakes the vehicle. Another player cannot steal a claimed tractor.
Replicated pose, wheel travel, gear and hitch state are independent of construction
records. Simulation uses bounded 30 Hz steps and actor relevance, with 15 Hz
network updates; this is not an FPS or production fleet-capacity measurement.

## Scope

This is a low-speed kinematic test pair with small-step overlap checks against
world blockers, steering and articulated trailer following. It is not Chaos tire
physics, terrain suspension, fleet dispatch or general interchangeable trailer
selection. It does not yet consume logistics-route reservations or persist vehicle
state in property saves. Construction save/load remains separate. Imported source
parts retain authored dimensions; collision is a conservative test envelope.

## Verification

Saved/Logs/TankerBlenderQA.json checks detached foot contact, raised-foot clearance,
ten road wheels, matching hitches and wheel rotation. TankerRigImport.json records
14 independently pivoted Unreal parts and their bounds. TankerRigAutomation.log
records construction/gameplay regressions including TractorTankerCoupling.
TankerNetworkQA.json records four-player client driving, coupling/gear/detachment,
driver ownership, wheel-state replication and a fifth late join.


## Product and vehicle asset separation

Manufactured tanker parts: `/Game/Foundryline/Products/Tankers/StraightBarrel`.
Pickup tractor parts: `/Game/Foundryline/Vehicles/Semi/Megapoly`.
Detail materials: `/Game/Foundryline/Vehicles/Shared/Materials`.
Original source FBXs follow the same Products and Vehicles/Semi split under
SourceAssets. Combined Blender animation reviews live under AssemblyReviews.
The test assembly does not imply that semis are manufactured products.

Truck visuals interpolate between the 30 Hz simulation steps. Network clients
use a short snapshot buffer; the camera follows the displayed pose after the rig
tick, keeping relative motion stable at close zoom.

Trailer additions: tyre tread, wheel hubs and nuts, reflective side tape, rear
lights and reflectors, underbody piping/valves, and landing-foot fasteners.
Tractor UV layers are consolidated before export to preserve palette textures.

Latest verification: 42 automated tests passed, including 120 Hz rendering
interpolation against 30 Hz vehicle simulation (no held visual frames). All
14 updated co-op checks passed, including automatic legs on detach and a fifth
late join. Visual smoothness and material appearance still merit user playtesting.
