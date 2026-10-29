# Office workstation presentation

The office computer is a game-owned triple-monitor workstation at true centimetre scale. Its existing 152 x 76 cm desk remains in the same office position, with a 78 cm work surface. Three matching active panels are 52 x 29 cm; the side panels turn inward 25 degrees. Overall hardware bounds are approximately 162.7 x 76.0 x 127.8 cm, with a small monitor overhang rather than a wider floor footprint.

The model includes a three-arm clamp mount, connecting reach arms, VESA plates, rear display ports/vents, cable grommets and tray, routed display/USB/power cables, stepped keyboard keys and legends, shaped mouse, and a detailed under-desk tower with ports, ventilation, panel screws, feet and Foundryline trim. All geometry is one static mesh with separate material slots for the three displays.

## Source and repeatable workflow

- Editable source: `SourceAssets/Operations/OfficeWorkstation/Foundryline_OfficeWorkstation.blend` (individual parts retained in the hidden Editable_Workstation_Parts collection).
- Shared geometry: `Tools/Blender/office_workstation_geometry.py`.
- Dedicated export: `Tools/Blender/build_office_workstation.py` produces `SourceAssets/Operations/OfficeWorkstation.fbx` and a dimensions/material manifest.
- Dedicated import: `Tools/Unreal/import_office_workstation.py` updates only the workstation and its office materials.
- Full operations builder and importer delegate to this geometry/import path, preventing regeneration of the old single-monitor computer. The combined Operations Blender source was also synchronized; unrelated source objects retained their dimensions.

## Display interface

Static mesh slots are exactly `OfficeDesktopScreen_Left`, `OfficeDesktopScreen_Center`, and `OfficeDesktopScreen_Right`. Each active panel has full 0..1 UVs. The shared `M_OfficeDesktopScreen` exposes the texture parameter `DesktopTexture`; runtime creates three independent dynamic materials/render targets.

The proven material path is Default Lit with texture RGB connected directly to Base Color and Emissive. The previous unlit/multiply graph rendered black in native review despite correct render-target pixels; the final path was verified on the actual workstation. `repair_office_desktop_material.py` verifies graph connections and parameter naming and is reused by the importer. The loading tablet keeps its original logo material.

The centre monitor shows the business overview, the left production/sales, and the right freight/named staff. Display information is local presentation derived from the authorized company state, refreshed once per second. The full office ERP still opens with E; the physical screens do not replace its controls.

## Local acceptance

The final import exited successfully with all thirteen material slots assigned and all three display slots preserved. `OfficeWorkstationImport.json`, `OfficeDesktopMaterialRepair.json`, and `OfficeOperationsSourceSync.json` record import, graph and source-preservation checks. Native review passed 6/6 checks: three live materials, first-person position, E opens ERP, hiring preserved, close returns to first person and the business baseline remains untouched. `Workstation_FirstPerson00001.png` was visually reviewed with three visible, distinct, correctly oriented displays. This is local native/editor acceptance, not a packaged or measured performance result.

Final physical interaction/collision evidence: WorkstationFinalState.json confirms physical E opening and Close return, actual walking-capsule sweep blocked by desk, restored first-person view, two employees//paused. Final phone screenshot Workstation_FinalDesk00000.png. The temporary Python get_pawn diagnostic spelling error was corrected using GameplayStatics.get_player_character; no game code failure resulted.

Final physical check: WorkstationFinalState.json confirms E opens/closes the ERP and a real player capsule sweep toward the desk is blocked. The player was restored to fresh first person with $4000, two staff and the simulation paused. Final image: Workstation_FinalDesk00000.png.
