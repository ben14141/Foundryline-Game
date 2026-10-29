# Agreed startup requirements

## Current source status

Update: FoundrylineEditor now compiles, all eight automation tests pass, and the
initial grass/Lumen/vegetation map plus 50 UI audio assets are saved. Play startup,
grid toggle and wheel zoom were checked in the editor. See BUILD_VALIDATION.md for
the complete current evidence; earlier preparation notes below are historical.

The project layout now follows all 19 system areas from Master Plan sections 20-22.
Grid math, placeable definitions, company/plant build regions, authoritative placement
requests and versioned placement snapshots have been written. Eight Unreal automation
test cases are authored but not compiled or executed. See Docs/ARCHITECTURE.md and
Docs/Architecture/PLACEMENT_ARCHITECTURE.md for boundaries and current limitations.
The Factory package is imported at Content/Factory. No vendor files were modified.
Camera/controller/game-mode source and input mappings now support pan, rotation,
zoom, ground picking, snap selection and a development grid overlay. No first scene,
production hologram material/widget, live navigation or networking transport exists.
Atomic draft plans/cost quotes, preview data/messages, local change notifications
and asynchronous placement save-slot APIs are now written. Rendering/UI integration,
cash transactions, save-file integration tests and all engine execution remain pending.

## Required foundations

- UE5, with C++ foundations and Blueprint/Data Assets for content and presentation.
- Multiplayer is a MUST from the start, including more than two players:
  explicit company/plant ownership, stable IDs, authoritative simulation state,
  validated player actions, and separation of visual presentation from simulation.
  Multiplayer implementation and local playtesting are now explicitly authorized.
  Four-player local testing does not establish a production player cap.
- True 1:1 scale: 1 Unreal unit = 1 cm.
- 1 m structural snapping, 25 cm fine snapping, 5-10 cm precision snapping.
- At least 60 FPS target with Lumen; target hardware and measured acceptance pending.
- Roofs default hidden in top-down view, can be toggled on there, and always show in first person.

## First scene (not implemented yet)

- Flat grass ground with sparse trees and bushes, using Factory LowPoly from the
  user's Fab library where suitable. Check asset dimensions, pivots and collision.
- Leave space for a town, multiple player companies, cooperative supply chains,
  roads, parking, freight and adjacent expansion parcels; exact map dimensions TBD.
- Lumen daylight: directional sun, sky atmosphere, skylight, restrained clouds/haze,
  soft shadows and controlled exposure.
- Camera pan, rotation and broad zoom from close inspection to town overview.
- Scale reference objects and a starter property boundary.
- World grid with separate surface, ownership and occupancy data.
- Cursor selection, cell highlighting, grid visibility and one placement test object.
- Hologram placement previews for walls and other objects, rotation, clearance and
  valid/invalid feedback. Ghost planning before committing construction cost.
- Save/load with stable IDs and versioned state.
- Initial frame-time baseline; 60 FPS has not been verified.

## Movement architecture

Plan for employee paths/material handling, employee car arrival/parking/departure,
freight routing/queues/loading, large-product clearance and town traffic.
Use shared/cached routes, staggered requests, local invalidation and reduced distant
detail. Plan intersections, reservations, spacing and blocked-route recovery.
These are future systems, not features of the blank scaffold.

## Visual direction

Use the Master Plan v1.1 page 2 logo/brand board and pages 29-35 UI guidance.
Charcoal/graphite panels, off-white text, muted steel borders, industrial blue
selection and restrained foundry orange accents. Minimal HUD, bottom construction
toolbar and consistent side inspector. Department identity and status are separate.

## Source reference

Foundryline_Master_Plan_v1.1.pdf is preserved in Docs/Design alongside its extracted
brand board. The original in Downloads has not been modified. Later explicit user
directions in this plan take precedence over conflicting original PDF guidance.

## Current execution constraint

Do not interrupt Claude's other Unreal work. No editor launch, build, shader cook,
asset import or engine/global configuration changes were performed for scaffolding.


Current construction UI/persistence update: see Architecture/PROPERTY_EDITING.md for
selection editing, redo, explicit Property01 disk saves, live restore semantics and
the multiplayer audit. Earlier statements that all practice placements are always
lost now apply only when exiting Play without using Save Property. Multiplayer is
still mandatory; actual cooperative construction synchronization remains pending.

Local co-op testing is now authorized and was performed on 2026-10-26. Independent
first-person/top-down mode states were checked; shared construction synchronization
is still incomplete. See COOP_TEST_REPORT.md. The user explicitly defers safeguards
for who is working on a section; mixed per-player views are required.


Current status supersedes earlier deferred/unsynchronized statements: construction
records now mirror to client-local visuals; host construction and load changes
synchronize. More than two players and many moving objects are requirements.
See SHARED_PROPERTY_TEST_REPORT.md for scoped test evidence and remaining work.


Player interaction follow-up: four-player native-input E door swings, closed-door
collision and open-door traversal now pass with grounded client/server convergence.
See PLAYER_INTERACTION_TEST_REPORT.md; remote latency and rendered acceptance remain.


Nearby draft protection is now authorized: short-lived server footprint claims,
shared-edge placement allowed, cancellation and expiry cleanup. Success banners
are removed; blocked drafts keep clear reasons. See ../Architecture/PROPERTY_EDITING.md and the
DraftClaims section of BUILD_VALIDATION.md. This is separate from full shared
client construction commands and general per-object editing locks.


Current shared-construction update supersedes the earlier reservation and host-only
construction notes: every player can submit construction changes through their
own controller; final server validation prevents conflicts. Temporary draft
reservations were removed at the user's direction. Local five-player acceptance
passed (four initial plus one late join). Save/load is still host-owned; co-op
undo/redo is disabled pending per-player histories. See SHARED_CONSTRUCTION_TEST_REPORT.md.
