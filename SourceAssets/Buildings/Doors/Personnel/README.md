# Original industrial personnel doors

Game-owned original geometry, created for Foundryline. No vendor artwork used.

- Office leaf: light-grey painted steel; shop leaf: graphite painted steel.
- Both have a real glazed vision opening, stainless rim/screws, safety-return levers on both faces, key escutcheons, kickplates, three barrel hinges and edge latch.
- Panel dimensions: **36 x 84 in**, thickness **1.77 in** (91.44 x 213.36 x 4.5 cm). Hardware extends outside the panel dimensions; do not fit by the complete mesh bounding box.
- Rough opening: **100 x 220 cm**. Fixed jamb assets have 20 cm (office) / 30 cm (shop) reveal depth, door stops, compression seals, casing, strike and low threshold.
- Leaf panel clearances: 2.8 mm each side, 3 mm head, 6.4 mm above the finished floor. The 4 mm threshold leaves 2.4 mm below the leaf.

## Runtime placement

Frame origin is the rough opening lower-left floor corner, +X along the opening; it stays fixed. Both leaf meshes use the same hinge-at-origin, +X-toward-latch convention. Place the moving component in Unreal at **(A + 4.28, +2.55, finished floor +0.64) cm** in the opening's local basis, rotated by the edge yaw. In Blender assembly the Y offset is -2.55 cm; FBX import flips that axis.

Do not nonuniformly stretch these meshes. Do not offset from their complete bounding box: hinges, latch and levers are included. Root Z is already the panel bottom. The frame has no convex collision hull across the doorway: the construction renderer must retain jamb/header collision, while the animated leaf gets simple dynamic collision. Only the leaf, its glass and its hardware swing.

`Tools/Blender/build_personnel_doors.py` exports four original FBXs and renders closed/open/hardware review images. `Tools/Unreal/import_personnel_doors.py` imports them under `/Game/Foundryline/Buildings/Doors/Personnel`, with matching original materials. No map is loaded/saved by the import script.

The saved Blender asset scene retains zero origins for all exported parts. Review geometry remains in the scene but is not exported.

The imported leaf lies on the local negative-Y side of its hinge. A **+90 degree local Unreal swing** opens away from the frame stop and clears the jamb. For opposite handing, place the hinge at the other end and rotate/reverse the complete frame/leaf basis consistently; do not force a -90 degree swing through the fixed jamb. Blender review uses -80 degrees because import flips Y.
