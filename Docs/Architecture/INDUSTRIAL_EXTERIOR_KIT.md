# Industrial exterior detail kit

The game-owned exterior kit is generated with every shop and office. It follows Foundryline's existing wall/trim colors and uses the saved shell and opening layout. It adds no build-menu options or separately managed objects.

## Pieces

| Detail | Physical construction |
| --- | --- |
| Concrete base | Existing shop base retained at 60 cm; office base becomes a smaller 20 cm concrete skirt. Folded cap flashing and drip lip stop at door thresholds. |
| Corner caps | Two 12 cm-wide metal legs at exposed corners; straight expansion seams do not receive duplicate corner caps. |
| Cladding seams | Thin seams at 120 cm spacing, cut around windows and doors rather than drawn over them. |
| Roof edge | Folded flashing and a drip edge on exposed perimeter spans. |
| Gutters | Open U-channel on the long eaves: 24 cm channel width, thin bottom and side folds, end caps at exposed ends. Straight joins keep the channel open. |
| Downpipes | Fixed 10 x 10 cm section, cut to the building's height. Collector, retaining straps, wall brackets, outlet shoe and concrete splash block. Placement avoids openings. |
| Loading/garage door | Jamb covers, header, roller housing/end plates, rubber bumper pads with fasteners, and a shallow 110 cm-deep canopy with folds/brackets where headroom allows. |
| Personnel entrance | A smaller 75 cm-deep canopy, threshold, wall-lamp housing/lens and a physical number plaque. The plaque uses small mesh strokes, not floating text. |
| Utilities | One suitably placed electrical cabinet with door/latch, conduit and junction box. |
| Ventilation | A compact high wall vent with recessed backing and separate louvers, positioned clear of openings. |

The lengths of rails, gutters and pipes fit their spans. Their cross-sections and sheet gauges retain real centimetre dimensions. These are reusable generated mesh parts, not resized whole-building models.

## Architecture and joins

- `Source/Foundryline/Public/Buildings/FoundrylineExteriorKit.h` and its private implementation own the deterministic part layout, opening clearance and shared-edge clipping.
- `AFoundrylineBuildSite::Render` batches generated pieces by finish into instanced static-mesh components tagged `FoundrylineExteriorKit`. The finishes reuse game-owned trim, interior, concrete and white materials, with dark rubber and galvanized-metal tints.
- The existing construction-change pipeline rebuilds nearby sections when a section changes, joins, resizes or is removed. Decorations regenerate from the exposed perimeter, including on clients and late join.
- An office retains its structural partition against a shop; that indoor face is separately excluded from exterior decoration.
- Weather-wall details already used at different-height connections remain in place. The kit does not add columns or skirts across a shared open shop floor.
- Details, canopies and gutter hardware remain with the exterior walls when the local player hides the roof. Existing roof controls and first-person roof requirements are preserved.
- The kit has no tick, separate ownership, new RPCs, or additional save records. It derives from the server-validated saved building layout and each player's local presentation.

## Scope

The added pieces are decorative, non-blocking geometry. Door openings and the underlying building retain their existing physical collision. Small trim, conduit, brackets and thresholds do not create new movement or navigation obstacles. The wall fixtures include housings/lenses; this pass does not add a new exterior-lighting simulation. No measured FPS improvement or production performance acceptance is claimed.

## Validation

`Foundryline.Construction.ExteriorKit.ScaleOpeningsAndJoins` checks fixed dimensions, positive geometry, door and window clearance, gutter gauge, open expansion seams, office/shop partitions and tall-shop downpipe dimensions. `Tools/Unreal/review_exterior_kit.py` checks generated instances, materials, collision-free previews, four-player parity, client construction/joins, a fifth-player join, local roof preferences and perimeter restoration after removal, while capturing close-ups in an unsaved review scene.

2026-10-30 acceptance: editor build passed; native exterior review passed all 12 checks (`Saved/Logs/ExteriorKitQA.json`, `ExteriorKitReview2.log`) with four peers plus a late fifth join. Each initial peer generated nine material batches / 376 decorative kit instances, with no missing materials or blocking kit collision. Client joins, tier changes, office/shop faces, removal restoration and local roof visibility matched. The subsequent full suite, including the office roof/light/glass additions, passed 157 tests with zero failures. Rendered close-ups are in `Saved/Screenshots/Reviews/Exterior_Kit_*00001.png`. All review construction was unsaved.
