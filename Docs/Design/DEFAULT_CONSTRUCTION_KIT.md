# Default construction and expansion kit

User-directed baseline, October 25, 2026. This specification replaces the initial
per-wall practice workflow. A capability being specified here does not mean it is
implemented or visually accepted.

Scope clarification: current work is the visual and placement foundation only.
Traffic AI, production, economy and multiplayer behaviour are parked. Existing
authority checks are retained; no new simulation work is part of this milestone.

Current implementation now includes rectangular drag placement and blue previews,
adjacent snapped additions with shared-wall removal, automatic exterior window
bays, separate personnel/garage openings, three marked road widths with edge
connections, native Factory paving/curb modules, and a basic painted parking lot.
The HUD uses the original plan logo artwork and brand palette. The daylight map
is L_ConstructionSite. Read the current practice guide for remaining limits.

## Common rules

- Real centimetres, 1 m layout snapping; detail offsets use the established fine grids.
- Road and foundation slabs extend from ground Z=0 to Z=20 cm. Their drive/walk
  surfaces meet flush. Raised slabs have visible solid sides, not floating planes.
- Concrete exterior walls are solid, initially 30 cm thick. Roofs stay invisible.
- Planning uses a blue translucent hologram, red when invalid, dimensions and
  a clear failure reason. A failed operation leaves the world unchanged.
- Authority, ownership, stable IDs, revision validation and save records remain
  separate from input and presentation. Multiplayer transport is a separate milestone.
- Construction clears vegetation under its footprint. Demolition/undo must eventually
  restore the appropriate terrain/vegetation state, not leave accidental bare patches.

## Building creation

Choose Building, press at one corner, drag a square or rectangle, release to commit
the slab and complete exterior shell together. Dimensions remain visible while
dragging. Initial workshop wall height is 6 m; office/shop height variants belong
in the catalog rather than scaling every imported asset arbitrarily.

After placement, add a personnel door and garage/loading door separately. Door
previews snap to wall runs and reserve corner/jamb space. They cut actual openings
through the concrete and have appropriate frames, lintels, thresholds and collision.
Opening/closing animation and route clearance need their own acceptance checks.

## Expansion and shared walls

Choose an existing building and drag an addition. The addition belongs to that
building, not a visually overlapping second shell. Geometry derives from a union
of occupied foundation cells/rectangles: exposed edges create exterior wall runs;
shared edges do not create duplicate or hidden interior exterior walls.

Recompute only affected wall runs. Preserve IDs for unaffected doors/windows and
interior equipment. If an addition would consume an existing exterior opening,
highlight the conflict before commitment; never silently delete it. Allow the
player to retain a former wall as a partition where requested. Keep ownership,
clearance, expansion pricing and undo atomic.

This requires a building topology model; stacking independent boxes is not an
acceptable implementation of expansion.

## Shop windows

Window modules belong only on exposed exterior wall runs. Initial default spacing
is approximately one shop-window bay every 4 m where there is room. Use a consistent
sill, head height, trim and corner margin. Skip doors, garage openings, corners and
structural piers. Provide automatic exterior-window placement and manual add/remove
or spacing adjustment. Windows do not become interior-wall windows when expanding;
resolve or remove consumed bays as part of the expansion preview/transaction.

## Road catalog and network

Initial catalog: 4 m service road, 8 m two-lane road, 14 m four-lane road.
Widths include deliberate lane and shoulder dimensions. Default markings include
edge paint, opposing-flow separation and dashed same-direction lane dividers.

The complete kit requires straight runs, 90-degree bends with useful vehicle
turning radii, T-junctions, four-way junctions, end caps, lane/width transitions,
driveway connections, crossings and road upgrades/demolition. Connected junctions
must replace paint/curbs in the intersection rather than overlap independent strips.

Road topology owns connection nodes, lane direction and connection compatibility.
The rendered asphalt/paint consumes that topology. Later traffic uses the same
network, including turns, intersection reservations and queues. Rendering a road
does not establish traffic/pathfinding functionality.

## Curbs, sidewalks and access

Curbs follow exposed road/parking boundaries with straight, corner and end pieces.
Sidewalks have their own raised height above the road slab. Suppress curbs across
driveway/parking entrances and add deliberate curb cuts or ramps at crossings and
personnel access. No continuous curb may block the garage driveway. Junction and
expansion edits regenerate only affected edge pieces.

## Parking lots

Drag a parking rectangle, select bay orientation and one- or two-sided aisles.
Include standard/accessible bays, painted dividers, aisle direction, end islands,
curbs, entrances/curb cuts and a pedestrian route to a personnel door. Keep stalls
and driving aisles as data, not only painted meshes. Validate manoeuvring room,
road access, ownership, building clearance and reserved loading access.
Employee parking, reservations and vehicle routing are later simulation integrations.

## HUD

Top-level categories: Buildings, Roads, Parking, Openings, Demolish.
Show catalog variants, dimensions, raised height, window settings, selected building
for expansion and actionable errors. Offer drag preview, cancel, and transaction
undo. Use readable labels and a consistent industrial dark/blue/orange style.
Production controls must eventually include persistence and sound feedback.

## Delivery gates and current work

1. Whole rectangular building/road drag, matching raised slabs, solid shell,
   marked straight road variants, blue preview, real door openings, authority/undo
   regression coverage. This is the current implementation slice.
2. Building topology/expansion, exterior-only window generation and opening migration.
3. Connected road topology, bends/junctions/transitions, curbs/sidewalks/curb cuts.
4. Parking generator connected to roads and pedestrian access.
5. Save/load UI and migration, door operation, traffic/pathing adapters and multiplayer.

Each gate needs a rendered close/overhead review, collision/access checks, atomic
undo/restore coverage and representative performance measurement. Source compilation
alone is not visual acceptance. Do not describe gates 2-5 as completed by gate 1.

### Height presets

Tier 1 retains the current 6 m shell for offices and light workshops. Tier 2 adds a 12 m high-bay shell for future crane-equipped manufacturing. Both share the 20 cm floor level and can connect with an open lower interior and an upper wall over the shorter section. Crane hardware and crane clearance validation are future work. These are construction presets; no progression unlocks are implied.


## Office and editing extension (2026-10-26)

Office tab: Open Office Area -> Draw Room -> Office Door / Interior Window. Areas are open-plan designations inside a host shell. Rooms use 15 cm partitions at 3 m height, 1 m structural snap, hidden ceilings, framed 2 x 1.8 m windows (1 m sill), and native Factory glazed office leaves in 1 x 2.5 m openings. Shop personnel openings are 1.2 x 2.5 m and use native Factory SM_Door_A1. Existing garage and semi openings remain separate. Doors display open; interaction/animation remains future work.

Edit tab exposes selection, section movement, nearest-corner resizing, exterior door relocation, section removal, parking grass toggle and crosswalk editing. Z undoes. Build adds a dedicated 6 m parking entrance; perpendicular same-plant road crossings share their asphalt surface, interrupt through paint and open connecting curbs. Square right-angle connections are supported; curved spline roads are not yet implemented.


### Exterior office rule supersedes shop-interior conversion

Office additions must be outside and directly connected to an existing shop wall (at least 4 m shared wall). Minimum annex 8 x 8 m; minimum individual room 4 x 4 m. The sample provides 7 x 8 m offices and a 16 x 10 m meeting room in a 28 x 24 m annex, with native-scale Factory furniture to judge usable space. Rooms inside the shop are prohibited. Keep real desk, chair, decor and circulation dimensions; grid snapping never rescales assets.


### Shared office walls and building finishes - 2026-10-26

Office rooms snap flush to the annex perimeter and reuse its walls. Adjacent rooms
share one partition. The showcase now has 9 x 10 m, 7 x 10 m and 18 x 12 m rooms
against the shell, with no perimeter setback. Required outdoor and shop doors remain.
Office windows are manual-only, including the showcase and office resize defaults.
Office Window places glazing on room partitions or directly on an open-office exterior.

Each shop/office record stores separate exterior, interior and trim colors. Defaults
use the original Foundryline palette: F7F5F1 warm off-white walls, 1F2327 charcoal trim;
F07A2F orange is available alongside six other palette choices. Select a building
in Edit, then open Paint and pick a swatch for each finish. Changes apply to that
building only and can be undone. Interior partitions use the interior finish.
Exterior walls remain 30 cm total: a 29 cm body plus an adjoining 1 cm interior
finish, with no overlapping wall volumes. Concrete foundations remain concrete.

SetBuildingColors is an authoritative action with ownership/revision checks and
finite 0-1 channel validation. It preserves collision and does not invalidate
navigation. Colors are SaveGame properties; transport and practice save UI remain
outside this slice. Vendor assets are untouched.


### First-person site inspection - 2026-10-26

The top-right Walk button or F switches from construction to a 1.8 m character
(1.7 m standing eye height). WASD walks, mouse looks, F restores the existing
build camera. The showcase starts the character 2 m outside the office entrance.
Construction controls are hidden while walking; a crosshair and E prompt replace them.
Personnel doors, including office room doors, use a smooth half-second 90-degree
swing. Aim at the leaf within 3 m and press E to open/close. Garage doors retain
their existing presentation. Door collision follows the leaf. Idle doors do not tick.

Door use validates authority, distance and line of sight on the server; walking
uses CharacterMovement and the door state is replicated. These are prepared
contracts, not a completed multiplayer session/transport integration. Runtime door
state survives local construction visual rebuilds, but is not saved across Play.
Practice doors initially remain open for the existing site review.


### Roof visibility and daylight openings - 2026-10-26

Latest user direction supersedes the earlier roof-free requirement. Building roofs
default hidden in top-down view. The top-right Roofs button toggles them there.
First person always shows roofs; returning to construction restores the previous
top-down preference. Roof panels, skylights and edge infills are grouped together,
including after construction edits/undo/repainting. This is local presentation and
does not mutate construction data or navigation. Roofs never block construction picking.

Factory 4 m roof panels retain native scale. A game-owned SM_Skylight_400 copy
retains the vendor geometry and separates its eight broad glazing polygons from
the opaque frame. The native panel opening is retained: no slab beneath a skylight.
Partial perimeter modules use precisely fitted 20 cm infills, without resizing
vendor meshes. Skylights repeat at 12 m spacing on complete modules.

M_WindowGlass uses two-sided translucent surface lighting with 0.16 opacity and
0.12 roughness. Wall glazing uses it too and does not cast opaque shadows. Roof
frames remain opaque with shadows; the skylight component is excluded from stale
opaque distance fields/ray-tracing geometry, allowing daylight through its opening.
No electric lights were added. Lumen/daylight visual review is separate from tests.
Vendor assets remain unchanged. Roof preference and runtime door state reset after Play.
