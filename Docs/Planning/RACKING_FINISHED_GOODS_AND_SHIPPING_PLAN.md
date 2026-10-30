# Standard Racking, Finished Goods and Direct Shipping Plan

**Date:** 29 October 2026  
**Status:** Detailed design and implementation plan; not a claim of implemented gameplay.  
**Related plan:** [Welding Workstation Plan](WELDING_WORKSTATION_PLAN.md)

## 1. Purpose

Make factory flow readable without requiring the player to move every pallet, create many specialized rack types, or transfer finished goods through redundant staging zones.

Use the same standard rack family for parts and finished goods. Its assigned purpose determines what it accepts, its sign, and whether it has the shipping tablet. Forklift employees perform the handling automatically.

The intended flow is:

```text
Receiving -> materials/parts storage -> workstation IN
           -> production -> workstation OUT
           -> finished-goods rack -> booked truck -> customer
```

For parts sold directly, the completed part is a saleable finished product and can enter finished-goods storage. For assemblies, components go to parts storage and the completed assembly goes to finished-goods storage. The item's use and production destination determine its handling; its mesh alone does not.

All ordinary shipments use pallets. Large products such as tanks retain separate large-product handling. Do not invent a different storage stand for every small product.

## 2. Agreed decisions and implementation assumptions

### Agreed decisions

- One standard rack family, with one, two or three levels.
- The floor is level one. Two pallet positions per level means two, four or six positions.
- Storage use is Materials & Parts or Finished Goods.
- Blue PARTS signs identify parts racks. Green FINISHED GOODS signs identify finished-goods racks.
- Signs show occupied / total pallet slots.
- Finished-goods racks have a tablet showing contents, order assignments and unassigned stock.
- Pallets contain one item type; exact quantities remain authoritative even when only a sample is rendered.
- Finished products may be stock without a customer order.
- Players select pallets and book collection at finished-goods storage.
- Forklifts collect selected pallets directly from racks for the arriving truck. A separate shipping staging zone is unnecessary.
- The truck still requires a loading position and clear forklift access.
- Placement uses a full-size hologram, loading-side arrows, forklift clearance and tablet access markers.
- Access holograms can overlap to support shared aisles and L-shaped arrangements.
- Actual racks cannot overlap or occupy another rack's required access area.
- Every pallet position must be accessible, including slots near corners.
- Compatible alternative racks receive overflow automatically. Production eventually pauses if output has nowhere to go.
- Storage use changes require an empty rack.
- Multiplayer authority and company ownership apply throughout.

### Starting assumptions to validate

- Begin with a 12 ft loading aisle, then validate it with the actual forklift, pallet load and turning envelope. This is a game-layout starting value, not an engineering certification.
- Mount the tablet on either front upright, facing the loading area, with a flip-side placement control.
- Use deterministic nearest practical compatible storage as the default destination policy, with explicit station links taking priority where present.
- A rack tablet starts scoped to its rack, with an optional clearly labeled view of other finished-goods racks belonging to the same business. Cross-rack booking can be added without changing inventory rules.
- Exact tablet standing-envelope dimensions and interaction distance must be fitted to the first-person character and actual rack model.

These assumptions may be tuned during testing without reopening the agreed storage and shipping flow.

## 3. Rack family and physical dimensions

| Variant | Levels including floor | Pallet positions | Intended behavior |
| --- | --- | --- | --- |
| Low rack | 1 | 2 | Floor positions with the low rack frame. |
| Standard rack | 2 | 4 | Two floor positions and two elevated positions. |
| Tall rack | 3 | 6 | Two positions on each of three levels. |

The existing original model family is the intended visual starting point. Verify its dimensions, pivots, shelf openings and collision before adopting it in gameplay. Existing design references describe approximately 10 ft width, 4 ft 10 1/4 in depth, and 5 ft vertical level spacing; these are not a guarantee that every product fits.

All geometry stays 1:1: 1 Unreal unit = 1 cm. Player-facing dimensions use feet and inches. Snapping changes position and orientation, never physical scale.

Each slot needs usable width, depth, height, supported load and a forklift pickup/drop pose. Product-plus-pallet bounds must fit the usable opening, not merely the rack's exterior bounds. Two adjacent positions must account for their actual separating structure and clearance.

The pickup headache rack is approximately 6 ft 6 in wide. Do not assume it fits one standard pallet bay because the full rack is roughly 10 ft wide. Verify an approved transport orientation and packaged dimensions. If it does not fit, provide a wider bay within the same standard rack family, or clearly defer rack storage for that product. Never shrink the product or silently occupy neighboring space. A wider-bay variant still follows the same storage-use, sign, tablet and logistics rules.

Likewise, a 5 ft x 10 ft steel sheet requires a compatible sheet-storage configuration. A Materials & Parts designation does not make a normal pallet opening large enough for full sheets. Keep machine sheet racks and roller cradles compatible with the equipment plan rather than forcing every material into these pallet slots.

## 4. Choosing the rack's purpose

During placement, choose Storage Use in the rack inspector:

| Use | Physical sign | Accepted inventory | Tablet |
| --- | --- | --- | --- |
| Materials & Parts | Blue PARTS | Compatible raw materials and unfinished components | Ordinary contents inspector; shipping tablet not required. |
| Finished Goods | Green FINISHED GOODS | Compatible completed, palletized saleable items | Mounted finished-goods and shipping tablet. |

Use a readable word label and simple icon as well as color. The choice should not require separate purchases of visually identical racks.

The storage sign updates automatically when use changes. It includes a compact occupied / total count. Temporary status can indicate full or blocked, but ordinary play should remain visually quiet.

A rack can change use only when it has no stored pallets, incoming slot reservations, active transfers or booked pickups tied to it. Explain the specific blocker. Do not discard inventory or redirect a moving forklift invisibly.

Repositioning, removing or changing levels also requires safe handling of stored goods and outstanding tasks. For the first version, require an empty, unreserved rack. Preview changes before committing them.

## 5. Placement hologram and layers of space

Treat three volumes separately:

1. **Solid footprint:** uprights, beams, stored pallet envelope and other occupied structure.
2. **Forklift service space:** approach, turning and pallet handling areas required by the slots.
3. **Tablet interaction space:** the standing and interaction envelope at the selected mounting end.

Show the actual rack at full size as a hologram. Display the loading face with arrows and distinguish the solid footprint from the translucent service area. Include slot positions and level height references when useful.

Use normal grid snapping and fine placement supported by construction. Rotation must clearly update loading direction, approach poses and tablet position. Preview and final placement must use the same transforms and validation dimensions.

### Overlap rules

| Pair | Allowed? | Rule |
| --- | --- | --- |
| Rack solid vs rack solid | No | Includes loaded pallet envelopes. |
| Rack solid vs another rack's required service space | No | Prevent blocked pallet access. |
| Service space vs service space | Yes | Shared aisles and L-shaped layouts are supported. |
| Service space vs suitable forklift lane | Yes | They can share drivable space; do not treat clearance as exclusive floor ownership. |
| Tablet standing marker vs service space | Yes | No additional dedicated aisle required. |
| Tablet standing marker vs wall/rack/machine solid | No | Player must physically reach the tablet. |

Floor markings alone should not invalidate otherwise suitable shared access. Respect the project's separate rules for work zones and lanes; do not accidentally remove work-zone overlap protection while allowing rack service areas to share lanes.

An overlapping hologram is permission to share open space, not permission for two vehicles to occupy that space simultaneously. Runtime traffic reservations handle that separately.

## 6. L-shaped and shared-aisle layouts

Support perpendicular racks around one open working corner. Their translucent access areas may cross. The placement validator must still test the actual approach to each slot, especially positions closest to the inside corner.

For every slot, verify:

- The loaded forklift can approach its loading face.
- There is sufficient room to align, raise/lower forks and insert/remove the pallet.
- The carried load and vehicle swept envelope avoid the adjacent rack.
- The forklift can reverse or leave after the transfer.
- The appropriate mast height can reach that level with adequate building clearance.

Do not accept a rack simply because its overall aisle rectangle intersects a navigable lane. Conversely, do not reject all L shapes just because two translucent clearance rectangles intersect.

For initial implementation, block placement if any intended slot is inaccessible. Avoid creating unexplained unusable slots in a rack advertised as having four or six positions.

Show specific feedback such as: "Inside-corner pallet position is blocked" or "Loading side faces the wall." Highlight the failing area rather than presenting only a red rack.

Later construction must revalidate affected nearby access. A wall or machine cannot permanently block a rack merely because the rack was placed first. Temporary vehicles or people are runtime obstacles, not permanent construction occupancy.

## 7. Tablet location and first-person interaction

Mount the tablet on the left or right front upright, facing the loading area. Keep it within comfortable first-person reach and protect it from the normal pallet insertion envelope.

Choose an accessible end automatically. Let the player flip it left/right while placing or editing the rack. Evaluate the tablet's own solid mount and standing marker against nearby geometry.

No separate aisle is required: the standing marker may share the rack's forklift service area. If both ends are blocked, placement is invalid and identifies the tablet-access problem. Do not spawn a floating tablet elsewhere as a silent workaround.

Approach and press E to open a large, readable software window. Closing it returns to the same first-person position. This interaction does not grant Property View or bypass the office-computer requirement for expansion and construction management.

When a player is using a tablet, normal vehicle obstacle avoidance must keep forklifts from driving through them. Shared space remains shared, so the design must avoid sending a forklift through the stationary user. Use a temporary local obstacle/interaction reservation and retry safely; do not permanently block all rack activity because a menu is open.

## 8. Tablet software and information hierarchy

Use the established light-themed computer software direction: light surfaces, gray structure, orange controls and clear company branding. Green identifies finished goods; blue identifies parts. Avoid constant floating text above every pallet.

The primary screen should immediately answer: What is here? What is promised? What can I ship?

### Header

- Rack name and business name.
- Storage use and occupied / total slots.
- Reachability or blocked status when relevant.
- Clear navigation for Contents, Orders and Pickups.

### Contents list

Each row represents one physical pallet and shows:

| Field | Meaning |
| --- | --- |
| Slot | Level and left/right position. |
| Product | Readable name and thumbnail/icon. |
| Quantity | Exact units and pallet capacity. |
| Weight | Calculated shipment weight, including defined packaging where applicable. |
| Assignment | Order/customer or Stock - Unassigned. |
| Availability | Available, reserved, awaiting collection or in transfer. |

Selecting a row highlights the corresponding pallet in the world and opens its details. Empty positions remain recognizable without dominating the interface. A rendered sample count is never used as the actual inventory quantity.

### Orders view

Show matching accepted orders, product, required quantity, already delivered quantity, outstanding quantity, allocated stock and deadline where applicable. Distinguish allocated from delivered. Order fulfillment advances only through the authoritative delivery rules.

Allow allocating available stock to an order and releasing an allocation if it is not locked into an active shipment. Automatically suggest a matching accepted order where useful, but do not silently commit generic stock to the wrong customer.

### Pickups view

Show scheduled time, carrier, selected pallets, capacity used, fee and pickup status. Booking review must make it clear what is leaving and what it costs. Do not add extra acknowledgment popups for routine successful actions.

## 9. Inventory and order allocation rules

One pallet contains one item type. Multiple pallets can contain the same type. A partial pallet is valid for storage and shipment, even if it contains only one bracket.

Use stable pallet IDs independent of their rack slot. Moving a pallet does not create new inventory or lose its order association. Exact quantities, weight and reservations belong to inventory records, not visual actors.

Completed inventory may remain Stock - Unassigned indefinitely. A player may later allocate it to a compatible accepted order. Stock cannot be promised twice. Track available, allocated and shipment-reserved quantities distinctly.

For the first version, keep each physical pallet's assignment simple: one order or unassigned stock. Splitting a pallet across orders requires an explicit supported inventory operation; do not quietly treat one physical pallet as two independently collectible loads. If partial-quantity selection requires repacking, make that operation explicit or defer it. Shipping an entire partially filled pallet remains supported.

Once reserved for pickup, a pallet's selected contents must remain stable. Production must use another output pallet rather than adding more pieces to a booked load. Cancellation releases valid reservations according to the existing booking rules.

No inventory may be consumed by manufacturing while also reserved for shipment. Separate-company inventory must never be selected as fallback stock.

## 10. Automatic storage and forklift dispatch

The player chooses production, staffing and layout. Employees handle routine transfers.

When a workstation has a collectible completed pallet:

1. Determine its item, handling dimensions, weight and destination use.
2. Find an eligible rack belonging to the same business and plant.
3. Prefer an explicitly configured compatible destination, otherwise choose a practical reachable compatible rack.
4. Reserve one fitting empty slot and one transport task atomically.
5. Assign an eligible available forklift employee using the existing priority system.
6. Pick up, transport and place the same pallet.
7. Commit the slot contents and release the transfer reservation.

Use route suitability and handling compatibility, not straight-line distance alone. A nearby unreachable rack is not a valid destination. Do not generate duplicate jobs for one pallet or reserve the same slot for two incoming pallets.

If the preferred rack is full, use another compatible rack automatically. If none exists, leave the pallet at workstation OUT. Continue only while that station has genuine remaining output capacity; once full, pause before creating goods that cannot be stored.

Explain the blockage in the station and logistics views: "Output full - add finished-goods storage" or "Storage available but route blocked." Do not spawn hidden pallets, delete goods or invent unlimited off-map storage.

Permanent route failure cancels/replans the task safely. Temporary congestion waits or reroutes. If a destination becomes invalid while a forklift carries a pallet, preserve that pallet and find a safe compatible fallback; never erase it to reset the employee.

## 11. Direct shipping from storage

Racks hold inventory. Bookings identify the inventory to ship. The loading position provides truck access. These have separate responsibilities.

The new flow replaces the requirement to move selected goods into a separate shipping staging zone or assign an order to that zone.

Booking steps:

1. Select available pallets from finished-goods storage.
2. Review their order allocation and exact quantities.
3. Choose a carrier/vehicle and an available pickup time.
4. Check pallet count, weight, dimensions and vehicle compatibility.
5. Review the fee and confirm.
6. Reserve the goods and charge according to the existing booking policy.

The vehicle's capacity limits selection. A visually small pallet cannot bypass its weight limit. The starter small freight truck may serve suitable small loads; actual bed, payload and loading compatibility still require asset validation.

At arrival, the truck waits at the assigned loading position. Forklift tasks retrieve reserved pallets directly from their rack slots and load the vehicle. Loaded pallets transition out of rack inventory exactly once. Their order quantities remain in transit until actual delivery confirms fulfillment and triggers the appropriate payment term.

Payment terms remain those of the order: upfront, per delivered load, or completion. Booking and departure do not invent a second payment event. Pickup fees and rebooking behavior remain consistent with the existing commerce plan.

## 12. Receiving and the shared loading position

The starter business has one shared receiving/shipping position. Incoming supplies are unloaded and taken to their designated compatible raw-material or parts storage. Finished goods travel the opposite direction when collected.

The ERP calendar/current shipping page continues showing delivery and pickup times. Overlapping bookings are allowed to create backups rather than being hidden by artificial scheduling protection.

Use vehicle queue and bay occupancy state so two trucks do not occupy the same spot. Forklift priorities should keep transfers ordered without removing player responsibility for a congested schedule.

Additional loading positions are a later property upgrade. Direct rack shipping must not require a dedicated outgoing-only dock in the starter shop.

## 13. Visual clarity in normal play and overlays

Normal play shows physical racks, pallets, visible product samples, PARTS/FINISHED GOODS signs and the mounted tablet. Avoid persistent wireframe boxes and floating quantity labels everywhere.

Selecting a rack may show slot markers, reserved pickups and its loading side. The logistics overlay can show forklift service space, blocked approaches, linked production sources and truck destinations.

Use the established flow colors consistently:

- Blue: parts/supply and IN handling.
- Green: completed output and finished-goods storage.
- Orange: selection, actions and workstation boundary identity where already established.

Text and icons must reinforce the colors. A blocked rack should say why; color alone is insufficient. The overlay must align with actual floor and placement coordinates rather than appearing floating or offset.

## 14. Multiplayer and business ownership

The server owns rack placement, storage use, slot contents, order allocations, reservations, bookings and forklift tasks. Clients display previews and submit intent; a green client hologram is not final authority.

Revalidate placement on commit against the latest solids, service spaces and ownership. Simultaneous placements cannot bypass checks using stale previews.

Every rack, pallet, order, employee, vehicle and booking needs company/plant scope. Co-op members with permission share their company's racks. Players running independent businesses cannot allocate, ship or consume another company's goods through proximity or a tablet request.

Two players selecting the same pallet must produce one successful authoritative reservation. The other screen refreshes availability with a concise reason. Avoid duplicate fees and double pickup tasks through request idempotency and transactional commits.

Late joiners must receive current rack use, pallet locations, reservations and shipment status. Tablet mount choice and rack levels are shared; a player's open screen and selected row are local presentation.

## 15. Data and system boundaries

Suggested records, to reconcile with existing types rather than duplicate them:

| Record | Required information |
| --- | --- |
| Rack definition | Model/variant, usable slots, dimensions, level heights, load constraints, service envelopes and tablet mounts. |
| Placed rack | Stable ID, company/plant, transform, definition, use, tablet side and state revision. |
| Rack slot | Stable rack/slot address, occupant pallet, inbound reservation and access status. |
| Pallet | Stable ID, item, exact quantity, packaging/weight, location and allocations. |
| Transfer task | Pallet, source, reserved destination, employee/vehicle, lifecycle and retry state. |
| Shipment | Selected pallet IDs, order associations, carrier, schedule, loading position, fee and delivery state. |

Construction owns placement; Inventory owns contents and reservations; Orders owns fulfillment; Logistics owns transfers and freight; Companies owns permissions; UI consumes their state. SaveSystem composes versioned records and does not become the owner of live gameplay logic.

Keep continuous forklift simulation separate from mostly static rack records. Update tablet lists through relevant state changes; do not scan the entire town every frame or issue global navigation rebuilds for cosmetic sign changes.

## 16. Save/load and transition from current shipping

Persist rack purpose, levels, transform, tablet side, slot contents, pallet identities, assignments and shipment reservations. Restore relationships before resuming transport. Validate missing references without duplicating inventory or charging again.

In-progress forklift and freight tasks need a defined resume/recovery path. Loading a save must not leave a pallet simultaneously on a rack and truck.

Before replacing existing shipping-zone staging, inspect the current implementation and saves. Preserve existing booked shipments and supported fulfillment flows. Introduce the new direct-rack source behind a compatible shipment-source contract where practical.

Do not silently discard old staged pallets or rewrite user saves during documentation work. Design an explicit migration or retain a legacy source until old pickups complete. Document the chosen migration and test it before removing the old requirement.

## 17. Implementation sequence

1. Audit existing rack definitions, pallet records, station output, shipping sources and saved reservations.
2. Validate the physical rack openings and transport packaging for the initial products.
3. Implement shared rack definitions, levels, use and signs without duplicating inventory.
4. Add full holograms, loading direction, overlapping service areas and per-slot access checks.
5. Add automatic output-to-storage transfers, reservations and full-storage backpressure.
6. Add the mounted tablet, accessible-side selection and contents/order screens.
7. Add direct-rack shipment selection and loading while preserving existing order/payment rules.
8. Add save migration/recovery, concurrent-player validation and late-join replication.
9. Perform first-person visual checks and an end-to-end production-to-delivery playtest.

Coordinate runtime integration with the welding-workstation work. This plan does not authorize interrupting another active Unreal session, modifying its worktree, or publishing purchased assets.

## 18. Acceptance scenarios

### Placement and access

- Place all three heights and verify two/four/six usable positions.
- Rotate racks; arrows, slots, tablet and final placement match the hologram.
- Place perpendicular racks in a valid L with overlapping service areas.
- Reject a tight inside corner that blocks one pallet slot, with specific feedback.
- Reject solids intersecting another rack or its required approach.
- Permit shared forklift aisle space without reserving it exclusively for one rack.
- Reject inadequate ceiling/mast clearance for upper slots.
- Verify loaded forklift approach, insertion, withdrawal and departure for every slot.

### Tablet and readability

- Automatically choose the accessible front end; flip to the other valid end.
- Reject both ends blocked; no floating replacement tablet.
- Interact in first person, read contents and identify the selected physical pallet.
- Verify names, quantities, order/stock labels and occupied counts agree.
- Test player presence while a forklift approaches; no collision through the user.

### Inventory and production

- Store unassigned stock, then allocate it to an accepted matching order.
- Prevent duplicate allocation and shipment reservation.
- Store a partial pallet and ship it with its exact count.
- Reject a product too large/heavy for the slot without changing its scale.
- Use alternative storage when the preferred rack fills.
- Pause safely when rack space and workstation OUT capacity are exhausted.
- Recover from route blockage without lost goods or duplicate tasks.

### Freight and business flow

- Produce a simple cut-part order, rack it, select pallets, book, load directly and deliver.
- Repeat for an assembly when its workstation integration is ready.
- Enforce pallet count, payload and handling compatibility during booking.
- Queue overlapping receiving/shipping visits at one loading position.
- Verify allocated, loaded, in-transit and delivered quantities at each step.
- Verify fees and all supported payment terms occur exactly once.

### Multiplayer and persistence

- Have two co-op players reserve the same pallet concurrently: one reservation only.
- Attempt conflicting rack placements concurrently: authority rejects the conflict.
- Verify separate-business permission isolation and company-scoped forklift fallback.
- Test more than two players and a late join during an active pickup.
- Save/load with goods stored, reserved, carried and in transit.
- Resume an existing staging-zone shipment through the chosen compatibility path.

Source review, compilation, automated runtime tests, multiplayer tests and in-engine visual acceptance must be reported separately. No performance target or working gameplay should be claimed from this document alone.

## 19. Deferred features

- Detailed engineering load ratings, rack damage and structural simulation.
- Automated pallet repacking and mixed-order pallet contents.
- Complex warehouse optimization and bespoke storage per product.
- Storage research and capacity upgrades beyond supported physical variants.
- Large-tank shipping and crane handling within this pallet-rack workflow.

The first version is complete when players can understand the blue-to-green flow, place accessible standard racks, store completed pallets automatically, and ship them directly through a reliable business-owned multiplayer system.
