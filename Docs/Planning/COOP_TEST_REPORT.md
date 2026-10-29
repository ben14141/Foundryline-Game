> Current update: shared client construction is now implemented, and temporary
> draft reservations are removed. See SHARED_CONSTRUCTION_TEST_REPORT.md. Earlier
> host-only/blocked-client conclusions below are historical.

> Latest interaction follow-up: doors and client traversal now pass four-player
> native-input acceptance; movement-base warnings are resolved. See
> PLAYER_INTERACTION_TEST_REPORT.md. Earlier open door/walking findings are historical.

> Update 2026-10-26: construction mirroring now passes a local four-player test,
> including late join after edits and host load resynchronization. Earlier missing
> client geometry findings below are historical. See SHARED_PROPERTY_TEST_REPORT.md
> for current evidence and unresolved door/walking/rendered acceptance.

# Co-op test report - 2026-10-26

Scope: one local listen server and one joined client, using Unreal PIE network
worlds in one editor process. This is not an internet-hosting or dedicated-server
acceptance test. No object edit locks or collaboration safeguards were added.

## Verified behavior

- The client completes the local connection handshake; both worlds report two
  PlayerStates.
- Client first person with host top-down works at the possession/state level.
- Both players can possess separate first-person characters concurrently.
- Host can return to top-down while the client remains in first person.
- Both return to their own top-down cameras.
- Remote possession no longer changes the host's roof visibility: roof cutaways
  and roof-associated lights follow the local player's view. Standalone retains
  its normal single-player roof behavior.

The controller's normal ToggleWalk and ToggleRoofs entry points are now Blueprint
callable, and roof application ignores non-local network controllers. Actual
mixed-view checks run ToggleWalk from native game-world timers. Calling RPCs
straight from an editor Python callback is not valid network evidence: Unreal's
GAllowActorScriptExecutionInEditor forces local callspace during those calls.

## Failed / incomplete behavior

The client receives runtime actor stubs and seven replicated mesh components (four door leaves and three office props),
but no generated building/road/parking mesh instances or ceiling area lights. The
host has 2,668 generic-actor mesh instances (including survey markers) and 18
ceiling area lights; the client has zero of each. The client has no BuildSite
actor or placement record catalog. Actor replication flags alone are insufficient
for the procedural construction representation.

Client construction UI deliberately displays its unavailable state. Shared
construction, finishes, landscaping, rooms, property load synchronization and
late joining are not accepted. Door interaction inside a shared building cannot
be accepted while the client lacks the building and its collision geometry.

Required next work: publish authoritative placement snapshots/deltas and render
those locally on clients, route editing commands through authenticated server
requests, and repeat co-op tests. Per-player edit locks/conflict safeguards are
explicitly deferred by the user.

## Rendering and evidence

The first D3D12 rendered attempt completed joining, then hit a GPU timeout while
running both views. Networking/state checks were repeated with NullRHI to separate
replication from rendering. There is no two-view visual or 60 FPS acceptance.

Evidence: Saved/Logs/CoopAcceptance.log and Saved/Logs/CoopProbe.json. The temporary
probe uses loopback port 17891 and leaves map assets/save slots untouched. Before
normal reopening, restore Saved/Config/WindowsEditor/EditorPerProjectUserSettings.ini
from the captured CoopSettingsBefore.ini to remove transient PIE overrides.
Tools/Unreal/probe_coop.py retains the sampling and native-timer sequence.
Final source validation: CoopViewBuildFinal.txt compiled successfully; CoopViewAutomationFinal.log passed all 23 regression tests, exit 0. Temporary PIE settings were restored before normal editor reopening.
