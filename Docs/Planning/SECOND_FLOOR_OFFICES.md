# Second-floor offices and staircase

Implemented 2026-10-31. Unreal centimetres remain the simulation units; player dimensions are shown in feet and inches.

## Player workflow

1. Use the office computer to enter property planning.
2. Open Build / Office and choose Turning staircase or Straight staircase.
3. Place the entire staircase hologram inside an office. R rotates it. Keep desks, room partitions, machinery and people outside its footprint.
4. Placement adds an upper office storey over that office, with a real stairwell opening, stairs, midlanding, upper guardrails and a roof.
5. Use GROUND and OFFICE 2 at the top of the planner to switch the editing layer. Office rooms, manual windows and $500 desk/computer/chair sets support the second floor.
6. Leave the computer and walk upstairs in first person. Both floors remain physically present, with roofs visible. Different players can walk and plan independently.

Placing the staircase tool again repositions the stairs in the same office, subject to clearance. Undo removes a newly added floor only when its upstairs furniture and occupants are clear. A two-storey office cannot be moved, rotated or removed underneath a controlled pawn or upstairs desk.

## Physical layout

- Finished floor heights: 20 cm and 340 cm, giving 320 cm (about 10 ft 6 in) floor-to-floor rise.
- Turning stair footprint: 300 x 512 cm (about 9 ft 10 in x 16 ft 10 in). Straight footprint including both landing clearances: 136 x 764 cm (about 4 ft 6 in x 25 ft 1 in).
- Both styles have 130 cm (about 4 ft 3 in) clear walking width and eighteen risers. Turning stairs have two flights and a 130 cm midlanding; straight stairs have one continuous 504 cm run and 130 cm entry/exit clearances.
- Continuous ramp collision beneath detailed visible steps avoids repeated collision bumps. The visible treads do not block the character separately.
- Upper floor panels are split around the opening. No invisible slab blocks the staircase.
- Upper perimeter windows are manual. Exterior doors on the upper floor are rejected because this version has no exterior balcony/landing system.
- Placement previews have no player collision.

## Office finish

Both stair layouts use timber-look treads, enclosed off-white risers and side fascia, a finished underside and clear glass balustrades with dark satin handrails. Glass/fascia are visual finishes; the explicit hidden walking ramps and existing rail collision are independent, so presentation changes do not alter player clearance. Each layout has its own catalogue render and hologram.

## Data and multiplayer

The existing owned office placement record saves the upper-floor flag, stair origin, quarter-turn orientation and stair style. Existing saves default to the turning style. Room and opening records save their floor index. Missing fields retain a single-floor office. No migration to a different office ID is required.

Construction uses existing validated server actions and company/plant permissions. Rooms, windows, doors, desk purchases and placement synchronize through existing property/factory replication. Floor selection and roof visibility are local presentation choices. Generated upper geometry has explicit lifetime ownership and is destroyed when its parent is rebuilt or removed.

Upper desks retain the ordinary paid workstation record and real floor elevation. Upstairs and downstairs desks may share XY coordinates, but same-floor overlaps and stairwell placement remain blocked. This does not add office-worker payroll or employee navigation jobs.

## Verification

- `Tools/Validation/Invoke-FoundrylineCheck.ps1 -Profile Full` runs the full suite, including stair geometry/collision, floor hole, furniture support, ownership, save round-trip and cleanup.
- `Tools/Unreal/run_office_storeys_review.ps1` runs an unsaved four-player listen-server session. It restores the project's prior play settings after the run.
- The native review uses a remote client to construct and furnish upstairs, then a different client walks up and down using actual character movement input. Initial positioning is a test fixture; upward traversal is not teleported.
- Reports: `Saved/Logs/FocusedCheckTiming.json` and `Saved/Logs/OfficeStoreysNativeQA.json`, `Saved/Logs/OfficeStoreysStraightNativeQA.json` and `Saved/Logs/StairStylesAcceptance.json`. Screenshots are under `Saved/Screenshots/Reviews/OfficeStoreys_*`.

Final verification on 2026-10-31: Development Editor build succeeded, all 200 automated checks passed, plus 57 native checks across separate four-player straight and turning stair sessions. Both styles were walked upstairs and downstairs with real player input. The remote pawn climbed and descended the full stair route using real CharacterMovement input. An upstairs desk replicated to all peers and charged the shared balance exactly once. Ground and upper planning views remained independent while another client walked. Screenshots were inspected, and the prior per-project play settings were restored byte for byte. No user map or player save was overwritten.

This verifies a local four-player listen-server session, not an Internet latency test or production player/FPS limit.
