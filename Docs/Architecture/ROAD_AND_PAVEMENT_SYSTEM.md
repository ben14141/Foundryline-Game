# Roads, driveways and custom pavement

## Player workflow

Open Planning View from the office computer, then Build > Site. Pavement has three tabs:

1. **Pave:** Rectangle, Driveway / Road, Custom Shape and Sidewalk. Drag rectangles; touching area patches merge. Custom shapes use corner clicks, Enter to finish and Backspace to remove a point.
2. **Paint:** Parking, white/yellow/dashed lines, arrows, keep-clear hatching and stop bars. Drag on asphalt; R flips direction. Parking starts with 9 ft spaces and 18 ft depth.
3. **Edit:** Click pavement or paint. Selected pavement shows directly draggable corner and edge handles. Roads show two endpoint handles plus a blue bend handle, including on straight roads. Click paint to select it; drag it again to move it, or press Delete. Deleting selected paint keeps the asphalt. Erase Asphalt trims an edge or makes an island.

The right inspector fits its contents; its input-blocking bounds match the visible panel. More Options exposes secondary tools and vehicle checks. Advanced options start collapsed when opening a pavement tab. Move, delete, curbs and selected-road width remain immediately available. Rounding and explicit reshape commands remain under More Options.

Roads default to a standard two-way driveway for a new player. The standard/truck preset, lane count and curved/straight choice remain explicit. Drag a straight run and release; for a curved run, release, shape the bend and click. R switches straight/curved mode. New roads receive lane paint automatically. Generated paint opens where another paved surface joins; manual markings remain under the player's control.

Drafts show dimensions in feet/inches and an area or clear road width, plus a short placement/cancellation hint or rejection reason. Cursor picking and the visible pavement grid use the same 25 cm spacing at the finished 20 cm surface plane. Buildings retain their existing structural snap. Direct handles run the same validated edit commands as the toolbar, including company/plant ownership and shared multiplayer state.

Existing pavement can be selected and updated with **Customize This Pavement**. This preserves its identity, world footprint and saved logistics coordinates while replacing the old template with custom geometry. Parking conversion lays out the new 9-foot spaces in the existing parking rows. A conversion can be undone in standalone play. Town-owned streets remain protected.

## Scale and presets

All geometry is in real centimetres, with 1 Unreal unit = 1 cm. The UI displays feet and inches. Fine pavement input uses the 25 cm grid; snapping changes positions, never model scale. Picking and the pavement grid use the finished 20 cm road/slab plane, preventing the cursor offset that results from projecting onto grass below it.

| Preset | Minimum clear driving width |
| --- | ---: |
| Standard one-way | 12 ft |
| Standard two-way | 24 ft |
| Truck one-way | 16 ft |
| Truck two-way | 30 ft |
| Four lanes | 48 ft |

Widths are game presets. Curbed road runs include an additional 18 cm strip per side outside the stated clear width. Arbitrary yard/apron shapes have no lane-width restriction. Erasing or rounding a road's actual surface converts that piece into a custom area; normal centerline/bend/width edits preserve its road classification.

Near a public street, supported road ends snap to its edge. Curved entrance tangents align perpendicular to that edge so the entrance meets the street cleanly. Native old-street curb openings and new pavement curbs use actual neighboring geometry.

## Paint and curbs

- Parking rows use 9 ft wide spaces, initially 18 ft deep; the row count follows the drag length.
- White and yellow lines use 4 in paint. Dashed lines have 10 ft dashes and 10 ft gaps, with phase retained across curve samples.
- Direction arrows follow the drag direction.
- Loading bays are rectangles with adjustable width and a length set by dragging.
- Keep-clear markings are bounded yellow diagonal hatching.
- Stop bars are approximately 16 in wide.
- Selected road runs offer **Paint Road Lines**: edge lines, a two-way yellow center and, for four lanes, dashed lane dividers. This generated road template follows later width and curve edits with stable marking identities.
- Use **Select Paint** to move or remove an individual marking group. Deleting paint preserves the asphalt beneath it.

Custom paint remains a local-space child of its pavement. Moving the surface moves its paint. Area merges rebase the paint and routing coordinates without moving them in the world. Cutting or reshaping preserves manual paint records; the inspector flags groups that no longer fit. Rendering clips their paint to asphalt, and invalid parking/loading groups do not create vehicle destinations.

Kerbs are actual beveled geometry, 18 cm wide with a 15.24 cm reveal above the 20 cm pavement surface. They have concrete finish, physical collision and spaced joints. They follow outer outlines and island holes. Joins with other pavement, sidewalks and building floors suppress the shared kerb. There are no internal kerbs between merged area patches.

Asphalt, concrete and paint have game-owned world-scale materials under `/Game/Foundryline/Construction/Materials`. Asphalt has restrained aggregate and broad variation; paint is opaque, non-emissive and slightly smoother; concrete has fine grain. Small detail fades with distance to avoid sparkling at town zoom. Vendor packages are not modified.

## Driving and logistics

The saved polygon, including holes, is the driving surface. An empty corner of a curved road's bounding box is not drivable. Vehicle body and articulated trailer checks use this same surface, along with curb clearance and existing adjoining surfaces.

The car/box-truck/semi preview simulates a sampled vehicle sweep along a proposed or selected road. Trailer offtracking is included. Blue indicates the sampled sweep fits; red indicates it needs more room. Short runs ask for more connected pavement to evaluate a full vehicle. This is a geometry check, not a guarantee against traffic or congestion.

New parking paint creates parking destinations with rotated bay footprints. Loading paint creates freight destinations. The logistics overlay clips area fills to actual pavement and shows these destinations. Street gate discovery supports the new polygons; one-way road starts allow entry, and one-way ends allow exit. The trip picker respects those directions.

Physical pavement, paint, and logistics routing remain distinct. Road runs are usable driving corridors; custom private yards continue to use the existing logistics driving-zone and node controls. Nodes cannot be newly placed in a pavement hole or a curved road's empty bounding-box corner. Geometry edits can invalidate existing routes, which must then be reviewed in the logistics tools.

## Data, authority and persistence

- `FFoundrylinePavementData` belongs to the placed-object record: polygons, holes, optional road controls, lane width/class, curb preference and marking groups.
- Polygon unions/differences use the engine GeometryAlgorithms library. Polygon triangulation uses GeometryCore constrained Delaunay, retaining holes.
- `UProceduralMeshComponent` presents static slab, kerb and paint sections. Physical collision comes from the slab and kerb triangles; paint has no collision.
- Geometry and markers have explicit complexity limits. Non-finite values, self intersections, bad holes, undersized roads, malformed IDs and out-of-bounds placement are rejected.
- A road record must match the surface generated from its saved width and controls; a client cannot claim a legal width while submitting an arbitrarily narrow road.
- Construction commits run through the existing authenticated, server-owned construction path. Merge/cut batches are restricted to pavement operations. Every affected record is checked against the requester's current company and plant; public street and other-business ownership are preserved.
- Local placement ghosts and vehicle previews do not reserve or mutate shared construction. The server validates the complete current-state batch before committing it, preventing overlapping competing edits from bypassing placement rules.
- Area merge and cut undo are compound transactions in standalone history. Original identities and paint records are restored atomically. Existing multiplayer undo restrictions remain in effect.
- Save/load and late-join replication carry the new nested geometry and paint records. Existing records with no pavement data retain their old representation until updated.
- Material/cosmetic details remain separate from continuous employee, machinery and vehicle simulation. No new per-frame actor is spawned for every paint stripe.

## Verification tools

- `Tools/Unreal/prepare_pavement_materials.py`: creates the four native finish materials.
- `Tools/Unreal/review_pavement_ux_quick.py`: focused rendered inspection of compact panels, default road paint, real handle input, exact fine-grid edits and four-player replication.
- `Tools/Unreal/review_pavement_ux.py`: complete interaction review including paint selection, movement/deletion and fifth-player late join.
- `Tools/Unreal/review_pavement.py`: an unsaved four-peer and late-join review of conversion, curved access, compound merges, cutouts, corner rounding, paint, collider heights, materials and actual pointer-driven drawing.
- `Foundryline.Construction.Pavement.*`: polygon/holes/triangulation, width and direction rules, street gates, authoring/undo, paint destinations and archive round trips.
- `Tools/Validation/Invoke-FoundrylineCheck.ps1`: focused construction or full automated checks.

Generated validation reports are in `Saved/Logs`; rendered review images are in `Saved/Screenshots/Reviews`. A source or editor build alone does not certify a packaged build or the 60 FPS performance target.

## Acceptance record - 2026-10-30

- Development Editor compilation succeeded.
- Full Foundryline automation: 149 passed, zero failed.
- Rendered four-player review plus fifth-player late join: 27 checks passed. This covers shared geometry and paint, merges/cutouts, road-width rejection, generated destinations, actual mouse drawing and curve handles, physical surface/curb heights, and late-join state.
- Rendered images confirm opaque asphalt, paint and beveled curbs. Slab faces use Unreal's front-face winding. HUD projections use the full viewport coordinate system so side borders do not shift selection outlines.
- Straight-road physical width is checked independently of the saved width value. The engine's bundled open-path offset implementation consumes full stroke width.
- Review fixtures use an unsaved starter property. Temporary multiplayer editor preferences are restored after the review closes.

Evidence: `Saved/Logs/FocusedCheckTiming.json`, `Saved/Logs/PavementRuntimeQA.json`, and `Saved/Screenshots/Reviews/Pavement_*00003.png`.

## Simplified planner acceptance - 2026-10-30

- Editor compilation succeeded; final full automation run: 149 passed, zero failed.
- The complete UX review passed 37 checks, including actual mouse-driven edge editing, paint selection/movement/deletion, four-player shared state and a fifth-player late join.
- The final focused rendered review passed 9 checks, including automatic road paint, compact/expanded panels, direct handles, physical surface height, four-player replication and an exact 25 cm grid edit. Its log contains no duplicate-hitbox warnings.
- Cursor snapping and visible pavement grid now share the same snap selection. A regression also preserves building structural snapping.
- The legacy odd-width road regression now selects outside the crossing road's footprint, so random object IDs cannot change which road the fixture resizes.
- Evidence: `Saved/Logs/PavementUXQA.json`, `Saved/Logs/PavementUXQuickQA.json`, `Saved/Logs/FocusedCheckTiming.json` and `Saved/Screenshots/Reviews/Pavement_UX_Close_Final00001.png`.

## Direct-handle recheck coverage

`Tools/Unreal/review_pavement_recheck.py` extends the full rendered UX review with ordinary selection mode and the explicit Select tool. It checks live previews, a 25 cm edge movement, an unchanged snap on mouse release, right-click cancellation without a property mutation, four-player replication and fifth-player late join.

The release handler captures its endpoint before clearing direct-drag state, retaining the pavement plane and fine grid through the complete input event. Direct drags take priority over normal tool preview dispatch, and their placement guide uses the finished pavement plane.

Recheck result (2026-10-30): Editor compilation succeeded, 149 automated tests passed, and all 44 rendered multiplayer checks passed, including ordinary-selection and Select-mode exact handle release, live previews, right-click cancellation, and fifth-player late join. No duplicate UI hitbox warnings were reported. Evidence: `Saved/Logs/PavementRecheckQA.json`, `Saved/Logs/PavementRecheckReview.log`, `Saved/Logs/FocusedCheckTiming.json` and `Saved/Screenshots/Reviews/Pavement_Recheck_Selection00000.png`. Temporary multiplayer editor preferences were restored after the review closed.

## Rounded road entrances

Road records store an entrance radius and connected-end flags. New roads start at a 10 ft radius; the inspector adjusts it in 5 ft steps, from square (0) to 50 ft. Existing records retain square ends until edited. Legacy roads and truck access can be customized into editable road records.

Circular curb returns add asphalt at connected ends. Each return is tangent to the actual road edge and the transverse connection, including on curved approaches. Normal lane width stays unchanged beyond the entrance. Returns must fit the approach and available property; invalid changes are rejected rather than silently changing scale. A larger radius provides room, but the vehicle/path checks still determine whether a particular layout works.

Curbs use joined corner profiles through the curve. Roadside sidewalks are clipped against the actual entrance polygon, removing the pavement obstruction inside the opening while retaining the sidewalk outside it. Road and yard surfaces stay at the existing 20 cm level. Existing sidewalk/crosswalk surface heights are retained.

Adding or trimming a custom yard refreshes connected road ends in the same validated transaction. Moving or bending a road recomputes its own connections. Radius metadata participates in save/load, server validation and replication. Vehicle preview uses the actual entrance shape and refreshes when its radius changes; its local frame supports negative world coordinates.

## Entrance acceptance - 2026-10-30

Editor compilation and all 151 automated tests passed. The articulated-rig regression blocked 28 poses at a square entrance and zero poses with the tested 40 ft circular returns; this is a specific test layout, not a guarantee for every radius/layout. Curved-road returns pass validation after origin normalization, radius metadata survives save/load, and vehicle sweep handles negative coordinates and radius-cache refresh.

All 33 rendered entrance/multiplayer checks passed, including 15/20/25 ft radius edits, four-player shared geometry, late join, and physical sidewalk removal inside the curve. Ray checks measured 20 cm pavement at both the opening and widened turning space, 28 cm retained sidewalk outside the curve and 35.24 cm curb tops. Reports: `Saved/Logs/EntranceRadiusQA.json`, `Saved/Logs/EntranceRadiusReview.log`, `Saved/Logs/FocusedCheckTiming.json`. Reviewed image: `Saved/Screenshots/Reviews/Entrance_Radius_25ft00002.png`. Temporary multiplayer editor preferences were restored after the review closed.

## Multi-point road splines - 2026-10-30

In the office computer's Planning View, open the tool wheel (Tab), choose Roads, then Road Spline or Truck Spline. Click the start and subsequent points to draw a continuous curved road. Enter finishes the visible preview; the Finish Road button commits the clicked points. Backspace removes the last draft point. Right-click/Escape discards the unfinished draft. Up to 32 points are supported, with at least 100 cm (about 3 ft 4 in) between neighboring points.

Road Spline begins with a 24 ft two-way preset and 10 ft entrance returns. Truck Spline begins with a 30 ft two-way preset and 30 ft entrance returns. Both include curbs. The detailed inspector retains one-way/two-way/four-lane and width choices. A wide road does not guarantee a semi can negotiate every curve: use Turning Check to inspect the vehicle sweep and widen tight bends. Rectangular and custom polygon pavement remain available for yards, loading areas and parking surfaces.

Select an existing spline in Edit Road. Drag its visible point handles to reshape it, or use Add Road Point and Remove Point. Existing straight roads and legacy three-control curves retain their original geometry. Snapping uses the actual public-street or owned-pavement boundary; connection guides appear on the preview. Finish or cancel a draft before opening the tool wheel.

The spline is a centripetal Catmull-Rom curve through the clicked knots. Surface, generated paint, curb geometry, preview, collision and vehicle-path centerline derive from that same saved definition. Sampling is bounded to at most 513 points; replicated payloads retain bounded geometry, marking and knot counts. Knot editing regenerates paint while retaining its identities. The saved Spline flag defaults false for older records.

Placement and editing use the existing authenticated company/plant construction requests and server-side geometry validation. Spline state belongs to the shared property; each player's drafting state, tool wheel and vehicle preview remain local. Save/load and late-join snapshots include the spline definition.

Spline acceptance: editor compilation succeeded and the full automation suite passed 152 tests. The final rendered mouse-input review passed all 25 checks, including five-point placement, point dragging, asphalt collision, adding/removing points, distant-click protection, valid paint after edits, draft cancellation, presets, submenu navigation, four-player shared state and fifth-player late join. The drawing hint and tool bar were visually checked for separation. Reports: Saved/Logs/RoadSplineQA.json, Saved/Logs/RoadSplineReview3.log and Saved/Logs/Focused-Full.log. Images: Saved/Screenshots/Reviews/Road_Spline_Hologram00002.png and Road_Spline_Final00002.png. Temporary multiplayer editor preferences were restored after the review closed. No authored level was saved by the acceptance script.

## Loading bay simplification - 2026-10-30

Loading bays now use complete presets: door, grade-level apron, bollards, markings and logistics destination. Custom loading paint is removed from the planner. Existing saved paint remains compatible. See [Loading bay presets](LOADING_BAY_PRESETS.md) for the current workflow and integration limits.
