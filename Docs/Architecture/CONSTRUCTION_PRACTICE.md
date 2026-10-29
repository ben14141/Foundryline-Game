# Construction kit: visual and placement foundation

Current map: `/Game/Foundryline/Maps/L_ConstructionSite`. Play starts a workshop,
marked access road, garage driveway and parking lot. Roofs are never generated.
The master specification is Docs/Design/DEFAULT_CONSTRUCTION_KIT.md.

## Controls

- Building: hold left mouse at the first corner, drag a rectangle, release to build.
  Drafts show the entire blue translucent building; invalid drafts are red.
  Dimensions use the 1 m grid, with sides from 4 to 64 m.
- Additions: drag beside an existing building. Edges magnetically snap within
  1.5 m; adjoining sections remove both shared walls. Windows and doors on those
  now-internal edges disappear from presentation. Undo restores the old exterior.
- Personnel door / Garage door: point at an exterior wall and click. Real wall
  openings are cut; corner margins and overlapping openings are checked.
- Roads: select Service 4 m, Two-lane 8 m or Four-lane 14 m, then drag a straight
  run. When starting or ending inside an existing road, the branch trims to its
  edge. Edge/center paint and sidewalk/curb cuts update around connections.
- Parking: drag at least 6 x 12 m. Generates painted bays, circulation arrows and
  perimeter curbs; sufficiently deep rectangles have two rows and a driving aisle.
- Right-click cancels; Z undoes the last operation; G toggles the local grid.
  WASD/arrows, Q/E, middle mouse and wheel retain the camera controls.

## Dimensions and presentation

Road and foundation slabs both extend from Z=0 to Z=20 cm. Solid walls are 30 cm
thick and 6 m high above the slab. Cube primitives are explicitly dimensioned;
snap resolution never resizes authored vendor meshes. Exterior window bays are
2 m wide and 1.8 m high with a 1 m sill, spaced about every 4 m where they fit.
Personnel and garage tools replace any affected automatic window bays.

Personnel doors are presented open; garage openings have jambs, a lintel and an
overhead roller casing. Opening/closing animation is not implemented. Window glass
uses a tinted reflective material rather than a full glass/interior art treatment.

Sidewalk paving uses the original Factory SM_Lane_100 mesh at 1:1 scale (2 m x 1 m,
10 cm thick). Curbs use SM_Border_100 at original scale. Supports touch the ground;
paving and curb tops meet approximately 18 cm above the road surface. Sidewalks
break at adjoining driveways/parking/building footprints. Vendor source assets
and their material packages are unchanged.

All structural parts use material-grouped instanced meshes. Construction clears
grass under committed footprints. Materials explicitly support instanced meshes
so they do not fall back to the engine default material during Play.

## UI and daylight

The HUD samples the original logo lockup from the imported brand-board texture,
without redrawing it. It uses charcoal #1F2327, graphite #4B5258, steel #9AA0A6,
fog #F7F5F1, industrial blue #2F6B8F and orange #F07A2F, with chamfered buttons.
The lower tool strip leaves the world view clear; the header reports dimensions,
connection state and errors. No fake economy/traffic controls are shown.

Lumen GI/reflections, movable daylight sun, real-time skylight and manual physical
exposure are configured in the construction map. Performance remains unmeasured.

## Boundaries

The placement service retains authority, ownership, revisions and atomic changes.
Openings have stable IDs and versioned save records. Additions are section records;
exposed facade geometry derives from adjoining sections. This is the visual and
placement foundation, not a completed building-management simulation.

Roads currently support straight orthogonal runs and edge branches, not arbitrary
curves, lane transitions or a complete junction editor. Existing runs cannot be
crossed by an overlapping single drag. Road topology/traffic, pedestrian routing,
parking reservations, economy and multiplayer transport are not implemented.

Practice edits reset on leaving Play. The HUD is not connected to save-slot APIs.
Undo does not regrow cleared grass until restarting the session. Door metadata and
window layouts have a tested serialization/restore path, but this is not a save UI.

## Building height tiers (2026-10-26)

- Tier 1 (`Building`): existing 6 m walls above the 20 cm slab, for offices, simple production and light workshops. This is a height preset, not an economic progression gate.
- Tier 2 (`Shop2`): 12 m walls above the same slab, reserving volume for overhead handling equipment and larger manufacturing work. Actual crane equipment, structural load certification and usable crane-hook clearance are not implemented; 12 m shell height is not a promised hook clearance.
- Both use the rectangle hologram, 1 m snapping, automatic exterior windows, separate personnel/garage installation and invisible roofs. Garage opening dimensions remain unchanged.
- Either height can join the other. Shared walls open below the shorter section's top; the taller section retains its upper wall at the step. Undo restores the previous perimeter.
- Height is part of each immutable placeable definition's 3D footprint/clearance. Existing Tier 1 definition IDs remain unchanged. The new `Shop2_<width>_<depth>` IDs preserve the chosen height in placement records/snapshots without a new save schema.
- The HUD offers T1 / 6 m and T2 / 12 m as separate tools. A selection does not retrofit an existing building.

Tier 2 automatic exterior windows now use a 9 m sill and 10.8 m top above the finished floor, leaving a 1.2 m wall band above them. Tier 1 keeps its 1 m sill. The saved opening record explicitly carries sill height, with a 1 m default for older records. Window height remains 1.8 m. High-window bays may still be replaced by a door bay in the current kit.

### Starter Tier 2 expansion

The practice site now includes a 16 x 12 m Tier 2 addition along the north edge of the 24 x 16 m Tier 1 starter shell. It uses 12 m walls, high windows at 9-10.8 m, and the shared 20 cm slab elevation. The connecting lower wall is removed; the height transition remains above it. This sample is regenerated each time Play starts.

## Detailed large practice site (2026-10-26)

The game-mode practice site is now larger, at authored real centimetre dimensions:

- Tier 1: 48 x 24 m, 6 m walls; connected Tier 2: 48 x 36 m, 12 m walls.
- Road: two joined 64 m runs, 128 m total, with an 8 m two-lane carriageway and sidewalks. A second 8 m wide driveway reaches the truck opening.
- Parking: 64 x 48 m, 2.5 x 5 m painted spaces, a central back-to-back bank and two circulation aisles.
- The construction parcel is 240 x 200 m. Instanced grass, bushes and trees are cleared only where construction/sidewalk footprints require it. Undo does not regrow vegetation during the practice session.
- Wall-top metal caps and a 60 cm concrete base band follow the exposed perimeter. Base trim and columns leave door openings clear; additions remove trim at shared walls.
- Tier 2 has regularly spaced steel wall pilasters below the high-window band, ending 8.8 m above the floor. These are a visual foundation, not an engineered crane-support frame. No free-standing internal column grid is added.
- Select Columns in the HUD, then click inside a Tier 2 section to cycle 6 -> 8 -> 4 -> 6 m bay spacing. Each section stores its own setting through validated authority, ownership, revision, undo and snapshot paths. Only 4, 6 and 8 m presets are accepted.
- Select Semi 5x5 m to install the purpose-built raised rolling-door frame. Its clear opening is 5 m wide and 5 m high above the finished floor, with jambs/lintel outside that opening. The existing smaller Garage tool remains. Truck opening dimensions do not guarantee turning/approach routing; no truck simulation or door animation is added.
- Vendor inspection measured the B1 leaf at roughly 1.52 m wide by 4.29 m high and the wall cap at 20 x 20 x 700 cm (a vertical strip). These original assets are not stretched into truck doors or horizontal caps. Current caps, base and truck-door geometry are game-owned procedural parts.
- Camera starts farther out to frame the enlarged site; zoom changes view only. Compact automation fixtures remain available independently of the game-mode showcase.

Reproducible vendor measurements: Tools/Unreal/inspect_building_details.py -> Saved/Setup/building_detail_assets.json.

## 2026-10-26 sidebar and open height transitions

Construction now uses a left sidebar instead of the header/bottom toolbar. It follows the original logo's cream paper, charcoal, steel and orange palette, angular orange strokes and chamfered tool cards. Tools are grouped into Buildings, Roads & Grounds, Doors & Access, and Structure, with vector icons, dimensions, selection state, wrapped status text and separate Cancel/Undo controls. Sidebar and input exclusion scale together with viewport height.

Connected Tier 1/Tier 2 sections now remove their shared wall across the full height, including the formerly visible upper transition wall and cap. This deliberately supersedes the earlier retained-upper-wall presentation. Exterior wall dimensions, high windows and roof-free gameplay remain unchanged; undo still restores the exterior boundary.

Parking now includes 1.2 m grass-filled verges along free outer edges, with original Factory straight curbs. Free-edge samples avoid adjoining construction/roads. The inner curb remains at parking edges; road-facing vehicle entrances stay open. Original Factory SM_Grass_02 is instanced at native scale, with 60-100 m culling and no collision/shadows. Flat green surface remains beyond that distance. Measured grass mesh bounds: about 74 x 71 x 43 cm. These are automatic visual edge treatments, not a standalone landscape placement/editor system; landscape routing and complete curb authoring remain future work.

## Full placement holograms and visible parking island

Every current placement tool now uses translucent, non-colliding ghost geometry for all generated parts: full building shells/windows/trim/columns, road paint/sidewalks/curbs, parking paint/planted borders, personnel/garage/semi frames and leaves, and column-bay adjustments. Valid drafts are blue and invalid drafts red. Solid objects appear only after commit. Road/parking previews no longer stop after showing only their slab. Column hover previews the next bay setting without changing records; clicking applies it.

Large parking lots now include a raised grass-filled island between their central back-to-back parking rows, bounded by Factory curbs. An 8 m gap remains at each end. Parking rows are moved clear of the planted strip. The large starter site regenerates this island on Play, so it can be inspected in the map, rather than existing only as an available feature.


## 2026-10-26 editing, road connections and office interiors

The sidebar has Build, Edit and Office tabs. Pointer press/release is sampled in PlayerController::InputKey while SceneViewport still holds each event's cursor position; initial mouse-down capture is enabled. This replaces deferred left-button action bindings that lost fast drag endpoints.

Edit supports section selection, metre-snapped moves, corner resizing of building/parking rectangles, personnel/garage door relocation within the same section, whole-section removal and parking greenery toggles. Resizing holds the opposite corner fixed, regenerates automatic windows and preserves manual openings; invalid door bounds or office layouts reject the resize. Moves carry a section's offices with it. Z cancels an active drag and undoes the last commit. Demolition undo restores the original server-owned record and identity from a tombstone. Old and new bounds both refresh adjoining visuals. Authority, company/plant ownership, current revision and geometry are checked before committing. Replication/RPC transport remains separate future work.

Roads can form same-plant perpendicular X intersections, T branches and square right-angle connections. Collinear overlap remains blocked. Asphalt overlap is rendered once, junction paint stops at connected approaches and curbs/sidewalks open at connected roads and parking entrances. The new Parking Entrance tool draws a 6 m driveway. Crosswalk places/removes a metre-snapped zebra crossing with 2 m curb ramps; Z restores the previous crossing layout. These are orthogonal construction surfaces, not a traffic simulation or curved spline road editor.

Office workflow: drag Open Office Area inside a building to designate open office floor; draw rooms inside that zone; click room partitions to place Office Door or Interior Window. Areas need at least 4 x 4 m, rooms 3 x 3 m, and a 1 m margin from the host shell. Rooms use 15 cm partitions, 3 m height, top trim and skirting that leaves doors clear. Interior windows are 2 x 1.8 m with 1 m sills. Ceilings remain hidden. Adjacent rooms receive matching opening cuts atomically. Office metadata is local to the host building and is included in snapshot serialization and undo. The large practice site includes an open office area, two offices and a meeting room with doors/windows.

Actual vendor leaves replace the old personnel-door cubes. Shop personnel openings are now 1.2 x 2.5 m to fit Factory SM_Door_A1 at its native approximately 1.12 x 2.45 m bounds. Offices use SM_Door_B2_Right, an approximately 0.88 x 2.38 m glazed leaf, inside 1 x 2.5 m openings. Leaves are displayed open at 90 degrees, without stretching; frames are authored to the opening. Vendor assets are unchanged. Measured bounds and material slots are in Saved/Logs/FactoryDoorBounds.json, reproducible with Tools/Unreal/inspect_door_assets.py. Door animation/interaction is not yet connected.

Source organization: FoundrylineConstructionEditing.cpp owns edit action composition; FoundrylineOfficeConstruction.cpp owns office/room commands; PlacementSubsystem owns validation and transactions; BuildSite owns presentation. Practice placements still reset after Play. Clearing original map foliage currently does not restore that foliage after moving/removing construction within the same session.


## Exterior-only office annexes and real furniture space (2026-10-26)

This supersedes interior shop office conversion. Office tab now draws a separate Office annex outside the shop footprint. It must share at least 4 m of an existing same-company/plant non-office building wall; overlap with shops, roads or parking is rejected. Office placement is at least 8 x 8 m, with 3 m exterior walls and the same 20 cm foundation elevation. Office definitions require their authoritative annex classification, preventing an unchecked Build request from bypassing the connection rule. Final transaction validation and preview validation preserve attachment when moving, resizing or removing either building.

Rooms can only be drawn inside an office annex. Their minimum is now 4 x 4 m, with 15 cm partitions and 3 m height. The sample office is a 28 x 24 m exterior addition on the shop's east side, replacing all former shop-interior rooms. It contains two 7 x 8 m rooms and a 16 x 10 m meeting room, plus generous open floor/circulation space. Native Factory desks, chairs and plants furnish the sample as a scale reference; ordinary user-drawn rooms start empty. Native desk-set bounds are approximately 318 x 228 cm, chair bounds 87 x 87 cm. All remain at scale 1; measured inventory is reproducible with Tools/Unreal/inspect_office_furniture.py (Saved/Logs/OfficeFurnitureBounds.json). These are visual layout checks, not an automated furniture/navigation planner. Doors and room windows retain their placement and undo workflow.


### Required office entrances

Each new annex includes two real OfficeDoor openings: one through the retained office-side partition into the shop and one on an exterior wall. The shop-side shared shell is removed so it cannot block the connection; the office-side wall remains solid away from its doorway. Validation requires both access roles, including for replacement/removal/undo actions, so the only shop connection or outside door cannot be removed. Collision traces verify both clear passages and a solid partition beside the shop doorway. Other room doors remain independent.


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

Roof-only upper cladding closes the height steps above adjoining lower roofs. It hides with roofs; the shared ground-floor opening remains unobstructed.
