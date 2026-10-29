# First-person physical objects

The shop's physical objects use explicit, game-owned query shapes. A component's
`BlockAll` profile alone does not prove that its imported mesh has simple collision.
The actual 30 cm radius / 90 cm half-height walking character is the acceptance
shape. Collision geometry is not a placement footprint or an employee work zone.

## Geometry and lifecycle

- Machinery keeps its primary mesh bounds and adds the separate operator console,
  power unit and extraction unit. Welding equipment and office furniture have
  bounded fallback shapes. Existing building walls and animated door leaves retain
  their own geometry and opening logic.
- Forklifts use a chassis, mast posts, cab posts and roof, rather than a tall solid
  box covering the entire open cab. Inactive pooled forklifts cannot block players.
  The authoritative server creates roots and bodies even without rendered meshes.
- Stock and carried pallets use the union of the visibly rendered pallet/supports
  and sample parts. Exact quantities remain in inventory. Raised rack loads stay
  raised; removed or off-site pallet IDs lose their collision components.
- Carrier bodies follow the displayed vehicle. Delivered, hidden and off-site
  carriers disable collision; reuse enables it for the next actual booking.
- Birch and broadleaf trees near the starter site use narrow, low stem capsules.
  The one-time scan is capped at 4,096 stems within 200 m of the origin. Grass,
  flowers, bushes and canopies remain passable. Vendor foliage assets are unchanged.

The common `FoundrylinePhysicalCollision` tag identifies generated shapes. Names
identify their object or pallet ID. Bodies block the Pawn channel and do not alter
the factory simulation's material-handling route graph or enable rigid-body physics.
These local pooled components are excluded from relative network movement bases.
The character can still collide with or step onto a low pallet, but sends its world
movement rather than an unsupported component GUID. Actual moving platforms retain
the ordinary Unreal based-movement path.

## Acceptance tools

`review_player_collision.py` exercises the possessed character's real movement
component, then holds the ordinary W input against representative objects. Setup
teleports are separate from measured movement. It also checks walls, the office
corridor, the clear shop aisle and the closed/open personnel doorway. Temporary
review positions never write a player save.

`validate_player_collision_network.py` repeats physical queries on four PIE peers
and compares actual client W movement with the authoritative matching character.
The test bridge matches PlayerState ID across the editor's PIE worlds; it exposes
no packaged gameplay privilege. `run_player_collision_network.ps1` restores editor
Play settings byte for byte after the run.

The optional collision mode of `validate_employee_jobs_network.py` adds a real
40-part production pallet walking/stepping check before shipment, followed by the
existing hiring, company isolation, late join and delivered-payment flow.

## Limits

Kinematic vehicles now block a player's own movement. They do not yet perform a
full swept traffic/yield simulation against a stationary human player. Employee
and forklift simulation routes remain separate from Unreal character collision.
No crowd-capacity, packaged-build or FPS claim follows from these local tests.

## Verified acceptance (28 October 2026)

- The complete collision/tree build passed **114/114** automation cases in
  `Saved/Logs/PlayerCollisionTreeFullFinal.log`.
- The final movement-base and inward office-door fixes passed **6/6** relevant
  collision, world and starter-layout cases in `OfficeCorridorAcceptance.log`.
  This is not a claim that the expanded 116-case suite was rerun in full.
- Four live PIE peers passed **72/72** collision/input checks in
  `PlayerCollisionNetworkQA.json`. Normal W movement stopped at eight object
  categories, with server/client agreement; 856 active generated bounds matched
  on each peer, including 843 nearby stems.
- The real five-player/three-business production, low-pallet step, shipping and
  payment flow passed **67/67** checks in `CollisionCommerceNetworkQA.json`.
  The reproduced unsupported movement-base/ignored-correction warnings are absent
  from the final `CollisionCommerceNetwork.log` after the scoped SetBase fix.
- Final rendered PIE passed **23/23** walkthrough checks in
  `PlayerCollisionNativeQA.json`: machinery/accessories, rack/fork/desk and tree
  blocking, walls/partitions, an unobstructed corridor, normal door interaction,
  closed-leaf blocking, open-leaf clearance and actual doorway traversal.

Internal office room doors now swing into their rooms; external exits retain
their previous swing. The corridor fixture starts clear of the lower wall and
tests the doorway without extending its endpoint into the far corridor wall.
Its imported animated leaf physics is verified in rendered PIE after asset
readiness; the focused automation case verifies the inward leaf bounds and free
centre corridor. Native tree alignment is illustrated in
`Saved/Screenshots/Reviews/TreeTrunk_CollisionAlignment00000.png` with a temporary
cyan debug stem, not a gameplay overlay.
