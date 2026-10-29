# Construction editing and property saves

The Edit sidebar uses the imported Foundryline logo, cream and charcoal panels,
orange active controls and animated hover/press feedback. A selected-object card
shows type, real footprint and orientation. Blue hover outlines and orange
selection outlines/corner markers identify the affected object without covering it.

- Select: click an owned section, including with no active construction tool.
- Move: drag a section with metre snapping. Within 2 m of a neighbour's edge it
  slides flush or into line; it never resizes and never snaps into an overlap.
- Resize: drag its nearest corner; the opposite corner stays fixed. Grab the middle
  half of a wall instead to move only that wall. Only the dragged edges magnetize.
  Doors, windows and office rooms keep their world positions when the south or
  west side moves, and a whole-floor open office grows with the building. Rotated
  sections work; road runs retain their authored width while changing length.
- Rotate: click a section for a 90 degree turn, keeping its lower world bounds.
- Duplicate: drag from a section to a clear new position. Finish, rooms, openings,
  furnishing and grounds metadata are copied; object, room and opening IDs are new.
- Move door: drag an opening to a valid wall on the same section.
- Delete: click a section. Undo restores its identity and details.
- Undo / Z and Redo / Y stay available in the fixed footer; a new action clears
  the redo branch. History is limited to 100 recent edits and resets after loading.

Each edit is validated by the authoritative placement service, including ownership,
revision, overlap, property bounds and office attachment/access. Invalid ghosts
cannot commit. UI and outlines do not own construction data. Construction records mirror to clients; each player can send construction
commands through their owning controller. Property saves remain host-controlled. See the current multiplayer status below.

Save Property writes the current property to the project-local Property01 slot.
Saving again updates that slot. Load Property replaces the active property;
unsaved changes require a second click on Confirm Load. Current construction stays
intact if the file is missing, unreadable or fails schema/ownership/geometry checks.
The UI shows busy, saved and unsaved states. Saves are explicit; startup still opens
the starter scene, so use Load Property after entering Play to resume your save.

Runtime procedural definitions are reconstructed from a strict kit/dimension
allowlist on fresh loads. The file cannot supply arbitrary meshes or construction
rules. Versioned records include IDs, colors, office rooms, openings, furnishing,
landscaping and road crosswalks. Lights and other generated presentation rebuild
from these records. Loading advances the current action revision, clears undo
history and notifies presentation only after validating the complete snapshot.
Office validation is independent of serialized host/annex order.

Default storage: Saved/SaveGames/Foundryline_Property01.sav. This is one local
property slot, not a cloud save or per-player multiplayer persistence system.

## Multiplayer implementation status (2026-10-26)

Construction is shared. Each client uses the same construction sidebar and local
blue/red previews, selection and camera/roof preferences. Placement, movement,
resizing, painting, demolition and the other construction request types go through
an owning-player-controller reliable RPC. Read-only preview staging can run in a
client world, but Execute/ExecuteBatch still require server authority.

Requests contain serializable construction data and the expected property revision;
they cannot provide a trusted company/plant context, mesh path, price or permission.
The server checks connected nonspectator membership in the shared starter company,
strict generated-definition catalog, request size/type, identity, ownership,
geometry, current revision and final occupancy. A failed action has no partial
construction changes. Door relocation retains its two-operation atomic batch.
Only failures produce a transient result message. Success remains quiet.

There are no hologram footprint reservations, leases, heartbeats or plot locks.
Players can preview the same area. If both commit it, the server accepts one and
rejects the conflicting/stale request. A stale result asks the player to retry;
there is no automatic replay of an action against changed property state.

BuildSite replicates versioned records, regions and stable object/opening door
state. Peers build their own meshes, instances, fixtures and planting from those
records. Generated visual components never become network identities. Initial
snapshots and committed edits use the same mirror, including late join.

The current cooperative starter company/plant is server-assigned to connected
players. Company selection, permission management and separate player histories
are future work. Save/load is host-owned. Co-op undo/redo is explicitly disabled
and labeled SOLO, so a player's old history cannot rewind another player's work;
standalone undo remains available. The old reservation implementation and test
are retired. See SHARED_CONSTRUCTION_TEST_REPORT.md for current evidence.

Construction replication is change-driven ordinary replicated state, not a
production-scale moving-object transport. Continuous cars, employees and machinery
must use separate relevance/update budgets. Do not rebuild property on movement
ticks. Dedicated servers, WAN latency/loss and capacity/FPS remain unaccepted.

### Generated floors and character movement

Committed construction instance groups and their roots are static. Placement moves
replace these visuals rather than moving a physics base. Preview groups remain
movable, and swinging personnel doors remain movable.

FoundrylineWalkCharacter excludes only static, FoundrylinePropertyVisual components
from Unreal based movement. Ground collision and authoritative movement validation
remain enabled; corrections use world positions without serializing peer-local
floor components. Authored ground and actual moving platforms retain Unreal's
normal movement-base path. Do not extend this exception to vehicles or moving
machinery. The native editor QA input helper feeds ordinary AddMovementInput; it
does not bypass movement RPCs or server validation.


### Placement feedback

The sidebar distinguishes READY TO PLACE, PLACEMENT BLOCKED and NOT PLACED. Successful changes
do not show a confirmation banner. Preview validation shares the authoritative placement rules; committed
feedback comes from the actual Execute/ExecuteBatch result. A failed click outcome stays
visible for 3.5 seconds even if later hover previews change. Changing tools clears
it. Hover ghosts remain local and use blue/red validity; validity is part of the
visual cache key, so an existing ghost cannot retain the wrong color.

Reasons cover overlap, permitted building area/setback, size/attachment/clearance,
stale revision retry, missing objects, busy property and unavailable definitions.
Early invalid rectangle sizes use their existing specific size instruction. Client construction now uses the owning-controller transport above; direct client
placement-service execution still fails authority validation. The feedback
layer does not bypass server authority or introduce player approval prompts.


### Nearby building drafts

The temporary reservation implementation was removed at the user's direction.
Nearby players coordinate; only final server validation prevents conflicting
commits. Previewing never reserves land, advances revision or changes ownership.
