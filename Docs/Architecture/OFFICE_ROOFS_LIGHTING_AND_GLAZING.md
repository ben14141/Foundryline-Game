# Office roof, lighting and manual glazing

All dimensions are authored in real centimetres; descriptions use feet and inches.

## Recessed office lighting

Office ceilings use standard 2 x 4 ft white recessed panel fixtures, separate from the hanging industrial shop fixtures. Each has a white housing, luminous diffuser and a real downward-facing rectangular light (5,000 lumens, 4,000 K). The housing is recessed into the ceiling slab, with no suspension rods. Rooms and remaining circulation space receive their own layout, nominally about 10 ft apart. Placement moves fixtures away from skylights and partitions. Fixtures are batched; light components do not tick.

## Skylights

Industrial skylights retain their native 400 cm square dimensions and center within roof cells, rather than recurring on an arbitrary tile modulus. Offices use compact 3 x 4 ft framed/curbed units. The roof slab is cut around each opening: glass is not placed over opaque roof. Rooms receive their own roof apertures with edge clearance, while narrow corridors omit oversized skylights. Glass does not contribute an opaque distance-field or ray-tracing blocker.

## Manual office glass walls

The Office palette adds GLASS WALL. Click an exterior office wall to fill the available span beside existing doors. A blue placement hologram previews that exact span; existing doors are preserved. Glazing is floor-to-ceiling with fixed-height frames and evenly spaced mullions at no more than about 4 ft. Corners keep structural posts. Smaller manually placed windows in that span are replaced. Offices receive no automatic exterior windows.

Glazing is a saved opening on the building, selectable/movable/deletable using the common planner tools, with undo. The server checks ownership, revision, office-only use, panel dimensions, corner clearance and separation from other openings. Existing enum values remain unchanged. A door cannot be inserted through an existing manual glass panel; remove/reconfigure that panel first. Resizing must keep manual openings within the new wall bounds.

## Multiplayer and roofs

Layouts derive from replicated building dimensions, rooms and openings. All players receive the same geometry and lights. Visibility stays player-local: hidden by default in planning, shown in first person. Changing one player's roof toggle does not change other players' preference. No authored map is modified by review scripts.

## Validation

Validated 2026-10-30:

- FoundrylineEditor Win64 Development compiled successfully (`Saved/Logs/OfficeRoofBuild4.log`).
- Full automation: 157 passed, zero failed (`Saved/Automation/Focused-Full/index.json`). Includes office/shop fixture dimensions, one real light per fixture, visible diffusers inside open housings, roof show/hide, exact roof coverage with genuine apertures, skylight/fixture separation, and manual glazing/entrance preservation/undo. The previous fixture test was updated to distinguish recessed office housings from native hanging shop fixtures.
- Native rendered review: all nine checks passed (`Saved/Logs/OfficeRoofQA.json`, `OfficeRoofReview2.log`). Four players received matching layouts after client-built office/room/glass changes; a fifth joining later received identical lights and glazing. The actual planning input path placed a glass wall from a collision-free hologram. First-person startup showed roofs; one player's cutaway choice remained independent.
- Inspected `Office_Recessed_Ceiling00001.png`, `Office_Roof_Skylights00001.png` and `Office_Glass_Wall00001.png` under `Saved/Screenshots/Reviews`. The first render caught a diffuser hidden by the housing; the corrected housing uses four open rim rails with its back above the diffuser. A regression check verifies the opening.
- Review construction was unsaved. Normal editor play preferences were restored after review processes exited. No FPS or maximum-player-count acceptance is claimed.
