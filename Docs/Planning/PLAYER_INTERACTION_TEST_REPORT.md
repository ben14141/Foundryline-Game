# Player interaction repair - 2026-10-26

Validated with one listen-server host and three clients in single-process NullRHI
PIE. This covers actual native client movement input and replicated door swings;
it is not rendered, WAN or maximum-capacity acceptance.

## Change

Committed procedural construction roots and instance groups are static; hologram
previews remain movable and personnel doors retain their animated movable leaf.
The walk character does not serialize peer-local static construction as a movement
base. Ground collision and normal client prediction/server correction remain active.
Authored ground and genuinely moving platforms keep normal Unreal based movement.

The earlier E test was affected by movement correction against local movable floor
components. With stable world-space movement and a bounds-based approach fixture,
E closes and reopens the real Factory SM_Door_A1 leaf through the owning-controller
RPC and server range/line-of-sight validation. The door state and swing replicate
through stable object/opening IDs; no component reference RPC is required.

## Results

All nine native-input acceptance checks passed:

- Four connected player worlds.
- Client E closes the door on all peers.
- Actual client movement stops at the closed door, x=-2834.21 cm for doorway x=-2800.
- Client E reopens the door on all peers.
- Actual client movement crosses the doorway and reaches x=-2359.64 cm.
- Server receives that movement and reaches the same destination.
- Settled client/server position difference is below 0.001 cm in this local run.
- Character remains grounded at z=112.15 cm on the shop floor.
- Door open rotation matches across peers.

No unresolved movement-base or NetGUID component warnings occur in this run.
Initial fixture setup uses test teleports on the server/client; measured travel
uses native AddMovementInput and the normal movement networking path, without
further teleports during each travel measurement.

C++ compiled successfully. All 23 Foundryline automated regression tests passed,
including added stationary-floor mobility and local-base exclusion assertions.
The preceding property test also passed real door close/reopen, four-player late
join after edits, host save/load resynchronization and independent local roofs.

Evidence: Saved/Logs/PlayerInteractionBuild.txt,
PlayerInteractionAutomation.log, PlayerInteractionAcceptance.json/.log,
and SharedPropertyInteractionPass.json (prior to final local-base exclusion).
Reproduce with Tools/Unreal/validate_player_interaction.py and four PIE players.

Remaining scope: rendered interaction/UI acceptance, remote-machine latency,
packet loss, dedicated servers, simultaneous contention, actual moving platforms,
player avatars and busy-factory performance are not established by these checks.
Client construction/save remains host-controlled as documented separately.
