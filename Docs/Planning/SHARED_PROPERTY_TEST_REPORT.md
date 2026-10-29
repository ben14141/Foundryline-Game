> Current update: shared client construction is now implemented, and temporary
> draft reservations are removed. See SHARED_CONSTRUCTION_TEST_REPORT.md. Earlier
> host-only/blocked-client conclusions below are historical.

# Shared-property network acceptance - 2026-10-26

This result supersedes the historical failure report below for construction mirroring.

Validated in NullRHI, single-process PIE: one listen-server host plus three clients.
Three players connect initially; a fourth joins after host construction and painting.

- Initial property: all peers match 2,668 generated instances, 18 ceiling fixtures,
  40 parking plants, real door anchors and mesh inventory.
- Host build/paint: revision 17 -> 19, instances 2,668 -> 2,833; record digests,
  meshes, fixtures and door snapshots match across connected worlds.
- Fourth player receives the edited property on late join.
- Direct client construction mutation is rejected without advancing revision.
- Host baseline load: revision advances to 20, instances return to 2,668 on all peers.
- Independent mixed first-person/top-down modes and local roof visibility match.
- Temporary unique QA save is cleaned; Property01 is untouched.
- C++ compilation succeeded. All 23 Foundryline regression tests passed.

Evidence: Saved/Logs/SharedPropertyAcceptance.json and .log;
Saved/Logs/SharedPropertyFixAutomation.log; SharedPropertyNetworkQABuild.txt.
Reproduction script: Tools/Unreal/validate_shared_property.py.

Follow-up: E close/reopen and real client travel through the doorway now pass
in a four-player native-input run. Static construction/local-base exclusion removes
the movement-base warnings. See PLAYER_INTERACTION_TEST_REPORT.md. The earlier
failed interaction fixture and warnings are superseded by that repair. No rendered Lumen
multi-view, WAN, dedicated-server, busy-factory performance or maximum player count
is established. Client construction/save remains host-controlled. Four players is
the tested case, not a design cap.

## Historical pre-fix report

# Shared property replication test - 2026-10-26

Result: FAILED. Read-only local PIE test, one listen server plus one joined
client, on loopback port 17891. Three samples at 12, 22 and 32 seconds agree.
No gameplay implementation changes, construction edits, slot writes or asset saves
were made by this test. Rendering was disabled (NullRHI); this is replication/state
evidence, not a rendered co-op or performance pass.

| Check | Host | Client | Result |
| --- | --- | --- | --- |
| Connected player states | 2 | 2 | Pass |
| Generated mesh instances, including survey markers | 2668 | 0 | Fail |
| Ceiling fixtures | 18 | 0 | Fail |
| Ceiling area lights | 18 | 0 | Fail |
| Parking trees/flowers | 40 | 0 | Fail |
| Runtime construction mesh components | 64 | 7 | Fail |
| Actual door leaves | 4 | 4 | Meshes arrive; transforms fail |
| Construction site service actor | 1 | 0 | Absent on client |

Missing native assets include road ground tiles (64), lane markings (238), curb
pieces (378), roof panels (33), skylights (3), hanging lamp meshes (6), parking
grass meshes (70), small birches (4) and flower clumps (36). Also missing are
1,836 procedural cube instances used for walls, floors, rooms, trim, paving and
survey markings. Full counts are in the JSON report.

The client receives four door leaves and three office props (desk, chair and
flower pot). All seven runtime component records have parent actor anchors at
world origin, while the corresponding host anchors are the actual shop/office
positions (-2800,1700,0) and (-2800,3500,0). Their relative offsets arrive, but the
parent position does not match. Therefore even the received props/doors are not
accepted as correctly placed shared property. Source review additionally found
that the office furniture helper creates personnel-door component instances for
these props; asset-based classification avoids miscounting them as doors.

Whole-factory finish consistency, shared room access/collision, live construction
updates and loaded-property synchronization cannot pass while the base property
geometry/state is absent. They were not exercised as mutation tests here. Edit
locks and who-is-working-on-what safeguards remain deferred by the user.

Evidence: Saved/Logs/SharedPropertyProbe.log and
Saved/Logs/SharedPropertyProbe.json. Probe completed with no Python errors.
Repeatable collector: Tools/Unreal/probe_shared_property.py. Temporary PIE settings
are restored from SharedPropertySettingsBefore.ini before normal editor reopening.
The next implementation step is authoritative property record synchronization,
client visual reconstruction and correct runtime actor/attachment transforms.
