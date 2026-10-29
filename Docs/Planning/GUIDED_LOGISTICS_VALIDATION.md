# Guided logistics UI - 2026-10-26

User-authorized scope: explain and test Entrance -> Loading -> Exit through a clear
sidebar, filled access/parking layers and a complete vehicle/trailer preview.

## Implementation

- Logistics/FoundrylineLogisticsTrip owns pure draft selection and complete-trip
  validation; UI controller owns per-player workflow, async query and playback.
- UI/FoundrylineLogisticsHUD draws filled green truck permissions, blue parking
  bays, amber staging/door markers, numbered gates and solid vehicle silhouettes.
- Existing server-validated truck-zone metadata remains the shared/saveable layer.
  Trip assignments and playback are local drafts; no delivery scheduling added.
- Both trip legs must pass. Departure uses the exact final arrival pose. The
  three-second stop, pause and restart operate only on the local preview timeline.
- HUD pointer exclusion now includes the top-right controls, preventing a click
  that opens logistics from also painting/selecting the map.
- Gate candidates are derived from actual paving/street contacts and cached by
  placement revision. Shared layout edits invalidate checked routes and refresh
  or clear affected gate/target selections. No new vehicle state replication.

## Runtime checks

FoundrylineEditor Win64 Development compiled successfully with UE 5.8. Existing
HUD float-conversion and engine deprecation warnings remain.
Saved/Logs/GuidedLogisticsAutomation.log: 28/28 Foundryline cases passed, exit 0.
New GuidedTrip checks missing/invalid entrance selection, actual loading-door clicks,
shared two-way gates, car/lorry destinations and complete arrival/departure paths.
Both profiles passed the real starter layout with exact pose continuity at the
stop. Approximate headless trip planning was 7.7 s for the semi and 1.4 s for the
car; these are async query times, not a frame-rate or fleet performance result.

## Multiplayer acceptance

Saved/Logs/GuidedLogisticsNetwork.json: completed, 14/14 checks passed, no script
exceptions. Four initial players plus a fifth late join, local PIE in one process.
A client painted shared access, selected the actual entrance/loading door/exit and
successfully ran a 1,271-pose two-leg preview. Other players retained empty drafts.
A second client erased access: all peers converged and the old preview invalidated.
A third client selected an independent car entrance. The fifth player received
shared access zones while starting with no other player's draft or overlay.

## Rendered acceptance

One rendered PIE client and a background server exercised the new sidebar and
complete semi preview. Verified solid articulated cab/trailer while reversing,
filled truck access and car parking, labels at real gates/doors, and arrival/exit
pass indicators. Presentation cleanup separated route outline/colour passes to
remove striped joints, expanded staging labels and fixed pending-button contrast.
Final screenshots are captured from the rebuilt version after that cleanup.

The previous truck-zone save/load checks remain relevant: this change does not
alter their record format or server transport. No purchased assets, map packages,
user saves, commits or other projects were changed. No production traffic or FPS
acceptance claim is made.

Final rebuilt screenshots: Saved/Screenshots/Guided_Logistics_Sidebar.png and
Guided_Logistics_Map.png. Observed readable solid routes, expanded staging labels,
arrival/stop/departure playback and pause switching to Play Preview.

## Additional visual polish

Added explicit active-step headings/progress, candidate hover prompts/halos and
local arrival/departure path filters. FoundrylineEditor compiled successfully;
Saved/Logs/LogisticsPolishAutomation.log reran the four Driving cases successfully.
No planner, placement, save schema or server RPC behavior was changed in this pass.

Rendered polish acceptance: Arrival and Departure buttons each hid the other
path while playback continued. Loading-door hover showed the larger CLICK label
and white halo. Progress strip/step prompt and selected-destination halo rendered.
Evidence: Saved/Logs/LogisticsPolishUI.log and
Saved/Screenshots/Logistics_Polish_Arrival.png.
