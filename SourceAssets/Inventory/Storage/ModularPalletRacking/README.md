# Original modular pallet racking

These three original Foundryline rack models share a 10 ft 0 1/16 in wide by 4 ft 10 1/4 in deep footprint (305×148.03 cm, including base rails/label). There are two standard pallet positions across each usable level. The floor IS level1: the1-level model is a low protected floor-storage bay, not a raised shelf with an additional floor level.

| Model | Total frame height | Pallet bottom / deck heights | Future logical positions |
|---|---|---|---|
| PalletRack_1Level | 5 ft | Floor0 | 2 |
| PalletRack_2Levels | 10 ft | Floor0,5 ft | 4 |
| PalletRack_3Levels | 15 ft | Floor0,5 ft,10 ft | 6 |

The three variants keep the same post locations, width, depth, orange beams and graphite C-channel frames. Side rails sit about1 in high, outside the pallet footprints; there are no tall four-corner pedestals or front floor beam obstructing the fork approach. Rear/side bracing stays outside the pallet envelopes. Upper levels use a shared open steel deck, not a floating pallet.

## Scale and access

- Dimensions are authored in metres in Blender, imported as real centimetres; no arbitrary instance scaling is required. Mesh origins are centre of the rack footprint at floor0; front approach is Blender-Y. UE import scene conversion may mirrorY; the rack is functionally symmetric front access except rear bracing, so inspect the front rather than rotating individual components at random.
- Pallet fit uses measured bounding dimensions only:117.974×115.706×17.557 cm. No purchased pallet mesh, texture or material was copied into these original assets. Rendered pallets/steel plates are independently built review geometry, excluded from the complete rack FBXs.
- Slot centres areX±70 cm, and the front opening between posts is282 cm. The two measured fork-blade centres101.702 cm apart plus11.677 cm blade width give a113.379 cm outer blade envelope. That fits each140 cm position. This verifies the rack entry; it does NOT certify unknown vendor pallet pocket geometry.
- The geometry envelope allows100 cm of goods above the pallet and8 cm of lift. The next orange beam underside is140.4 cm above the prior deck, leaving14.843 cm clearance over that raised loaded pallet.
- Highest deck304.8 cm plus8 cm lift is312.8 cm, within the current330 cm forklift lift range. Existing mast/truck collision and actual dynamic put-away animations are separate integration concerns.
- Allow at least12 in overhead beyond the frame. The3-level rack therefore needs about16 ft clear interior height; choose a taller shop rather than shrinking the model. Payload dimensions here are geometric envelopes, not a structural load-rating certification.
- This system is for standard part pallets. Full60×120 in raw steel sheets need the separate dedicated sheet-storage system/cradles; they do not fit these standard pallet positions.

## Contents and integration boundary

Complete FBXs: `PalletRack_1Level.fbx`, `PalletRack_2Levels.fbx`, `PalletRack_3Levels.fbx`. Shared shelf and three upright-frame FBXs are provided for future modular assembly. Each variant has an editable Blender file, plus `PalletRack_AllLevels_Review.blend` and one side-by-side render. All geometry, review props and procedural materials are original; there are no external image/font files or vendor dependencies.

`manifest.json` provides material colours/metallic/roughness, dimensions, shelf tops, explicit slot positions and target native asset paths. `geometry_qa.json` records33 numeric scale, separation, approach and clearance checks. The generator is `Tools/Blender/build_modular_pallet_racking.py`, with saved-geometry/contact verification in `Tools/Blender/verify_modular_pallet_racking.py`, run in background with2CPU threads and a12-sample CPU review render.

This delivery supplies MODEL VARIANTS only. It does not alter gameplay source, storage schema, placed rack inventories, pathfinding, UI, save data or ledger capacity. Future integration can map a selected model's levels to2×levels actual slots; no capacity has silently changed here. No active Unreal/Claude process was launched or interrupted by the rack author.
