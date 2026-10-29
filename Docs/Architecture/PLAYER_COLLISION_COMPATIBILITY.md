# Physical walking collision and simulation compatibility

Physical walking blockers are separate from the deterministic factory simulation. The new runtime boxes respond to the Pawn channel and do not become rigid material-handling obstacles, placement preview blockers or automatic NavMesh generators. Existing machine/rack solids, product/welder bounds, painted routes, role ownership and inventory jobs remain authoritative for the factory simulation. No new simulation pathing rules were required by this patch.

## Coordinate and lifetime rules

Factory record positions and navigation solids remain in plant-local centimetres. Physical shapes attach to the corresponding plant/equipment components and inherit the company transform. Forklift cargo bounds follow the visible carriage, yaw and lift; the pallet record's Position is the forklift root while carried and must not be treated as the visible load centre.

Factory floor stock includes Receiving, Rack, MachineInput, MachineOutput, Buffer and Pickup. Forks use the carried visual transform. Inbound and Outbound are carrier-owned loads; InTransit is offsite. These are excluded from factory floor pallet collision to avoid duplicate or stale barriers. Bounds cover the representative visible parts and wooden supports, not an invented full-quantity stack.

Pallet boxes are keyed by pallet ID and removed when a pallet merges, disappears, ships or moves into a carrier/offsite place. Unallocated forklift pool entries disable their blockers. Retired carriers and offsite in-transit bookings disable collision; a reused visible carrier reenables it. Client carrier ticks also follow the replicated hidden state, preventing an invisible vehicle wall from persisting on peers.

Dedicated-server setup creates forklift roots/carriages and physical bounds before skipping cosmetic rendering. This is a source-reviewed code path; the multiplayer acceptance run uses a listen server and does not establish a packaged dedicated-server certification.

## Regression evidence

The combined collision implementation built successfully, then all 113 Foundryline automation tests passed (`Saved/Logs/PlayerCollisionFullAcceptance.log`, exit code 0). After the bounded nearby tree-trunk wrappers were added, the final combined build and all 114 tests passed (`Saved/Logs/PlayerCollisionTreeFullFinal.log`, exit code 0). The unchanged employee jobs, real production, forklift loading, paid delivery, shipping/payment, save/migration, company ownership and lane regressions remain green.

The new carrier lifetime test performs a real first-person capsule sweep at a shifted and rotated plant, verifies active collision, retires the delivered actor and verifies no invisible wall remains, reuses the same one-entry pool with collision enabled again, then verifies an in-transit actor is nonphysical. Physical-object tests cover the other new bounds separately.

PIE-only test helpers can place one controlled character and its matching authoritative peer at the same reviewed starting position, then report server/local positions. Normal character input and network movement validation are used for collision acceptance afterward; the helper does not move business objects or mutate inventory/accounts.

## Replicated low-pallet stepping

The four-world collision/normal character movement run passed 72 checks (`Saved/Logs/PlayerCollisionNetworkQA.json`). A separate real production flow then staged an actual bracket pallet and drove a client character onto it. Although the business-flow checks passed, the network log exposed a real defect: `FNetGUIDCache::SupportsObject` rejected the local `Pallet2025_Physical` movement base and `ClientAdjustPosition` ignored a correction whose relative base could not resolve.

The scoped fix extends the existing generated-floor SetBase policy: components tagged FoundrylinePhysicalCollision retain their real collision and floor/step detection, but do not become replicated character movement bases. Character positions stay authoritative in world coordinates. Untagged real moving platforms retain standard Unreal based movement. This avoids pretending dynamically created local collision components have stable network names.

That final movement-base build passed in 4.78 seconds, followed by all four targeted Collision/World cases (`Saved/Logs/PlayerCollisionMovementBaseAcceptance.log`, exit code 0): carrier lifetime, tagged-local-base exclusion with collision retained and real moving platforms preserved, tree stems, and the normal first-person/personnel-door suite. The full pre-fix suite passed 114 tests; this final isolated fix adds the new movement-base case and was verified with the targeted run rather than claiming a new full 115-test run.

The actual five-player production/pallet-step rerun then completed all 67 checks with no errors (`Saved/Logs/CollisionCommerceNetworkQA.json`). The final `FoundrylineCollisionCommerce.log` no longer contains the unsupported pallet movement-base or ignored ClientAdjustPosition correction warnings seen in the pre-fix run. This confirms the real staged-pallet step case as well as the production/shipping/payment and company/late-join flow; the earlier green business assertions alone had not established that correctness. Native walkthrough evidence is recorded by the UI coordinator after completion. Kinematic factory vehicles and employees do not yet implement a general passenger-platform or human-player yielding system.
