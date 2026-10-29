# Handyman employee presentation

## Asset and identity contract

The purchased `/Game/Handyman` content remains unmodified. Default presentation loads `/Game/Foundryline/Employees/Handyman/DA_HandymanEmployees`. This profile references the original Handyman skeleton, idle/walk clips and three coherent modular outfits (long-sleeve work clothing, boots, work caps and varied heads/facial hair). Source geometry was measured at roughly182 cm including footwear/cap, with soles at0 cm; display uses1:1 scale and a -90-degree facing correction from the model's native+Y forward to simulation+X.

Variant selection is `FCrc::StrCrc32(Employee.Key) % 3`, where the authoritative key includes company/plant/employee ID. Role changes, seating and pooling retain the same component and variant. No roster, hiring, jobs, ownership or save schema was added or changed.

Each shared appearance profile builds its three runtime meshes once using the native skeletal merge operation. It retains the original weights, reference skeleton and palette material, reducing each worker to one visible skinned body. These merged meshes are transient render resources, not editor-imported source assets. Saving a runtime merge directly as an editor SkeletalMesh was rejected because it does not carry a reimportable ImportedModel; no invalid saved variants remain in Content.

## Animation and pose

A hidden skeletal sampler evaluates included `anim_Idle` and `anim_Walk_1` clips. One visible poseable mesh consumes those bone poses. Walking phase follows actual smoothed distance, and leg IK supplies a planted stance/swing arc at measured ankle-to-sole offset. Simulation positions remain authoritative; skeletal meshes have no collision or root-motion authority.

Keyboard and steering-wheel hands use measured local job targets and bounded two-bone arm IK. Forklift boots sit at the authored83 cm cabin floor. Welding palms are fitted after the real gun moves, using its actual grip/support points; error is measured from the resulting skeletal palm position. Existing standing Pawn capsule remains separate and is disabled while seated. Dedicated servers build only that collision/presentation root, without loading or merging character meshes.

`GetPresentationReport` preserves the old movement/collision/feet/limb fields and adds `skeletal`, actual transient `mesh`, `profile`, `outfit` source paths, `variant`, `animation`, palm local positions and palm world arrays. Multiplayer appearance comparisons use stable key/profile/outfit/variant, not transient object suffixes.

## Scope and limits

This pass targets the starter plasma operator, forklift driver and actual welding job. Work caps are the vendor models; specialist welding PPE and bespoke additional gestures remain future art. The existing roller console location is beyond a normal operator's reach; moving that console and its navigation/collision footprint is a separate equipment-layout change. No roller geometry or backend station positions were changed here.

## Verification

Profile activation succeeded with zero errors/warnings. A separate process rebuilt the three outfits from persistent original parts and confirmed14476,14566 and14024 render vertices, with87 bones each (`Saved/Logs/HandymanActivationProof.json`). Source compiled successfully. Focused actual-bone gait, grip, seat, pause and identity tests plus native job/network acceptance are recorded below by the acceptance coordinator.

### Final focused acceptance (2026-10-28)

- `HandymanFinalEmployeesAcceptance.log`: 15/15 Employees automation cases pass, exit0. Includes actual-bone planted gait across the full phase sweep, keyboard and gun palm targets, translated/rotated actor, forklift seat/floor, pause, stable identity, company authority and saved jobs.
- `HandymanNetworkQA.json`: 66/66 checks pass with four initial peers and a fifth late join. Outfit/profile/variant agree for each shared employee key, without duplicate bodies; private business commands and jobs remain scoped.
- `HandymanStarterNativeQA.json`: real rendered 40-bracket production, staging, carrier delivery and $440 payment completed with $4,112.08 remaining. Used real UI command paths and shipped8x control; no inventory/cash grants or simulation-advance helper. Two original observer flags reflected obsolete assumptions (persistent mesh path and unscaled animation-culling bounds); runtime merged mesh/profile and bounds-scale-aware checks were corrected in the observer source. Functional, identity, animation, production and payment checks passed in the recorded run.
- `EmployeeWelderJobNativeQA.json`: 9/9 checks pass in the transient stocked larger-workshop fixture. `EmployeeJobVisual_welder.json`: 8/8 visual checks pass, including actual gun palm alignment, sparks, gait, stable component through assignment and idle holstering. This fixture uses the existing simulation-advance helper and is separate from the normal starter-order proof.
- Native reviewed images: `HandymanStarter_OperatingPlasma00000.png`, `HandymanStarter_ForkliftDriver00000.png`, `EmployeeJobs_WelderAtProduct00001.png`. The8x moving walking capture has temporal trails; component counts confirm a single body.
