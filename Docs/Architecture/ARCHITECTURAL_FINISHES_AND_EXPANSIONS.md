# Architectural finishes and expansion detailing

The building kit remains 1:1 centimetres, with feet/inches in the player interface. These changes affect generated presentation and its existing collision geometry. They do not change construction prices, ownership, snapping authority, office rules, floor height or saved building dimensions.

## Finishes

Four game-owned materials live in `/Game/Foundryline/Construction/Materials`:

| Material | Purpose |
| --- | --- |
| `M_WallCladding` | Painted ribbed steel exterior, with restrained sheet seams and normal detail |
| `M_InteriorFinish` | Painted interior wall finish, independently tinted |
| `M_ArchitecturalTrim` | Painted metal columns, frames, fascia and caps |
| `M_FoundationConcrete` | Concrete slab and shop skirt, subtle aggregate and floor control joints |

The shader uses world centimetres rather than stretched cube UVs. Exterior ribs repeat every 9 inches and sheet seams every 3 feet, regardless of building width, depth or height. Concrete control joints use a 600 cm module. Fine detail fades with viewing distance, and seam edges use pixel derivatives to reduce aliasing. These are procedural surface details, not extra rib geometry. The original Foundryline exterior, interior and secondary trim color controls still supply the tints. Original vendor materials remain untouched.

`Tools/Unreal/prepare_architectural_finishes.py` creates or rebuilds only these four owned materials. Their folder is included in packaging settings. The renderer retains its previous materials as a fallback if a new finish is missing.

## Scaling and facade layout

Windows retain their existing physical dimensions; changing a building changes bay count and spacing. `FFoundrylineFacadeLayout::ColumnsForSpan` places complete pilasters inside each exposed wall interval, checks door/window frame clearance, prefers the exposed ends and removes near-duplicate candidates. A narrow leftover return does not receive a sliced pilaster. Posts are omitted where an opening occupies their space instead of being shifted into a neighbouring window.

Same-height sections along a continuous facade share ownership of the seam post through their saved object IDs. This prevents twin corner posts at an expansion seam and keeps the choice deterministic across peers. Different-height sections retain their respective structural framing.

Office corners use slim casings at exposed interval ends. Office windows remain manual. The existing door, room and shared-perimeter rules remain in force.

## Window and roof transitions

Exterior windows gain a projecting sill, header flashing and drip edge. Their physical depths remain fixed as the wall grows. Existing transparent glazing and its lighting settings remain unchanged.

The taller side of a roof-height transition retains its structural infill wall, interior finish and applicable high glazing. It gains full-width end returns and flashing above the actual 20 cm roof panel. Upper columns use the same opening clearance and exposed-span rules as the lower facade. The shared shop floor stays open. Step walls remain visible when roofs are hidden.

Partial roof tiles now use the same roof finish as full tiles, including exterior/interior tint selection, instead of a separate trim finish. This avoids a visibly different perimeter band on buildings whose dimensions are not a multiple of the native roof module.

## Validation workflow

```powershell
.\Tools\Validation\Invoke-FoundrylineCheck.ps1 -Profile Construction
```

Construction tests cover adaptive bays, full-width partial-span posts, opening clearance, odd-sized buildings, edit/resize/undo, office perimeter reuse, doors and connection snapping. The runner counts both clean successes and successful tests that report warnings.

`Tools/Unreal/review_architectural_expansions.py` performs an unsaved multiplayer review through the office planning view. It constructs odd-sized sections, adds a partial taller expansion and an office from different clients, requests a late join, captures exterior/material/interior/roof-off views, checks the rendered window dimensions, removes the tall section and reconnects a same-height replacement. Reports and screenshots are saved under `Saved/Logs` and `Saved/Screenshots/Reviews`; the map is never saved.

Visual checks and local peer checks do not establish a town-scale frame-rate result. The materials use additional shader work; no FPS improvement is claimed.
