# Starter-shop world playtest review

Review scope: the fresh cut-parts starter profile in the Unreal editor, its first
purchase-to-payment order, cooperative business authority, and first-person/property
views. Other game projects and vendor asset packages were not changed.

## Findings fixed

- The freight driveway previously overlapped the apron by 2 m, so placement rejected
  the run and left a grass gap. Its endpoint now touches the apron edge. The protected
  street continues far enough for the whole truck to leave on pavement.
- The starter access mouth and carrier street turns now accommodate the measured
  truck tyre sweep. The departure completes a wide junction turn, then smoothly
  returns to the eastbound lane.
- The actual starter carrier has bounded Pawn-query collision at its bed, chassis
  and cab. The plasma, press brake and roller use the actual primary-body mesh bounds
  for walking collision. These are separate from larger placement clearances and
  do not block dynamic material-handling helpers.
- The read-only native motion probe now examines visible mesh attachment parents
  and copies numeric position snapshots. All required checks begin false, so missing
  observations cannot silently count as a pass.

## Functional evidence

- All 93 Unreal automation cases passed, including actual character-capsule sweeps
  against the truck and all three machine bodies, and clear sweeps outside their
  physical bounds. See `Saved/Logs/FirstOrderFullAcceptance.log` and the final
  shared build/test logs recorded by the acceptance coordinator.
- The four-player run plus fifth-player late join passed 39 network checks; the
  first-person office/tablet/full-order walkthrough passed 29 native checks.
- `Saved/Logs/PlaytestWorldVisualProbe.json`: all six native checks passed at 1302
  samples. Forklift travel, mast movement, ground-level root position, plasma torch
  movement, cutting sparks and rack/pallet material assignments were observed.
- `Saved/Logs/BackendPlaytestObserver.json`: all five checks passed at 1041 samples,
  with authoritative invariants intact and real driver/machine/torch motion.
- Static geometry review sampled 2418 arrival poses, 1054 docking poses and 2728
  departure poses with all measured tyre contacts on the authored pavement union.
  See `Saved/Logs/CarrierTyrePavementReview.json`; this is geometry evidence, not a
  native physics or frame-rate benchmark.

## Native visual review

All ten floor-level captures listed in `Saved/Logs/PlaytestWorldCameraQA.json` were
inspected. No visual blocker was found. The camera-only review restored the original
view target and did not change inventory or player saves.

| Capture under Saved/Screenshots/Reviews | Reviewed behavior/appearance |
| --- | --- |
| WorldQA_ShopInterior00000.png | Overall scale, clear aisles, floor markings and interior lighting |
| WorldQA_PlasmaDetail00000.png | Machine geometry, controls, materials and placement |
| WorldQA_RawRacking00000.png | Raw rack geometry, shelf heights, floor contact and access |
| WorldQA_FinishedRacking00000.png | Finished rack geometry and surrounding clearance |
| WorldQA_ForkliftParking00000.png | Forklift materials, parked fit and floor contact |
| WorldQA_OfficeCorridor00000.png | Door placement, corridor and office lighting |
| WorldQA_LoadingThreshold00000.png | Shop/pavement height transition and loading opening |
| WorldQA_ExteriorOffice00000.png | Exterior/office connection, ground contact and windows |
| WorldQA_CeilingLighting00000.png | Hanging fixtures, skylights and complete first-person roof |
| WorldQA_GroundVegetation00000.png | Flat ground, close grass, trees and bushes |

Earlier first-order native captures separately show incoming sheets and outgoing
cut-part pallets fitted on the actual purchased flatbed. The editable Blender rigid
part rig and its ten pose/control checks are documented in
`STARTER_FREIGHT_PRESENTATION.md`.

## Readiness boundary

The starter first-order/editor playtest is ready for user feedback. This review does
not establish packaged/Internet-session acceptance, a production player cap or the
60-FPS target. Carrier paths remain authored for the starter property; arbitrary
property edits do not regenerate them yet. Other carrier visual variants, employee
avatars/player yielding, loose-pallet physics, and multi-business combined world
saving remain later work. Small exterior office partition seam lines are cosmetic.
