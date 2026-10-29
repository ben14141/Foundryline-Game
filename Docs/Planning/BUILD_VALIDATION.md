# Foundryline build and setup validation

Validated October 24, 2026, after the user explicitly authorized Unreal work.
Project: C:/Users/curly/Documents/Foundryline Game/Foundryline.uproject
Engine: installed Unreal Engine 5.8.2.

## Construction practice update

The editor target rebuilt successfully with the construction site actor and Canvas
HUD. Nine Foundryline automation tests pass, including the starter layout, surface
overlap rejection, parcel boundaries, floor/wall layering, duplicate wall rejection
and undo. Report: Saved/Automation/Construction/index.json.

Rendered Play inspection showed the open-roof shell, loading-door frame, road,
clickable tool selection and grid. HUD font size and daylight preview brightness
were subsequently increased. Construction practice is local and resets on exiting
Play; traffic, cash and multiplayer transport remain deferred. See
Docs/Architecture/CONSTRUCTION_PRACTICE.md.

## Completed

- FoundrylineEditor / Win64 / Development compiled successfully, including UHT.
  Build concurrency was limited to four actions. Engine/toolchain deprecation and
  non-preferred compiler warnings did not prevent compilation.
- Eight Foundryline automation tests passed; zero failed or not run.
  Report: Saved/Automation/Foundryline/index.json.
  Log: Saved/Logs/FoundrylineAutomation.log.
- Startup map created at /Game/Foundryline/Maps/L_StartupSite and set as editor/game
  default. Grass plane is a provisional 600 m square, not the final town size.
- Scene contains sun, skylight, atmosphere, light fog, controlled manual exposure,
  a 100 x 80 m visual property marker, scale references, 24 trees and 36 bushes.
  The boundary is a visual marker, not authoritative land/region initialization.
- Six vegetation meshes were inspected at original 1:1 scale; heights ranged from
  about 80-112 cm for bushes and 335-570 cm for trees. No vendor mesh was modified.
- All 50 Softline WAV files imported as SoundWave assets beneath
  /Game/Foundryline/UI/Audio/Softline. Import commandlet reported zero errors/warnings.
- Foundryline's GUI editor opened the saved map and entered/exited Play mode with
  its own game mode, controller and camera pawn. G visibly toggled the local grid,
  and mouse-wheel input visibly changed zoom. No Play-start errors were observed.
- Rendered daylight was inspected. A Lumen cached-radiance exposure warning was
  corrected using the project-specific r.EyeAdaptation.CachedLightingPreExposure=10
  setting. The warning was absent after applying it. No engine/global config changed.
- The saved editor view was adjusted to an overhead overview. This is an initial
  empty-site scene, not finished world art or an operational factory.

## Separation

All commands specified the Foundryline project path. Authored maps/materials/audio
are inside Content/Foundryline. Original Content/Factory paths were preserved.
No SpellScape project, source, assets or configuration was modified, opened or closed.
No shared processes were terminated. The installed engine/toolchain and normal
engine caches are shared infrastructure; project content/configuration are separate.
No Git push or merge was performed.

## Acceptance still pending

- User visual acceptance and fuller camera/pan/rotation/zoom-limit checks.
- 60 FPS performance measurement on a representative workload.
- Complete construction UI, hologram material and presentation/navigation adapters.
- Cash integration and file-based save/load round-trip tests.
- UI sound audition, mixing and widget binding.
- Multiplayer transport and replication (deliberately deferred).

Earlier source documents describe pre-build status. This record supersedes those
historical uncompiled/test-unrun statements for the existing eight automation tests.

## 2026-10-25 construction kit update

- FoundrylineEditor Win64 Development build succeeded; `Saved/Logs/BuildingWorkflowBuild.txt` records the build.
- All ten Foundryline automation tests passed. Report: `Saved/Automation/ConstructionKit/index.json`; log: `Saved/Logs/ConstructionKitAutomation.log`.
- Coverage includes rectangular buildings, solid wall geometry, exterior openings and window replacement/undo, magnetic additions and shared-wall removal/undo, road profiles and branch connections, parking geometry, 20 cm slabs, save-state opening round trips, ground picking, authority validation, and instanced-material support.
- Visually inspected `L_ConstructionSite` in Play: concrete floor, thick off-white walls, window trim, personnel/garage openings, asphalt, white/yellow paint, Factory sidewalks/curbs, parking bays/arrows, and original-logo construction HUD render distinctly under the saved Lumen daylight setup.
- Fixed missing instanced-static-mesh material usage on the nine kit materials and added regression coverage. Adjusted skylight to 2.0 and manual exposure shutter to 125 at ISO 100, f/16, bias 0.
- HUD Building selection and grid input responded in Play. Automated mouse drags did not yield a confirmed placed rectangle; manual sustained-drag/hologram acceptance remains pending. No claim of a completed mouse interaction acceptance test.
- Scope remains the visual/placement foundation. Roads are straight and orthogonal; curves and an advanced junction editor remain future work. Practice placements reset when Play stops. Multiplayer transport and representative 60 FPS performance acceptance remain pending.

This update supersedes earlier test counts and construction-kit status above.

## 2026-10-26 building height tiers

- Added T1 / 6 m and T2 / 12 m construction presets; existing Tier 1 IDs and starter site remain unchanged.
- Rendering and reserved 3D bounds now use the definition height. Connected sections remove lower shared walls while retaining the high-bay facade above a lower neighbour.
- FoundrylineEditor Win64 Development compiled successfully (`Saved/Logs/BuildingTierBuild.txt`).
- All eleven test cases completed with Result={Success} (`Saved/Logs/BuildingTierAutomation.log`; report `Saved/Automation/BuildingTiers/index.json`). The report separates success-with-warning cases from plain successes.
- New physical line-trace coverage checks the open lower seam, retained upper wall, full 12 m exterior, no roof, standard garage installation and undo restoration/removal.
- Editor relaunched with the updated module. Visual acceptance of the new tall shell and live mouse-drag acceptance are still pending. Crane equipment and usable hook-clearance validation are not implemented.

### Tier 2 high windows (2026-10-26)

Tier 2 automatic windows now have 9 m sills and 10.8 m heads above the slab; Tier 1 retains its original elevation. Explicit saved sill metadata defaults to the old 1 m height when absent. Build succeeded (`HighWindowBuild.txt`); all eleven automation cases passed (`HighWindowAutomation.log`, report `Saved/Automation/HighWindows`), including high-window elevations and serialization preservation. Visual editor acceptance of the tall shell remains pending.

### Connected Tier 2 starter sample (2026-10-26)

Added the 16 x 12 m high-bay extension to the starter site. Build succeeded and all eleven automation cases passed (TierExpansionBuild.txt, TierExpansionAutomation.log; report Saved/Automation/TierExpansion). Inspected the generated extension in Play: double-height shell, high exterior windows, shared lower opening and retained upper transition wall render correctly. Mobile screenshot: Saved/Screenshots/Sharing/Foundryline_Tier2_Expansion.jpg. This is a visual foundation; structural engineering/crane equipment and manual mouse-drag acceptance remain separate outstanding work.

## 2026-10-26 enlarged site, adjustable columns and trim

- Authored 48 x 24 m Tier 1 plus 48 x 36 m Tier 2, 128 m road, 64 x 48 m parking with extra bank/aisles, and separate small/truck driveway connections. Game-mode site uses expanded owned parcel; compact regression fixture is independent.
- Added metal wall caps, 60 cm concrete base trim, regular Tier 2 pilasters and per-section 4/6/8 m bay adjustments through validated ownership/revision actions, undo and saved metadata.
- Added a purpose-built raised truck entrance with 5 x 5 m clear opening. Vendor door/cap pieces were measured and preserved rather than stretched.
- Win64 Development build succeeded (DetailedSiteBuild.txt). Twelve automation cases pass, including DetailedLargeSiteAndTruckClearance (DetailedSiteAutomation.log; Saved/Automation/DetailedSite).
- New tests check large layout records/dimensions, semi opening kind, a 4.9 x 4.9 m collision sweep through the finished doorway, solid wall above it, open building connection, actual column-count changes, spacing validation, undo, saved spacing and rejection of foreign-company edits.
- Visual acceptance and screenshots recorded separately below. No live semi turning test, crane hardware, structural engineering or 60 FPS measurement is claimed.

Live editor check: enlarged building shells, continuous cap trim, concrete base band, both entrances, road/sidewalk connections and four parking rows rendered in Play. Selected Columns in the HUD and clicked the Tier 2 interior; status changed to 8 m and the visible column spacing widened. Cancel removed the planning overlay. Screenshots saved as Foundryline_Detailed_Factory.jpg and Foundryline_Expanded_Site.jpg under Saved/Screenshots/Sharing. Live click-drag rectangle placement remains an earlier separate acceptance item; this check confirms the new column adjustment interaction.

## 2026-10-26 sidebar, full-height shared-wall removal and planted parking edges

Win64 Development build succeeded (SidebarLandscapeBuild.txt). All twelve tests passed (SidebarLandscapeAutomation.log; report Saved/Automation/SidebarLandscape), including collision traces confirming no upper shared wall or floating cap, decorative grass instances, instanced green material compatibility, and existing truck-clearance/column/ownership/save checks.

Live Play inspection confirms the logo-led left sidebar, wrapped instructions, grouped icon cards, orange selected state, Cancel behavior, continuous floor across both tiers without the former dividing wall, and green curb edging along the parking exterior. No instanced-material usage fallback errors appeared in the checked editor log. Full overview and phone-readable sidebar crop saved under Saved/Screenshots/Sharing as Foundryline_Sidebar_Green_Parking.jpg and Foundryline_Sidebar_Closeup.png.

Detailed 3D grass is distance-culled; the overview shows the persistent green verge surface. Full grass LOD performance and general landscape authoring are not acceptance claims for this pass.

## 2026-10-26 complete holograms and planted island on the map

Build succeeded (HologramIslandBuild.txt); all thirteen tests pass (HologramIslandAutomation.log; report Saved/Automation/HologramIsland). AllPlacementPartsAreHolograms checks both building tiers, three road profiles, parking, personnel/garage/semi doors, columns and a blocked building draft. Every preview mesh material is translucent, preview collision is disabled, and placed-object count/revision stay unchanged. Generated road/parking details now participate in the ghost preview, and column adjustments preview before clicking.

Added a raised grass-filled island bounded by Factory curb modules between the central parking rows, with open space at either end. Inspected it in the running map together with existing outer planted edging. Screenshots: Saved/Screenshots/Sharing/Foundryline_Planted_Island_Map.jpg and Foundryline_Planted_Island_Closeup.png. Far views show the green surface; detailed grass is distance-culled as before. Manual sustained rectangle-drag acceptance and representative performance testing remain separate outstanding checks.


## 2026-10-26 construction editing, junctions, offices and real door leaves

Win64 Development build succeeded (Saved/Logs/OfficeConstructionBuild.txt). All seventeen automation cases passed (Saved/Logs/OfficeConstructionAutomation.log; Saved/Automation/OfficeConstruction). New coverage checks move/resize/door relocation/removal undo, retained identities, rejected overlaps, greenery undo, same-plant X crossings, a single asphalt surface at crossings, curb openings, zebra placement/undo, a six-metre parking entrance, replacement ownership and tombstone restrictions, open office zoning, room bounds/overlaps, office ghosts, actual native door mesh scale, clear room door paths and serialized room/opening metadata. Existing thirteen cases still pass. OfficeFloor is game-owned and instancing-enabled.

Live editor/input acceptance is recorded separately after inspection. No packaged, live network, traffic or performance acceptance is implied by these tests.

Live editor acceptance: corrected press/release handling successfully created a new room by dragging, then Z removed it; a full 14 x 10 m building draft showed its blue complete-shell hologram and committed on release. Repeating the same drag was blocked without adding a building. The Edit tab moved that section to a new position and Z returned it; nearest-corner resize changed its proportions and Z restored them. A final undo removed the temporary building, leaving the authored showcase clean. Build/Edit/Office navigation and wrapped instructions were inspected. Office floor, 3 m partitions, interior glazing, open native office leaves, exterior shop personnel leaf, crosswalk and ramp presentation render in Play. No construction-related material fallback errors appeared in the checked GUI log. Screenshots: Foundryline_Office_Details.jpg, Foundryline_Office_Rooms_Phone.jpg, Foundryline_Office_Controls_Phone.png, Foundryline_Office_Roads_Phone.jpg. These are under Saved/Screenshots/Sharing. Individual room/window placement, resizing, demolition restoration and junction collision paths also retain automated coverage; no multiplayer transport or 60 FPS measurement was performed.


## 2026-10-26 exterior office correction and furniture scale

ExteriorOfficeBuild.txt confirms Win64 Development compilation. All seventeen tests passed (ExteriorOfficeAutomation.log; Saved/Automation/ExteriorOffice). Office tests now check rejection of shop-interior offices/rooms, detached annexes, rooms below 4 m, and shop removal that would orphan an annex. The sample retains three rooms, doors, windows, ghost isolation and serialized state; real desks/chairs and door leaves use native unit scale. Existing construction tests pass with the ninth showcase object representing the exterior annex. Native furniture bounds are logged in OfficeFurnitureBounds.json. Final transaction and single-action preview paths enforce the same dependencies.


## 2026-10-26 original logo, animated sidebar and required office doors

BrandedOfficeBuild.txt confirms Win64 Development build; all seventeen tests pass (BrandedOfficeAutomation.log; Saved/Automation/BrandedOffice). The office test additionally rejects loss of either required entrance, traces through the outside and shop doorway openings, and checks solid partition beside the shop doorway. OriginalLogoImport.log confirms the dedicated T_FoundrylineLogo import. Vendor artwork and geometry remain unchanged.

Live Play inspection confirms the 28 x 24 m exterior annex, cleared shop interior, its retained office-side separating wall, native-scale desks/chairs/plants in all three rooms, exterior entry and shop entry. The imported original logo displays as a dedicated full-image header. Build and Office tabs work; hover produces the animated card accent/chevron without activating the tool. Larger card labels/details and category navigation fit the sidebar. Screenshots under Saved/Screenshots/Sharing: Foundryline_Exterior_Office_Branded.jpg, Foundryline_Exterior_Office_Phone.jpg, Foundryline_Refined_UI_Phone.png and Foundryline_Hover_UI_Phone.png. Door/path collision is automated; live network transport and target-FPS acceptance remain outside this check.


### Office walls, manual windows and paint - 2026-10-26

FoundrylineEditor Win64 Development compiled successfully (OfficeColorsBuild.txt).
All 17 Foundryline automation tests passed, exit code 0 (OfficeColorsAutomation.log,
Saved/Automation/OfficeColors). Regression checks cover single shared wall geometry,
manual office exterior glazing, separate paint channels, ownership denial, invalid
color rejection, no navigation invalidation, undo and serialized color preservation.
The older wall test now checks the 29 cm body plus the separate interior finish.
Live editor/UI review is recorded separately below after observation.


Final walk/doors validation: FoundrylineEditor Win64 Development succeeded
(WalkDoorsBuild.txt). All 18 Foundryline automation tests passed, exit 0
(WalkDoorsAutomation.log, Saved/Automation/WalkDoors), including the new
WalkAndPersonnelDoor test: character possession, 1.8 m capsule, outdoor spawn,
return to build camera, intermediate door angle, full open/close, distance rejection
and disabled idle ticking. No multiplayer session or FPS acceptance is claimed.

Live PIE: selected office survives the Edit-to-Paint transition. Blue interior
changed room partitions and inside faces while the exterior stayed cream, then
Undo restored defaults. Walk button entered first person; F restored the same
construction view. Mouse look worked; aiming at the native office leaf displayed
E Close Door. E visibly swung it closed; after aiming at the closed leaf, E Open
Door appeared and E visibly reopened it. Screenshots are saved in
Saved/Screenshots/Sharing/Foundryline_Office_Paint.jpg, Foundryline_First_Person.jpg
and Foundryline_Door_Interaction.jpg, with phone crops alongside them.


Roof validation (2026-10-26): game-owned skylight prepared from vendor geometry;
eight glazing polygons separated, original Factory package untouched. RoofBuild.txt
records successful editor compilation; RoofAutomation.log records 18/18 tests,
exit 0. Tests cover default-hidden roofs, top-down toggle, mandatory first-person
roofs, restored top-down preference, native skylight scale, no construction picking
collision, translucent panes and opaque frames. Live PIE confirmed the toggle and
first-person roof ceiling; returning to top-down restored Hidden. Daylight passes
through wall glazing as visible sun patches within wall shadows. The first-person
windowless office corridor remains dark without electric lighting, deferred by user.
No photometric or hardware performance acceptance is claimed. Final roof-only upper
cladding closes stepped roof seams while preserving open connected ground floors.

Roof transition finish fix (2026-10-26): replaced dark trim on the upper height-step wall with the building exterior finish, separate interior finish and narrow roof-edge trim. Roof instances now group by mesh and material so infill cannot inherit the wrong finish. RoofWallFixBuild.txt: compilation succeeded. RoofWallFixAutomation.log: all 18 tests passed, exit 0. Visual review follows separately.
Live PIE confirmed the height-step wall now shows the light exterior finish with narrow dark trim; roof toggle still works. Screenshot: Saved/Screenshots/Sharing/Foundryline_Roof_Wall_Fixed_Phone.jpg.

Branded facades (2026-10-26): restored high wall glazing and clipped column bay details above adjoining lower roofs; openings use saved window records and native centimetre dimensions. Step glass remains translucent and does not cast opaque shadows. Defaults now use charcoal exterior/roof, orange trim, off-white interior. BrandedFacadesBuildFinal.txt succeeded; Initial BrandedFacadesAutomation.log: 17 passed; paint undo test still expected the previous cream default. Updated that assertion to the default color contract. Final rerun pending.
Final BrandedFacadesAutomationFinal.log: 18/18 passed, exit 0; build succeeded.
Live PIE confirmed charcoal roofs and exterior, orange trim, light interior faces, restored high windows and clipped column sections at the roof transition. Roof toggle works. Screenshot: Foundryline_Branded_Facades_Phone.jpg.

Full-height columns and palette swap (2026-10-26): Tier 2 pilasters now reach 1200 cm above the slab with caps at wall top; roof-step sections reach the upper roof. Centers shift beside window frames when a nominal column bay would cover glazing. Column preview no longer filters on the old 880 cm height. Defaults swapped to orange exterior/roof, charcoal trim, retaining off-white interior. FullHeightColumnsBuild.txt succeeded; FullHeightColumnsAutomation.log passed 18/18, exit 0, including column bay count/undo checks updated to full height. Editor reopened; visual acceptance remains separate.

Dark roof finish (2026-10-26): roof panels now use the charcoal trim channel, matching edge infills and exterior trim; walls remain orange. Skylight glass and frames retain their materials. DarkRoofBuild.txt confirms successful compilation. No additional automation run for this material assignment.

Adaptive facade layout (2026-10-26): AdaptiveFacadeBuild.txt succeeded, AdaptiveFacadeAutomation.log passed 19/19 tests, exit 0. New AdaptiveFacadeBays checks all 4-64 m lengths in metre increments at 4/6/8 m preferences, symmetry, fixed-size glazing/column clearance, manual garage retention, stable repeated reflow and office exclusion. Existing resize, column adjustment, undo and serialization tests passed. Live visual inspection separate.

Starter property (2026-10-26): StarterPropertyBuildFinal.txt succeeded. StarterPropertyAutomation.log passed 20/20 tests, exit 0. New StarterPropertyAndProtectedStreet checks all eight starter surfaces, one small shop, dedicated freight yard and approaches, setback rejection, property boundary rejection, town-street selection denial, minimum truck apron and snapshot ownership/setback restore. StreetAssets.json confirms native 400 cm surface tiles; authored vendor materials preserved. Traffic and swept-path validation remain deferred.
Live PIE: starter shop, two separate street entrances, car parking, pedestrian strip, six trailer staging bays, clear manoeuvring apron and dashed property limits rendered. Roof toggle confirmed; Truck Access and Semi Yard cards fit and appear in Roads sidebar. Factory street surface uses native pack finish. Screenshot: Saved/Screenshots/Sharing/Foundryline_Starter_Property_Phone.jpg. No moving traffic or vehicle turning simulation tested.

Loading dock and office (2026-10-26): LoadingDockOfficeBuildFinal.txt compiled successfully. LoadingDockOfficeAutomation.log passed 20/20 tests, exit 0. LoadingDockPlacementQA.log additionally passed the expanded starter test checking all dock preview materials/collision, wall-snap relocation and undo. Starter now includes 16 x 12 m office, furnished private room, both required connections and a saved Walkway item. Dock deck/opening dimensions, collision below deck, clear raised loading opening and saved restoration validated. Visual inspection recorded separately below.
Live PIE inspection confirmed the raised bay platform/canopy, internal ramp, attached office with furnished private room, shop doorway and external doorway. Current cutaway screenshot: Saved/Screenshots/Sharing/Foundryline_Loading_Dock_Office_Phone.jpg. Geometry and snap/undo are verified; active freight handling remains future simulation work.

Playable interior lighting and level access (2026-10-26): supersedes the raised dock/ramp described above. Loading openings now have level access, with road, driveway, parking, walkway and shop at 20 cm; collision traces verify transitions within 0.3 cm. Parking landscaping adds small birches and flower clumps with distance culling. Midday daylight uses a high sun and slightly warm 5400 K color; Lumen remains enabled.

Automatic ceiling fixtures now use shadow-casting area lights: evenly spaced hanging shop fixtures for both tiers, compact ceiling panels independently distributed across office rooms, open areas and hallways. Lamp faces have emissive diffusers, roofs retain charcoal tops with light interior undersides, and shared office/shop walls use interior finishes. Histogram exposure supports outdoor and enclosed interior views. Roof visibility controls both fixture geometry and light components.

InteriorLightingFinalBuild.txt confirms successful compilation; InteriorLightingFinalAutomation.log records 21/21 successful tests, exit 0. Coverage includes ceiling fixture scale/output, roof visibility, level loading access, transitions, placement and existing construction behavior. Native PIE screenshots visually reviewed: Foundryline_Tier1_Lit_Interior.png, Foundryline_Office_Workroom_Lit_Interior.png and Foundryline_Tier2_Lit_Interior.png under Saved/Screenshots/Sharing. The enclosed office visibly receives ceiling illumination and all three views show glowing lamp faces. No FPS measurement or multiplayer runtime acceptance is claimed. Review cameras are temporary PIE state; normal startup is restored after capture.

Property editing and persistence (2026-10-26): adds a dedicated Edit inspector with selected type, real dimensions/orientation, blue hover and orange selection/corner markers, compact two-column actions, imported logo, animated hover/press states and persistent history/save controls. Added validated rotation and duplication, fresh copy IDs with retained finish/room metadata, rotated section and fixed-width road resizing, redo and redo-branch invalidation. Existing road junction, level transition, office attachment, shared-wall and undo tests remain in the suite.

Save/load now integrates the explicit asynchronous slot service with the HUD. Procedural kit dimensions reconstruct from a strict allowlist, and complete snapshots validate before replacing active state. Live restore advances revision and emits presentation updates. A failed file load keeps current construction. Native PIE verified Save Property, unsaved-state feedback, a temporary blue interior finish, two-click load confirmation, restored original cream finish and cleared history. Initial full suite passed 23/23; final authority/UI build and suite recorded below after completion.

Multiplayer review: authoritative ownership and stale revision checks retained; added duplicate permission/stale-request assertions and client UI restriction. Complete client construction commands, procedural state synchronization, late join, per-player conflict-aware undo and multi-client acceptance remain pending. See Docs/Architecture/PROPERTY_EDITING.md for explicit networking boundaries. No multiplayer playtest or FPS acceptance claimed.
Final PropertyEditorBuildFinal.txt: compilation succeeded. PropertyEditorAutomationFinal.log: 23/23 passed, exit 0, including fresh-session disk restore, atomic live restore, new copy identities, redo, rotated resize, office disconnection rejection and duplicate ownership/permission/stale-request checks. Multiplayer transport and multi-client play remain unverified.
Final native PIE visual check confirmed the selected 24 x 18 m shop inspector, orange perimeter/corner markers without a selection grid, accessible action buttons and persistent footer. Screenshots: Saved/Screenshots/Sharing/Foundryline_Property_Editor.png and Foundryline_Property_Editor_UI.png (UI crop). Final editor left in normal playable PIE; no camera tick disabled.

Co-op and independent views (2026-10-26): tested a local listen server plus one client in PIE. See COOP_TEST_REPORT.md for the current findings and limits. Added Blueprint-callable normal view toggles and restricted network roof application to the local controller so one player's possession cannot change the host's cutaway. Mixed first-person/top-down state works, including both players walking and returning to their own cameras. Procedural construction replication fails: client receives some door meshes/actor stubs but zero generated construction instances and zero ceiling area lights. Client construction remains unavailable. Initial rendered two-view run hit a GPU timeout; state/replication evidence uses NullRHI and is not visual/performance acceptance. Edit safeguards are deferred at the user's request.
Final CoopViewBuildFinal.txt: compilation succeeded. CoopViewAutomationFinal.log: 23/23 regression tests passed, exit 0. Temporary co-op editor settings restored byte-for-byte from CoopSettingsBefore.ini before reopening normal Foundryline. Final network report: CoopProbe.json, completed with no probe errors and native-timer view actions.
Shared property focused test (2026-10-26): FAILED. SharedPropertyProbe.log/SharedPropertyProbe.json contain three stable host/client comparisons with no probe errors. Host has 2668 generated instances, 18 fixture meshes/area lights and 40 parking plants; client has zero. Four real door leaves and three office props arrive, but all parent anchors are world origin instead of host shop/office anchors. No gameplay code or map/save assets changed. Temporary editor settings restored; see SHARED_PROPERTY_TEST_REPORT.md. Rendering disabled; live construction/load mutation and visual acceptance remain untested.


## Shared-property repair and four-player scope - 2026-10-26

Compiled successfully (SharedPropertyNetworkQABuild.txt). All 23 Foundryline tests
passed in SharedPropertyFixAutomation.log, exit code 0. NullRHI listen-server plus
three clients matched construction records, local instances, fixtures and plants,
including late join after host edits and host baseline load. Mixed local views and
roof states passed. E interaction did not demonstrate a state transition; generated
floor movement-base resolution also produces warnings. These are open acceptance
items, not passes. Rendered performance and production player capacity are unmeasured.
See SHARED_PROPERTY_TEST_REPORT.md for exact scope.


## Player interaction acceptance - 2026-10-26

PlayerInteractionBuild.txt: C++ build succeeded. PlayerInteractionAutomation.log:
23/23 passed, exit 0, including static floor/local-base regression checks.
PlayerInteractionAcceptance.json/.log: all nine four-player checks passed using
native movement input, replicated E close/open, closed-door collision, open-door
traversal and grounded client/server convergence. No unresolved movement-base or
NetGUID component warnings. See PLAYER_INTERACTION_TEST_REPORT.md for exact scope;
this supersedes the open door/floor findings above. No rendered/WAN/FPS claim.


## Placement feedback - 2026-10-26

PlacementFeedbackBuild.txt: compiled successfully. PlacementFeedbackAutomation.log:
24/24 tests passed, exit 0. Added real-site feedback coverage for valid placement,
overlap, outside plot, too-small drafts and persistence of rejection across hover.
PlacementFeedbackNetwork.json/.log: four-player NullRHI check passed host accepted
commit, precise overlap rejection, all three clients denied with host-controlled
reason and unchanged revisions, and matching committed records across four peers.
The normal rendered editor sidebar was visually checked: red NOT PLACED label and
overlap reason fit the card and remain readable with the full footer visible.
No map/save files were changed. Shared client construction remains separate work.


## Quiet success and draft footprint contention - 2026-10-26

Removed the CHANGE APPLIED confirmation. Failures retain their reason. This
supersedes the earlier successful-confirmation description above.
DraftClaimsBuild.txt: C++ build succeeded. DraftClaimsAutomation.log: 24/24 tests
passed, exit 0, with the feedback expectation changed to quiet success.
DraftClaimsNetwork.json/.log: all ten four-player NullRHI checks passed: one
winner for overlapping drafts, losing owner's reason, host commit denied without
revision mutation, touching-edge drafts accepted, replicated claims on all peers,
owner-only cancel, automatic expiry, later use of released space, and existing
construction overlap rejection. Native editor test input queues owning-controller
RPCs with pointer-preview ticks disabled to hold deterministic endpoints.
No rendered simultaneous-draft, WAN/packet-loss or explicit disconnect-transport
acceptance is claimed. Lost-heartbeat expiry is verified. Shared client construction
commits remain host-controlled; draft transport is implemented separately.


## Shared client construction and reservation removal - 2026-10-26

SharedConstructionBuild.txt: compiled shared construction transport. Final UI-only
pending and SOLO-label polish compiled in SharedConstructionFinalUIBuild.txt.
SharedConstructionFinalAutomation.log: 24/24 passed, exit 0.
SharedConstructionNetwork.json/.log: all 34 checks passed with four initial players
and a fifth late join, including client placement, paint, resize, move, remove,
independent mixed views, convergence and server rejection of stale/overlapping,
foreign-ownership and unknown-definition requests. Temporary reservations and their
network/timer implementation were removed. Success stays quiet. Save/load remains
host-owned and co-op undo/redo remains disabled pending per-player history.
See SHARED_CONSTRUCTION_TEST_REPORT.md for exact evidence scope and limitations.


Rendered follow-up: SharedConstructionClientUI.log records one accepted client
placement (error 0, reply 1) with a background PIE server. The client viewport was
visually checked with the full construction sidebar, host-save restrictions and
SOLO history labels visible; no success banner. No map or save asset was written.


## Discrete camera wheel zoom (2026-10-26)

Camera zoom now consumes MouseScrollUp/Down press events, rather than a continuous
legacy mouse-wheel axis. Wheel input over the top-down HUD is excluded. The
FoundrylineEditor build passed and 2/2 Foundryline.View tests passed, exit 0, in
Saved/Logs/ZoomInputFinalAutomation.log. WheelZoomStops exercises wheel events,
settling, idle stability and opposite-notch restoration. The initial test setup
was corrected to mark its controller local. Handoff uses start_map_playtest.py,
which starts normal Play without camera-driving review/test callbacks.
