# Physical collision audit

Player dimensions remain 30 cm capsule radius, 90 cm half-height and 25 cm step height. Authoritative queries and client presentation use game-owned bounded primitives; no vendor meshes were edited and no physics simulation was enabled.

## Coverage

- Plasma primary body, slat structure envelope, operator console, power unit and extraction unit; brake primary body and operator console; roller primary body. Bounds follow actual asset component transforms.
- Factory forklifts use eight compound chassis/mast/cab-post/roof boxes, leaving open space above the chassis rather than a single full-height rectangle. Pool entries without a live driver explicitly disable collision. Roots exist on dedicated servers too.
- Physical pallet stacks union the displayed wood supports and actual part mesh bounds. Rack rotations, zone rotations, fork smoothed position/yaw and lift follow visible rendering. Offsite/carrier cargo is excluded from plant floor collision; removed IDs destroy their old component.
- Racking, physical product and zone welder existing boxes are retained and tagged for native audit.
- Office room tables, chairs and plant pots receive bounds-based Pawn fallback; office computer retains its imported collision. Standalone welding bench/body/legs now have physical bounds.
- Starter truck retains measured compound boxes, now attached to the smoothed display. Other temporary carrier types receive proxy-bed/cab bounds. Delivered/offsite pooled actors disable collision; reuse reenables it, and clients follow hidden state.
- Existing walls, doors, curbs and floors retain authored building collision; previews, floor paint and roof visibility layers remain nonblocking.

All new bounds are tagged `FoundrylinePhysicalCollision`, QueryOnly, WorldDynamic object type, Pawn block, other channels ignored, and navigation-affecting disabled. Existing structural/rack boxes retain their original profiles.

## Intentional limits

Thin empty pallets and a few flat sheets can be stepped over at the real 25 cm step height. Small cables, grass, flowers, tool leads and holograms are decorative. Moving scripted vehicles have player-query collision but do not implement human-player-aware traffic yielding or swept rigid-body motion. Product collision remains the existing assembly envelope after physical production starts. Final employee skeletal models are independent of this collision pass.

## Validation

Focused automation covers the actual walking capsule against machine bodies, forklift chassis and outside negative paths, pool retirement, and pallet geometry/retirement. Live possessed-character movement sweeps and representative keyboard approaches are coordinated with the native review harness. Results are recorded after the acceptance run, rather than inferred from source.

Compiled successfully and the combined automation suite passed 113/113 on 2026-10-28 (Saved/Logs/PlayerCollisionFullAcceptance.log, exit code 0). Native player and multiplayer acceptance remains a separate check.

Nearby starter birch/broadleaf instances also receive conservative, narrow low-trunk capsules through FoundrylineTreeCollision. This scans loaded instances once after startup, bounded to 200 m from the starter origin and 4096 stems; canopies and undergrowth remain passable. Explicit species stem profiles are scaled with instance transforms, rather than deriving collision from the canopy bounding box. Tree batch acceptance is recorded separately from the initial 113-test result.

Tree-inclusive build and full automation passed 114/114 (Saved/Logs/PlayerCollisionTreeFullFinal.log, exit 0). A real multiplayer low-pallet step reproduced an unsupported local MovementBase; the walking character now excludes tagged physical bounds from based-movement replication while retaining stepping/floor collision. The repeat live multiplayer shipping/stepping run passed 67/67 with no movement-base warnings. Native visual fit is the final separate review.
