# Assembly Supply Bins and Parts Collectors

Date: 29 October 2026

This design supersedes the mandatory kit-box and mixed warehouse-pallet proposals. Implementation evidence and remaining limitations are recorded separately in the test report.

## Player flow

1. Buy and receive materials or purchased components.
2. Plasma and brake employees make the selected parts.
3. Forklifts store single-item pallets in the business's blue parts storage.
4. Select an assembly recipe and batch quantity at a fabrication workstation.
5. Assign a parts collector to that assembly area through the ERP employee screen.
6. The collector gathers a capacity-limited mixed cartload from available parts storage.
7. They replenish the area's Assembly Supply Bin automatically.
8. Welders consume the recipe's ingredients and build the product.
9. Completed products collect on single-product OUT pallets.
10. Forklifts move those pallets into compatible finished-goods storage for sale, or parts storage when required by a later assembly.

The collector is assigned to the destination area, not individual racks. The player does not create shopping lists or one route per part. One initial assembly area corresponds to one welding workstation; grouping multiple benches under a single collector assignment is a future extension, not a hidden assumption in the first implementation.

## Containers and handling

| Container | Contents | Limits |
| --- | --- | --- |
| Warehouse pallet | One item type | Product weight, packaging and physical fit. |
| Collector cart | Several part types in separate trays | Total payload and finite carrying space. |
| Assembly Supply Bin | One part type per compartment | Per-compartment payload and count/space limit, plus whole-unit payload. |
| Frame-section position | Bulky frame components | Existing forklift delivery and physical stock capacity. |
| OUT pallet | One completed product type | Product-specific stacking and payload. |

No kit table, kit-box inventory, or packing employee is required. "Kit" in older code comments can mean the recipe's ingredients; it does not authorize introducing a new box item.

Small parts are transported on carts. Forklifts retain bulk pallet, frame-section and large-component work. Large tanker assemblies use floor positions or cradles rather than being squeezed into small-parts compartments.

Collectors may gather from any level of a compatible parts rack. Lowering upper-level pallets and reach equipment are deliberately abstracted for this version.

## Assembly Supply Bin

The standard bin has four compartments. Upgrade it to six or eight. The larger variants add physical width and remain the same storage family.

Each occupied compartment has one assigned item, an exact quantity, unit weight, capacity and refill target. Unused compartments remain available. Initial recipe assignment is automatic. Targets cover a small batch, never more than the current remaining job requires.

Initial gameplay ratings are 100 lb per compartment, subject also to the item's count/fit limit. Four, six and eight compartments have 400, 600 and 800 lb total ratings respectively. These are tunable game values, not real-world engineering ratings. Additional rating upgrades can be introduced through the same capacity definitions; do not treat stronger ratings as permission to exceed physical fit.

All player-facing measurements use feet/inches and pounds. Unreal remains at 1 UU = 1 cm. Neither products nor storage are resized to force a fit.

## Integrated station layout and hologram

The Assembly Supply Bin is part of the fabrication station, not an independently placed requirement. Show the actual bin in the combined placement hologram.

Replace the current small-parts IN pallets with the bin. Keep one separate frame-section position and the existing OUT area. The supply bin sits within the IN side of the station and faces its outside access end. Preserve both welder positions and their access.

Upgrades use the actual wider bin geometry. Validate its complete footprint, including feet, against the station and nearby solids. Rotation and swapped IN/OUT sides must transform the bin, its compartments and its cart approach together.

The placement preview and committed layout must use the same transform helper. An invalid whole station remains red, including the bin. Do not leave obsolete small-parts pallet outlines under the new bin.

## Request calculation

Requests originate from the active assembly recipe and batch quantity. For each required small item:

```text
Wanted = minimum(refill target, remaining recipe demand, physical/weight capacity)
Request = maximum(0, Wanted - stock already present - stock already incoming)
```

The active unit's ingredients are already committed and must not be requested a second time. An incoming reservation counts from dispatch through unloading, regardless of whether it is still on the source pallet or already on the cart.

When no active batch needs supplies, do not keep filling bins speculatively. Partial remaining stock stays available at the station. A future recipe change must either reuse compatible compartments or return incompatible stock before reassigning them; never overwrite inventory to change a label.

## Collector behavior

Collectors belong to one business and plant. Assign existing hired employees through ERP Team > Parts collector > Assembly area. The same employee ID and name persist across job changes.

1. Read outstanding requests for the assigned area.
2. Select available compatible storage, preferring a practical nearby source.
3. Reserve source quantities and destination capacity before departure.
4. Combine multiple ingredient types in a trip when cart limits permit.
5. Walk to each source rack and perform a loading interaction.
6. Travel to the bin's cart access position.
7. Perform an unloading interaction and transfer exact quantities.
8. Repeat or return to the waiting area when no delivery is needed.

More than one collector may serve the same area. Reservations prevent duplicate collection. Missing stock leaves a request pending; new stock becomes eligible automatically. Stock committed to customer shipments is unavailable to collectors.

A release request lets the collector finish its current delivery and return before changing jobs. Do not teleport an employee or discard carried parts. Temporary traffic and blocked paths wait or retry without consuming stock twice.

## Animation and presentation

Use the existing employee walk cycle, with both hands positioned on the actual cart handle while pushing. Cart travel and employee motion share the same replicated simulation state.

Wheels rotate by actual travel divided by wheel radius. Casters steer during turns. Wheels stop when travel stops; pause freezes work animation. Preserve real wheel/caster pivots as separate meshes.

During loading/unloading, keep one hand at the cart and animate the other reaching toward a tray. Quantity transfers happen at the handling event, not every rendered frame. The first version abstracts individual shelf picks; it does not claim hand placement on every item at every rack level.

Show representative parts in occupied bin compartments and cart trays. Exact quantity remains simulation data. Do not spawn a separate actor for every bracket or imply ten visible pieces means exactly ten stored pieces.

The bin inspector/report should make available, target and incoming quantities clear, alongside pounds used and rated capacity. Routine successful deliveries need no intrusive confirmation messages.

## Finished output

Headache-rack OUT pallets hold four finished racks, independent of input batch size. A forklift collects a full pallet, or a partial pallet when the batch ends or an eligible order requires it. Production waits when OUT has no room.

Changing the stacking limit requires preserving existing older five-stack inventory until it can be shipped or explicitly repacked; never delete the fifth product during save migration.

Finished-goods rack signs, tablets and direct-rack shipping remain governed by the separate racking plan. Do not claim those larger shipping changes are implemented merely because this collector system exists.

## Authority, persistence and scale

The server owns assignments, recipes, requests, reservations, cart cargo, bin stock and production consumption. Owning-player commands validate business permissions through the existing plant command path. Clients render the resulting snapshots.

Store goods in exactly one location: source pallet, cart, supply bin, committed assembly or finished output. Reservations identify commitments without creating extra inventory. Include carried cargo in material-mass accounting.

Save collector identity, area, path progress, load lines, source reservations, picked state and handling progress. Restore those together before dispatch resumes. Keep private camera, menu and overlay choices local.

Use bounded trips and event/retry-driven route requests. No per-frame town-wide stock scan or per-part replicated actors. Support multiple players and companies; testing with four players does not establish the final player cap or 60 FPS target.

## Required validation

- Four/six/eight compartment model dimensions and placement at every quarter turn and side swap.
- Mixed cartloads under payload and space limits.
- Two collectors requesting the same item without double reservation.
- Missing and order-reserved stock, new stock arrival and safe retries.
- Supply delivery actually enabling an assembly to finish.
- Frame sections still delivered by forklift.
- Material mass preserved during pickup, transit, unload and assembly.
- Save/load before pickup, mid-trip and during handling.
- Employee release without teleport or lost cargo.
- Actual handle hand placement, wheel rotation, caster steering and pause behavior.
- Separate-company authorization and late-join snapshot consistency.
- Existing cut-part sales, welding and shipping regression tests.

Record source review, compilation, automation, multiplayer and visual acceptance separately. Missing checks remain explicit rather than being inferred from a successful build.
