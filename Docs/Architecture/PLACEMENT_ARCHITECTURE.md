# Placement foundation

Status: C++ source written; NOT compiled, UHT-validated or run in Unreal yet.
Do not start builds/editor sessions until the user's other Unreal work is finished.

## Coordinates and definitions

Grid addresses and saved bounds use integer Unreal centimetres, anchored at world
zero. Structural, fine and precision steps are 100, 25, 10 and 5 cm respectively. Physical dimensions remain
independent of snapping. Bounds are half-open boxes, allowing edge adjacency.
Definition assets contain stable content IDs, soft mesh references, mesh offsets,
footprints, working clearances, allowed surfaces, prices and optional function IDs.
Price is metadata only; no spending system is implemented.

Initial placement is ground-level and uses quarter-turn yaw. Rotation is about the
placement anchor, which need not match the imported mesh pivot. MeshOffsetCm must
be rotated with the object when the future renderer spawns it. No scaling is applied.
Bounds currently have 1 cm precision and must conservatively enclose authored meshes.
Free rotation, wall attachment and elevated furniture placement are deferred.

## Authoritative actions

UFoundrylinePlacementSubsystem owns the data in standalone/server worlds. Clients
cannot execute actions. Trusted server/local code registers definitions and creates
company/plant regions, then submits construction requests with a current revision.
The trusted context supplies company, plant and construction permission; never
copy these fields directly from a client request. Networking transport, player
authentication and replicated previews are not implemented.

Build creates a server-side GUID; move, rotate and remove retain that ID. Validation
runs before any mutation. Stale revisions are rejected rather than replayed. Future
RPC integration must add request IDs/session epochs and rate limits; the revision
is not a complete network replay-protection or authorization mechanism.

For now, an object's complete working clearance must fit within one rectangular
owned region with an allowed surface. Regions cannot overlap in XY. Joining adjacent
parcels and painting mixed surfaces require a later spatial-layer implementation.
Collision checks include all placed objects, not just the current company.
Functional blockers and minor obstructions both reserve their actual boxes;
cosmetics reserve no collision and cannot have a simulation function. Clearance may
overlap other clearance, but solids may not intrude into either object's clearance.
Navigation updates and collision components are not yet implemented.

The object list currently uses linear collision scans. That is deliberate for the
small first placement prototype, not the final town-scale spatial index.

## Draft plans and atomic placement

FFoundrylineConstructionPlan owns a local draft with up to 128 ordered operations.
Beginning a draft captures the current revision. Adding/removing draft operations,
previewing or cancelling never changes placement state. Batch validation simulates
operations in order, so removing an existing object can free space for a later build.
Build IDs in a preview result are provisional; do not retain them or reference them
from later requests. Editing a newly queued build means changing the draft, not issuing
a move against that provisional ID. Adjacent parcels and arbitrary final-state
reordering remain deferred; the batch is validated sequentially.

Commit requires a freshly resolved trusted context and a maximum approved cost.
Permissions, ownership, revision, every operation and integer cost overflow are
rechecked. Failure keeps the draft and leaves all live objects/revisions untouched.
Success replaces the object list once, advances revision once and closes the draft.
Build costs sum definition prices; move/rotate/remove are currently free and there
are no refunds. Cost approval is NOT a finance transaction; no balance is checked
or deducted. A future Economy integration must commit funds and placement together.

## Preview and local change events

FFoundrylinePlacementPreview produces snapped anchor/rotation, footprint, clearance,
soft mesh reference, pivot-corrected transform, price and readable validation text.
It does not spawn a mesh or material. A renderer can display invalid geometry using
the reported reason; missing/invalid geometry is distinguished explicitly.
Previews currently validate against live authority, not another uncommitted draft.
Use PreviewBatch for complete draft validation. Multiplayer needs server validation
responses and a replicated catalog adapter; this is not implemented transport.

OnPlacementChanged broadcasts before/after records and local clearance bounds only
after the full batch commits. Cosmetic changes set bAffectsNavigation=false. All
listeners see final state, and recursive mutation during delivery is rejected.
Events do not perform navigation rebuilding or rendering themselves. Consumers of
save restoration must reconstruct the scene from the loaded records, since restoring
bootstrap state does not issue individual construction events.

All placement/draft/catalog methods are game-thread APIs; no concurrent access is
supported. Definitions are authored catalog data and should remain immutable once
registered for play. Retuning definitions requires a deliberate migration/reload.

## Save records

UFoundrylinePlacementSave stores schema version, world revision, regions and placed
object records including stable IDs and manual/auto-furnished provenance. Definition
assets are registered from the content catalog before restoring. Unknown definitions,
duplicate IDs, invalid placement or unsupported schemas reject the entire restore.
Restore is bootstrap-only; it cannot rewind an active world's action revision.
Async file-slot IO is now provided by FoundrylineSaveSubsystem; migrations,
per-object functional state and the rest of the simulation save remain future work.
The bAutoFurnished field is reserved; no furnishing generator
exists yet.

The game-instance save service accepts only 1-32 ASCII letters/digits/underscore/
hyphen for slot names, prefixes storage slots with Foundryline_, uses local user
index 0, and allows only one outstanding save/load per game instance. Save captures
placement immediately, then writes asynchronously; later construction is not included
in that snapshot. Loading requires the same live world, a registered catalog and
empty bootstrap placement. Wrong types, versions, IDs or geometry fail restoration.
Existing slots require explicit bAllowOverwrite=true. No automatic file operations
are connected to startup or input. No save files were created during implementation.
Crash-safe backups, save browser/widgets and filesystem integration tests are pending.

## Integration order

1. Compile FoundrylineEditor and run the Foundryline automation test group.
2. Create a small concrete/grass test region and one placeable Data Asset.
3. Connect camera/cursor world hits to SnapWorld and server validation results.
4. Connect the preview contract to a hologram renderer and the draft plan UI.
5. Subscribe presentation/navigation adapters to local change events; integrate economy.
6. Extend ownership/surface layers, edge walls/doors and the town-scale spatial index.

No foundation code ticks every frame. The tests cover scale/snap/rotation, clearances,
ownership, stale requests, mutations and atomic restoration. They are source-only
at this stage and have not been executed.
