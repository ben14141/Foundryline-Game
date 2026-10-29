# Shared construction acceptance - 2026-10-26

This supersedes the host-only construction and temporary reservation reports.

## Implementation

The same construction sidebar is available to host and clients. Client tools
create ordinary requests and send them through the owning player controller.
The server applies its shared company/plant context, strict definition catalog,
connected-player/spectator checks, expected revision, geometry and final overlap
validation. Requests commit atomically. Results return to their requesting player;
only failures show a result banner. Previews and selections remain local.

Temporary draft claims, footprint locks, lease timers and heartbeat RPCs were
removed. Two players can preview the same space; final server validation prevents
overlapping committed buildings. There is no automatic retry against changed state.

## Local network acceptance

Saved/Logs/SharedConstructionNetwork.json and .log record 34 passing checks in
single-process NullRHI PIE. Four players start; a fifth joins after construction.
Native input callbacks preserve normal owning-controller RPC callspace.

- All three initial clients place a building; records converge across every peer.
- Client interior paint, resize, movement and demolition reach all peers.
- Both same-space previews are valid without a reservation; simultaneous commits
  produce one winner. Fresh overlap and stale revision requests are rejected.
- Direct client placement-service execution still fails authority validation.
- A forged target in the public-street company is rejected without advancing revision.
- Unknown definitions cannot supply arbitrary construction content.
- One player walks in first person while another edits; roof views stay independent.
- A fifth late join receives the already edited property with matching record digest.

The initial fixture intentionally failed site validation in two places: its first
location was outside the plot and its first move crossed the freight yard. Those
were corrected without weakening placement rules; the final run has no failures.

C++ build passed (SharedConstructionBuild.txt). The final UI-only pending-feedback
and SOLO history labels compiled in SharedConstructionFinalUIBuild.txt. All 24
regression tests passed after those changes in SharedConstructionFinalAutomation.log.
Reproduce the network acceptance with Tools/Unreal/validate_shared_construction.py.

## Scope

Save/load remains host-owned. Network undo/redo is disabled and labeled SOLO until
per-player histories can avoid undoing someone else's edits. Standalone history
continues to work. The starter co-op company/plant is server-assigned; multi-company
membership/permissions UI is not implemented. Core place/paint/resize/move/remove
transport is accepted above; not every room/door/ground-tool UI path was exercised
by this run. WAN latency, packet loss, reconnect, production dedicated hosting,
player-capacity/FPS and busy-factory performance remain unmeasured.


## Rendered client smoke check

A normal rendered PIE client with its background server displayed the full
construction sidebar, enabled build/edit/office/paint tabs and disabled host-save
and SOLO history controls. The client placed the review building; its reliable
reply reported error=0, replies=1 in SharedConstructionClientUI.log. The building
was visible in the client world and no success banner appeared. This is a local
client UI smoke check, not production dedicated-server or WAN acceptance.
