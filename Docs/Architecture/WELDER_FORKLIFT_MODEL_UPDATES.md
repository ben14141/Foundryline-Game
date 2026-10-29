# Welder and forklift model updates

## Publication boundary

This update publishes the entirely original welder/cart/bottle/gun rebuild, original amber beacon, and original forklift enhancement kit/tools. The full forklift remains Factory-derived and is NOT distributed in this public repository. An authorized local Factory-derived forklift must supply the base vehicle when applying the enhancement tools. Original game model files remain unchanged until explicitly imported by the equipment implementation.

## Welder, gun and bottle

Sources: `SourceAssets/Production/Equipment/Welder/QualityReview_2026-10-29/`.

- `Foundryline_MIGWelder_QualityReview.blend`: editable original assembly.
- `Welder_Machine.fbx`, `Welder_MIGGun.fbx`, `Welder_Leads.fbx`, `Welder_WorkClamp.fbx`: compatible exports.
- CartFrame, CartWheels, PowerUnit, GasCylinder, GasRegulator, MIGGun, Leads and WorkClamp FBXs: separately editable mechanical groups.
- `Welder_Complete_Review.fbx`: preview assembly only; do not spawn alongside component meshes.
- `manifest.json`, `geometry_qa.json`, `label_surface_qa.json`: dimensions, anchors, source validation and corrected labels.

New details include a real wire-feed window/spool/feed rollers, front controls/sockets, service seams/vents, shaped cylinder/base ring, valve/regulator/two gauges, retention chain/hoses, hollow copper nozzle/contact tip/wire, trigger and ground clamp. Labels have no extrusion/billboard behavior: bottle/torch markings follow curved surfaces; panel legends and gauge markings sit on their intended faces. All fourteen labels were checked at vertices and face centres against the actual meshes (0.143–0.297 mm gaps, minimum normal alignment 0.9967).

Gun attachment anchors remain Grip(0,0,0), Support(3,-3,8), WeldTip(23.9,0,13.5) in local centimetres. Thirteen source/export checks pass, including open nozzle bore, contacts, exact grip/tip preservation and unchanged production assets. These are source-model checks; Unreal materials, collision and final worker/animation adoption must still be verified after import.

## Forklift and amber beacon

The original standalone beacon lives under `SourceAssets/Vehicles/Forklift/AmberBeacon`. It provides a hollow amber lens, visible light engine, base and fasteners, with SOCKET_Bulb / Bulb_LightAnchor. Mount at forklift-local roof(-115,0,253) cm; bulb is(-115,0,263.1) cm. The manifest records a warm orange light configuration and small attenuation radius. Actual Unreal point-light creation is not implemented by publishing these model files.

The local licensed review source is `SourceAssets/Vehicles/Forklift/QualityReview_2026-10-29/Forklift_QualityReview.blend`; it is deliberately excluded from Git. Do not upload its complete or component FBXs. The quality pass preserves the eight moving group transforms, original seat/controls/forks/wheel positions and measured lift travel. Sixteen source checks passed. It adds body/service/vent/step/guard/wheel/mast details and one roof beacon.

`Tools/Blender/refine_forklift_quality_20261029.py` applies the authored refinement to an existing licensed local source set; then `fix_forklift_review_labels.py` removes the old floating glyphs and places two flush printed panel marks. `verify_forklift_quality_20261029.py` checks compatible local results. The tools expect the authorized MaterialHandling manifest and source files at the documented local project paths; a clean public clone alone does not contain those licensed inputs.

Original additive meshes, when supplied in the public enhancement kit, are not a replacement for the base forklift. Do not spawn the old embedded beacon together with the new housing. Component and material/attachment manifests are the handoff for integration.

## Isolation

No Unreal editor, import, build, gameplay logic, maps, employee assignments or player saves were changed for this source-art update. No worktree or live file used by Claude was modified. The pending unrelated tanker edit remains paused and excluded.

## Forklift integration (game)

- The quality-review components are imported locally over `SM_Forklift_Body`, `MastOuter`, `MastInner`, `Carriage` and the four wheels by `Tools/Unreal/import_forklift_quality_review.py`, after the four Blender tools above have run against the licensed local sources.
- The review FBXs are exported in vehicle space. The importer moves each component back by its manifest rest position, so the pivots, wheel spin and mast lift match the previous meshes.
- Nothing produced by these steps is committed: the meshes and their `FQ_`/`Beacon_` materials stay in the ignored `Content/Foundryline/Vehicles/Forklift/FL01`.
- The old embedded beacon is removed, and Codex's beacon housing is part of the body at roof-local (-115,0,253).
- The beacon is deep blue in game, like a warehouse pedestrian-warning light (Ben, 29 Oct). The lens and bulb use blue variant materials, and Codex's amber definitions are left unchanged.
- `FFoundrylineForkliftBeacon` adds the point light at the bulb (-115,0,263.1) on every forklift, both the factory fleet and the material-handling cell. It uses the manifest's 45 lm, 120 cm radius and no shadows, and it is built locally on each machine (not on a dedicated server).
- Check: `Tools/Unreal/run_forklift_beacon_network.ps1` (two players) and `review_forklift_beacon.py` (rendered).
