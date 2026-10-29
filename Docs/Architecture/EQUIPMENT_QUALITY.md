# Equipment art and mechanical-motion quality pass

Game-owned machine meshes remain separate by mechanical function. Blender sources and FBX exports live under `SourceAssets/Production/Equipment/{PlasmaTable,PressBrake,SheetRoller}`. Units are real centimetres in Unreal; machine dimensions and production recipes are unchanged.

- Plasma: additional linear bearing shoes, grease nipples, Z-axis bushings/coupling and torch clamp screws. Only long structural spans are extended to the large bed; small in-envelope hardware no longer stretches with the table. The actual factory cycle has eased rapid travel, torch lowering, stationary pierce dwell, decelerated contour corners, retraction and return. A bounded 64-link instanced return chain folds with the actual carriage; six existing spline hose spans follow both axes.
- Press brake: encoder scales/readers, ram guide wipers, back-gauge lead screws/bearings and die segment identifiers. Motion separates rapid approach, slower pressing, pressure dwell and retraction, with eased gauge and stock feed. The displayed stock/output now correctly maps the SupportChannel recipe to ReinforcementPlate/SupportChannel instead of brackets.
- Roller: independent upper bearing carriers and hydraulic rods follow the pressure roll; rotating end hubs and drive bolts make real shaft motion readable. Feed, curvature and pressure engagement ease in/out. The operator stand is a separate mesh moved 47 1/4 in toward its actual employee station; keyboard is within the calibrated 19 11/16 in hand target. Machine root, process duration and material accounting are unchanged.
- Standard welding bench retains its separate glossy painted frame/polished top, and separate rack/fixture assets. The unfinished pickup rack is unchanged.

Motion is driven from existing authoritative Remaining/Duration snapshots and bounded server-clock interpolation. Actual operator attendance is now required for extrapolation and VFX. Pause/absence holds the in-process pose; no client stock, production result or fake part is created by animation. Employee identity, company ownership and save schema remain unchanged. Dedicated servers do not build the cosmetic plasma chain.

All small manufactured details join their owning mesh; chain uses one 64-instance component and updates only when carriage X changes. Existing actor relevance/culling remains; this is not an FPS measurement or a production player-limit claim.

## Limits

These are game mechanical presentations, not hydraulic/thermal/structural simulations. Press parts still switch between authored flat and formed meshes during the stroke; there is no continuous metal deformation/strain simulation. The roller uses the existing bounded 32-strip forming surface. No new product recipes, stats or process times were introduced.

## Acceptance

Combined machine importer: 13 plasma, 12 press and 8 roller assets, scale checked against each authored manifest. `Saved/Logs/EquipmentQualityFinalImport.log` completed cleanly. Runtime/native/network results are recorded after the shared QA run.

### Focused final evidence (2026-10-28)

- `EquipmentQualityBuild.log` and `EquipmentQualityFinalBuild.log`: source builds succeeded. `EquipmentQualityProductionFinal.log` has13/13 successful Production cases; `EquipmentQualityEmployeesAcceptance.log` has15/15 successful Employee cases, including actual roller keyboard reach. The first Production run exposed two test assumptions (old plasma emission interval and a manually displaced worker retaining a seeded completed path); meaningful fixture corrections passed in the final run without changing production rules.
- `RollerCarrierOrientationQA.json`: corrected the new upper-bearing export's unapplied rotation. Actual Unreal dimensions are33.0×397.5×47.5cm, with explicit long-axis/height checks, rather than an erroneous397.5cm vertical span.
- `EquipmentQuality_After.json`:13/13 native checks pass. Real existing-stock cycles start and complete, cutting sparks and physical press/roller motion are observed, pause freezes the mechanical pose, bearing carrier/shaft positions agree throughout the sampled cycle, and first-person E opens each of the three machine computers. No normal save slot was written.
- Clear reviewed native images: `Saved/Screenshots/Reviews/EquipmentQuality_After_Plasma00001.png`, `EquipmentQuality_After_Brake00001.png`, `EquipmentQuality_After_Roller00001.png`. These use a transient close-up review camera with extra exposure; the map lighting and materials are unchanged by the camera. The first after-camera attempt was obstructed by foreground racks and excluded from final visual evidence.
- Baseline `EquipmentQuality_Before.json`:6/6 existing-cycle checks passed before the art/motion changes. Before shots remain available for comparison. This is functional/visual validation, not a measured performance claim.
- `EquipmentQualityNetworkQA.json`:12/12 checks pass across four initial co-op worlds and a fifth late join: real three-machine cycles, physical motion and nearby plasma sparks on all initial peers, frozen pause poses, real completion and exact late-join final poses. The review cameras are explicitly within VFX range; initial overview cameras were about14,000 cm high and correctly culled sparks at the existing5,500 cm cutoff. No production or relevance thresholds were changed for testing.
