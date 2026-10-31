# Loading bay presets

The office computer's Planning View places a complete loading bay. Choose **Site > Loading Bay Preset** (also available through the wheel and Doors), point at an exterior shop wall and click. The wall determines orientation. There is no custom loading-bay paint tool.

## One owned unit

A saved building opening owns the whole preset: clear door opening, grade-level concrete apron, protective concrete bollards, vehicle stop markings, fixed forklift access stripes and direction arrow. The initial hologram includes the imported two-pallet flatbed. The apron contributes an actual paved driving surface and physical floor; the preview has no collision. No raised platform or loading ramp is created.

Select the bay at its door or anywhere on its apron. Move it to another wall to turn/reposition the entire preset. Delete removes the opening, apron, bollards, paint and derived logistics destination together. There are no independent paint settings, sizes or paint objects for its fixed markings.

A nearby existing garage/truck door can be converted when placing the starter preset. Conversion retains its opening ID. Ordinary garage and truck doors remain available independently.

## Upgrades

| Preset | Apron | Door opening |
| --- | --- | --- |
| Small flatbed | 25 ft x approximately 45 ft | approximately 11 ft 6 in square |
| Box truck | approximately 30 ft x 50 ft | approximately 13 ft 1 in x 13 ft 9 in |
| Semi | approximately 35 ft x 80 ft | approximately 16 ft 5 in square |

Select a bay and choose **Preview Box Truck Upgrade** or **Preview Semi Upgrade**. Review the larger hologram, then confirm or cancel. The upgrade validates the entire new footprint and door opening before committing. Upgrades keep the opening ID used by the logistics target; they do not delete/recreate the bay or rewrite orders/rack assignments. These are fixed development presets, not a completed research/pricing progression system.

The starter dimensions account for the imported vehicle's approximately 10.5 m overall envelope and access beside its bed, rather than its two-pallet capacity alone. Semi turning manoeuvres still need a connected driveway and sufficient approach pavement beyond the stopped-vehicle apron.

## Contracts

- `FFoundrylineOpening::LoadingTier`: 0 = legacy opening, 1 = small flatbed, 2 = box truck, 3 = semi. Old saves retain legacy dimensions and behaviour.
- `FFoundrylineLoadingBayLayout` owns dimensions, local geometry and world clearance bounds. Rendering, validation and driving use this shared definition.
- The opening ID remains the destination ID across conversion, upgrade and relocation. The synthetic driving-surface ID is derived from it. Deleting the opening removes both from the next revision of the driving map.
- Client construction goes through the owning player's validated server requests. The server validates company/plant permissions, revision, door conflicts, lot bounds, other buildings and bay overlap. Tool state and holograms remain local.
- Loading paint remains in the saved enum only for compatibility with existing pavement. The planner does not offer it as a drawing tool. Fixed preset paint is rendered from its bay and cannot be selected independently.
- The existing ERP freight simulation has an authored starter handling layout; this change does not implement a new dynamic multi-bay carrier dispatch system. The placement/logistics destination contract is ready for that follow-up.

## Acceptance - 2026-10-30

The editor build succeeded. The full automation suite passed 154 tests; a final focused construction run passed all 31 tests after the preview and test-helper changes. The rendered review passed all 18 checks: complete-bay wheel selection, custom loading-paint removal, collision-free preview, non-mutating preview, four-player placement, starter vehicle clearance, existing-door conversion without a duplicate destination, upgrade preview, stable IDs through both upgrades, per-player preview state, a client upgrade to semi, complete relocation, physical floor, overlap rejection, fifth-player late join, and complete deletion including floor collision and logistics destination.

Evidence: Saved/Logs/LoadingBayPresetQA.json, LoadingBayPresetReview3.log, LoadingBayPresetBuild7.log, Focused-Full.log and Focused-Construction.log. Reviewed images: Saved/Screenshots/Reviews/Loading_Bay_Small_Hologram00002.png and Loading_Bay_Semi_Placed00002.png. The review used an unsaved PIE scene; no authored level was overwritten. Temporary four-player settings were restored after all review processes exited. No performance/FPS or live dynamic ERP-dispatch acceptance is claimed.

Placement preview refinement: the fixed 20-cell cursor lattice was removed from the shared placement HUD. Snapping, the snapped cursor, actual hologram and draft footprint/dimensions remain. This applies across placement tools, including loading bays; switching tools no longer draws the repeated square grid.

Paint cleanup: a company's first preset bay suppresses the old fixed outdoor arrival/apron decals from the starter factory floor atlas. The atlas cache includes preset presence, so placement/removal and replicated changes refresh that choice. Indoor shipping/storage markings remain. Presets now draw one white vehicle outline plus yellow side-access stripes; selection uses one temporary outer footprint highlight, without a separate door selection box.
