# Welding Workstation Plan

**Date:** 29 October 2026

**Status:** Design and implementation plan. This document does not claim the workstation gameplay below is already implemented.

**Initial product:** Steel pickup headache rack.

**Goal:** Replace small-product work-zone setup with one clear, placeable assembly workstation that one or two assigned welders can operate.

## 1. Agreed direction

- Place one complete welding station instead of drawing separate product, supply and output zones.
- One painted outer border contains the table, parts handling positions and both worker spaces.
- Blue IN position for supplies; green OUT position for finished assemblies.
- One welder is sufficient. A second welder helps complete the same product faster.
- Incoming parts and finished-product collection are automatic once the station has a recipe, quantity, staffing and connected logistics.
- Product geometry stays at 1:1 scale. All player-facing measurements use feet and inches; Unreal geometry remains centimetres.
- The table is an independent asset. Product, removable fixtures, floor markings and station gameplay records remain separate.
- Four shared assembly part categories only: Steel Plates, Steel Brackets, Frame Sections and Light Kits.
- Employees belong to their business. Co-op players share that business's stations and staff; separate companies cannot control or consume one another's resources.

## 2. Scope

### First playable implementation

One standard welding-station type, one headache-rack assembly recipe, one or two welders, automatic supply/output transport, painted placement preview, a compact station interface, save/load and multiplayer validation.

Keep the existing cut-part sales loop working alongside assembly sales. Do not require a new player to complete the tanker workflow or buy every machine to use this station.

### Deferred

- Truck bumpers and equipment platforms as additional finished recipes.
- Larger assembly fixtures and tanker floor bays using the same general station concepts.
- Payroll, skills, research, custom jigs, maintenance and consumable inventories.
- General automatic factory expansion, arbitrary material nesting and structural simulation.
- Detailed crane/lift physics and full manual handling simulation.

These later systems must not block a simple headache-rack order from reaching delivery and payment.

## 3. Physical station layout

### Default dimensions

Revised 29 October 2026 (Phase 1): Ben asked for a welding machine on each side of the table. The existing Welder_Machine (4 ft 0.3 in x 2 ft 3.8 in) does not fit the spare corners of the original 16 x 12 ft layout without shrinking a worker clearance or a port, so the reserved station grew by 1 ft in each direction to 17 x 13 ft.

Revised 29 October 2026 (OUT pallets, Ben's option B): OUT holds two floor pallets side by side, each with its own stack of finished racks lying flat. A 6 ft 6 in rack cannot lie along a 4 ft bay, so OUT widened to 6 ft 6 in and the station to 19 ft 6 in x 13 ft. Nothing else changed size: the bench, both welder strips, both machine bays, IN and the parts bay keep their 17 ft plan (the "core"), and the extra 2 ft 6 in is added at the OUT end.

| Element | Size | Treatment |
| --- | --- | --- |
| Complete reserved station | 19 ft 6 in x 13 ft | Foundryline-orange perimeter. |
| Welding bench | 8 ft x 4 ft, 36 in high | Glossy painted frame and polished bare-steel worktop. |
| IN position | 4 ft x 8 ft | Blue outline, light diagonal hatch, white IN label. |
| OUT position | 6 ft 6 in x 8 ft | Green outline, light diagonal hatch, white OUT label. Two wood pallets on the 2 ft fork line, each taking a stack of 5 finished racks (10 at OUT). |
| Worker A clearance | 8 ft x 4.5 ft along one long table side | Unpainted clear floor. |
| Worker B clearance | 8 ft x 4.5 ft along opposite long side | Unpainted clear floor. |
| Welder A / Welder B machine bays | 4.5 ft x 2.5 ft each, diagonal corners | One existing MIG welding machine per welder, with its own gun, lead and holster. |
| Outside access ring | 4 ft on every side | Forklift docking at both port ends; people walk the whole ring to reach both welder sides. |
| Painted line width | 3 in | Real floor-aligned paint, not floating lines. |

Both worker areas are reserved when placing the station, even when only one employee is assigned. Adding a second employee must not require enlarging the layout.

### Exact placement within the 19 ft 6 in x 13 ft footprint

Local station coordinates use feet below. The origin is the lower-left corner of the reserved footprint; these are planning coordinates, not a change to Unreal's centimetre storage. `FFoundrylineWeldingStationLayout` is the single source for these values.

Default side (IN on the left). With sides swapped, OUT is 0–6.5 ft, IN 15.5–19.5 ft, and every core element moves 2.5 ft right.

| Element | Local X range | Local Y range |
| --- | --- | --- |
| Bench | 4.5–12.5 ft | 4.5–8.5 ft |
| Worker A area | 4.5–12.5 ft | 0–4.5 ft |
| Worker B area | 4.5–12.5 ft | 8.5–13 ft |
| IN area (default side) | 0–4 ft | 2.5–10.5 ft |
| OUT area (default side) | 13–19.5 ft | 2.5–10.5 ft |
| OUT pallet 1 / pallet 2 (centres) | 17.5 ft | 4.5 ft / 8.5 ft |
| Welder A machine | 0–4.5 ft | 0–2.5 ft |
| Welder B machine | 12.5–17 ft | 10.5–13 ft |

```text
                                19 ft 6 in overall
             +--------------------------------------------------+
             |  (IN)    |     WORKER B CLEARANCE    |MACH B|     |
             |  BLUE IN    +------------------+  GREEN OUT [P2] |
             |  4 x 8 ft   |  WELDING BENCH   |  6'6" x 8'      |
             |             |     8 x 4 ft     |            [P1] |
             |             +------------------+                 |
             |MACH A|      WORKER A CLEARANCE    |  (OUT)       |
             +--------------------------------------------------+
                                13 ft overall

          Forklift approaches are outside the left/right ends.
          Worker approaches are outside the upper/lower sides.
```

The diagram is schematic; the coordinate table controls actual geometry. Orange perimeter and internal paint must not shrink the required clearance. Swapping sides exchanges the IN and OUT ends; the bench, clearances and machines keep their place beside IN, so they move 2 ft 6 in with the swap.

### Placement options

- Rotate the whole station in 90-degree steps.
- Swap IN and OUT sides as a single station option.
- All work points, dock poses, fixtures, paint and local inventories follow the station transform.
- Require adequate shop height and physical product clearance above the bench.
- Keep forklift approach space outside the station, connected to an allowed lane.
- Keep both worker approach sides connected to personnel access.
- Do not place across existing lanes, walls, doors, other stations, occupied zones or equipment.
- The default is a fixed usable layout, not a new collection of freehand sub-zones.

## 4. Independent assets and appearance

### Standard bench

The bench remains its own placeable/model asset with a floor-centred datum and real 8 ft x 4 ft x 36 in dimensions.

- Glossy Foundryline-orange painted frame.
- Dark painted braces/shelf and feet.
- Polished bare-steel top; do not paint the active welding surface.
- Existing perforations and useful fixture mounting details.
- Model-aligned player collision that leaves access space clear.

### Fixture kit

Risers, stops and clamps are a separate removable fixture kit. They are station-owned presentation for the selected recipe, not extra inventory products the player must purchase individually in this first version.

### Product

The current pickup rack is raw carbon steel with mill-scale/weld detail, cab-shaped corners, bed-rail saddles, stake-pocket inserts and two lights. Draft size is 6 ft 6 in wide and 28 in above the bed rail, with 4 in inserts below the rail.

It lies flat in the bench fixture at unchanged scale. The fixture must support actual frame members and leave projecting mounts clear of the worktop. Do not scale the rack to fit the table or intersect its mounting hardware with the surface.

The OUT position holds finished racks on two wood pallets (the game's SM_Pallet_Wood, 3 ft 9.6 in x 3 ft 10.4 in). Each rack lies flat on its back, its 6 ft 6 in length along the 6 ft 6 in bay, and stacks on the rack below. A stack of 5 stands 5 ft 2 in. Each pallet is centred on the same 2 ft fork line as IN, so the forklift stops the same 2 ft 6 in outside either port. The forklift spur keeps 65 cm clear of the station, so the pallet cannot sit deeper in the bay. The stack is centred on the bay: it is flush with the pallet's outer edge and runs on 2 ft 7 in past the pallet's inner edge. Its centre of mass stays 8 in inside the pallet.

## 5. Four standard assembly parts

| Category | Initial source | Meaning |
| --- | --- | --- |
| Steel Plates | Existing plasma production | Standard plate units consumed by assembly recipes. |
| Steel Brackets | Existing plasma production initially | Standard bracket units; avoid adding a brake prerequisite to the first rack unless a later recipe explicitly requires it. |
| Frame Sections | Purchased ready-cut initially | Standard sections for uprights/crossmembers; a saw/tube-stock chain can come later. |
| Light Kits | Purchased complete | One complete lamp kit per inventory unit, with associated wiring/hardware abstracted. |

Fasteners, welding wire and gas are not extra inventory categories in this first pass. Their cost may be included in established operating costs. Payroll remains disabled.

### Starting headache-rack bill

| Part | Quantity per rack |
| --- | ---: |
| Steel Plates | 2 |
| Steel Brackets | 4 |
| Frame Sections | 6 |
| Light Kits | 2 |

These are the previously discussed initial balancing quantities, not a count of every mesh object or an engineering bill. The four-category abstraction is intentional. Visual slats, gussets, saddles, bolts and welds must not silently create additional inventory SKUs.

Select explicit canonical inventory mappings before implementation: the initial proposal is MountingPlate for Steel Plates and the existing Bracket blank for Steel Brackets, with new purchased FrameSection and LightKit IDs. Do not treat all existing plate types as interchangeable or rewrite their current customer recipes. Record fixed weight, price and container capacity for the two new purchased items before enabling supplier bookings.

Other assembly bills belong in product data. Do not hard-code the headache-rack quantities into the station class.

## 6. Supply handling without extra micromanagement

### The single-IN-position problem to avoid

The current pallet rule is one part type per pallet. A station needs four categories. Leaving the first delivered pallet parked permanently in the only IN slot would prevent the other three categories arriving and deadlock assembly.

Therefore, the IN marking is a **transfer port**, not the complete internal inventory.

### Proposed supply sequence

1. A station calculates deficits for the next assembly kit and a bounded replenishment buffer.
2. The server reserves available matching stock from an eligible same-business rack or supplier flow.
3. One forklift at a time claims the IN dock.
4. It delivers a single-type pallet/bundle.
5. A short unload interaction transfers the delivered quantity into the station's internal bins/frame cradle.
6. The dock clears for the next required category.
7. The station can request another supply without leaving the first pallet blocking the port.

Internal stock is exact data and has representative visible plates/brackets/lights and frame-section bundles. It must fit the designed station storage; do not display four full floor pallets or let invisible inventory grow without limit.

Initial buffer target: one complete unreserved kit in addition to the currently reserved/in-progress unit, subject to actual configured capacity. Cap the buffer by quantities and physical container limits. If the full next kit cannot fit, show the capacity issue and prevent a circular transport dependency.

The first implementation may abstract empty-pallet return as part of unloading; do not introduce an empty-pallet commodity or a separate logistics chore.

### Source selection

- Optional preferred supply rack for players who want control.
- Otherwise choose an eligible available same-business source automatically.
- No fetching from another company's rack.
- No automatic purchase of missing parts without a visible authorized supplier order.
- Missing purchased frames/lights should produce a clear Buy parts action.
- Respect existing driver priorities, route reachability, stock reservations and payload limits.

## 7. Work assignment and one/two-welder speed

### Staffing

- Two named employee slots: Welder A and Welder B.
- Each slot maps to one distinct standing/work position on opposite long sides.
- Assign through the existing employee job flow and station interface.
- Existing company/plant permissions apply.
- An employee can occupy one job only.
- The same employee cannot fill both station slots.
- Assignment is exclusive per slot; simultaneous competing claims have one winner.
- An assigned worker must physically reach their work point to contribute.
- No forklift driver or nearby unassigned person counts simply because they are standing close.

Do not globally remove the existing one-welder-per-product-zone guard. This new bench station deliberately has two slots; larger legacy zones can keep their current contract until separately migrated.

### Initial speed curve

| Productive welders physically present | Total work rate |
| --- | ---: |
| 0 | 0% |
| 1 | 100% |
| 2 | 170% |

The second worker improves throughput without perfect doubling. Treat 170% as an initial configurable balance value, not a fixed engine rule. For a unit requiring `W` work-seconds at normal simulation speed, one worker needs `W` and two need approximately `W / 1.7`, excluding delivery, positioning and output waits.

- Store one shared progress value for one physical assembly.
- Do not run two independent copies of the recipe or consume two bills.
- Count worker presence on the authoritative server.
- Apply the existing shared simulation-speed setting after the staffing work rate.
- If a worker leaves, work drops to the remaining valid rate and preserves completed progress.
- If both leave or the plant is paused, the assembly stays at its exact current state.
- A worker replacement must reach the vacant slot before contributing.

### Tools and animation

- Each productive worker gets a separate gun/lead/holster position.
- Tool hardware can be represented by compact paired welding equipment within the station's reserved envelope; do not place carts in clear aisles.
- Workers use different current weld points so their arms, guns and effects do not overlap.
- One worker follows the full weld-point sequence; two divide eligible work on the same product.
- Sparks/light only appear at active owned weld points while real progress advances.
- The Handyman appearance adapter, grounded walking and tool-grip IK remain in use.
- A worker paused or absent must not keep showing an operating animation.
- Actual material installation stages can include quiet fit-up/positioning periods; both workers do not need to emit sparks continuously.

## 8. Recipe, progress and state machine

### Player settings

- Select product recipe.
- Set batch quantity.
- Assign one or two employees.
- Optional preferred supply/output storage.
- Start/Pause/Resume through validated controls.

Changing products should finish the current unit before switching, or leave an explicitly paused WIP unit. Do not silently delete partial assemblies or refund installed parts.

### Station states

1. Unconfigured: choose recipe and quantity.
2. Waiting for staff: no assigned productive worker at the bench.
3. Waiting for supplies: show exact missing/incoming quantities and the appropriate action.
4. Ready: reserve one complete kit and an available output capacity claim.
5. Fit-up: display/position the first physical parts in the fixture.
6. Welding: advance shared progress at the valid staffing rate.
7. Final assembly: install lights/remaining visual details without inventing extra inventory categories.
8. Completed on bench: one finished product exists; wait safely if the output handoff cannot finish.
9. Output ready: product is transferred to its actual output pallet/cradle.
10. Awaiting collection: dispatcher sends an appropriate forklift to storage or shipping.
11. Batch complete: stop requesting new kits; retain useful leftover supplies.

Pause, blocked access and missing output capacity are overlays on these states, not triggers to reset progress.

### Material accounting

- Reserve the complete kit before beginning a unit.
- Convert reserved parts into WIP/installed quantities at explicit milestones.
- Keep remaining reserved stock and installed product stock distinct.
- No second station can reserve the same source quantities.
- If a supplier is late, the station waits; it does not spawn replacement parts.
- Staff changes do not consume/refund materials.
- Final product creation, kit consumption and output ownership transition occur exactly once.
- Preserve the existing cut-part sales inventory and customer orders.

## 9. Finished output

- OUT has two pallets of `RacksPerOutPallet` = 5 racks each (tunable in `FFoundrylineWeldingStationLayout`), 10 in all. A finished-rack pallet record holds up to 5 (`HeadacheRack` pallet capacity 5).
- Each finished rack goes on the first pallet with room; the second pallet starts only when the first is full. Racks on one pallet are one stock record with the average mass per rack.
- Racks lie flat on their backs, stacked on each other; they are not leaned or nested.
- OUT's 6 ft 6 in x 8 ft footprint fits the headache rack's length, not proof that any future assembly fits.
- If OUT is full, do not begin a new unit that has nowhere to go. Existing WIP remains valid and both workers wait when appropriate.
- Reserve output capacity when beginning a unit; revalidate it at handoff.
- Show a short controlled transfer from bench to cradle. Detailed lifting physics can be deferred, but avoid an unexplained teleport across the building.
- Once OUT is occupied, the forklift dispatcher requests collection automatically.
- Preferred finished-goods storage is optional; otherwise choose a suitable same-business destination.
- If linked directly to an accepted shipping order, use the existing exact-pallet/manifest booking rules.
- Clear bench/OUT representations when their authoritative object leaves; no duplicated finished rack.

The standard station must validate product fit. A larger equipment platform requires a larger assembly fixture rather than scaling the product down or overflowing the worker/forklift lanes.

## 10. Paint, selection and placement preview

- Orange outer boundary identifies the whole station.
- Blue IN and green OUT use matching outlines, light hatching and white text.
- Worker spaces remain unpainted so the station does not become a multicolour floor block.
- Paint remains visible in normal first-person play.
- Detailed inventory/clearance graphics appear only on selection or in the relevant overlay.
- Preview shows the bench, fixture, sample product, both work positions, IN/OUT and outside approach checks.
- Blue valid / red invalid follows existing placement feedback.
- Use existing structural/fine snapping without resizing models.
- Rotate/swap-side operations update every anchor consistently.
- Do not rebuild station art or paint on every worker movement tick.

## 11. Player-facing interface

### Station panel

Show the minimum useful information first:

```text
WELDING STATION 01
Headache Rack — 2 of 10 complete

Welder A: Jordan Miller — working
Welder B: Casey Sullivan — walking to station
Current rate: 100%   |   With both present: 170%

Steel Plates     2 / 2 ready
Steel Brackets   4 / 4 ready
Frame Sections   6 / 6 incoming
Light Kits       0 / 2 missing

Status: Waiting for light kits
Next action: Open Purchasing
```

Names above are illustrative; actual names come from the persistent roster. Ready/incoming/reserved counts must be distinct, and rates must reflect actual presence rather than mere assignment.

Controls: Select product, Batch quantity, Assign A/B, Release, Start/Pause, Open supply record and View output. No unsupported decorative buttons.

### ERP integration

- Production lists the station, batch, staff, progress and blocker.
- Team assigns an employee to Station 01 — Side A/B.
- Inventory lists station buffer, reserved kit, WIP and finished output separately.
- Purchasing can buy missing frame sections/light kits using the normal paid supply flow.
- Sales can offer a headache-rack contract only when the business can actually fulfil it.
- Dispatch continues to show real booked pallets, quantities, capacity, fees and deliveries.
- Overview recommends the next real action instead of asking for more parts when supplies are already inbound.

Retain the existing light ERP theme, grey/orange hierarchy, company marks, first-person access and visible speed controls.

## 12. Multiplayer and business isolation

The server owns station IDs, company/plant IDs, placement, recipes, quantities, reservations, employee slot claims, progress and output.

- Controllers send intent, not trusted company IDs or arbitrary mesh/price/time data.
- Validate connected company membership and exact target station/plant.
- Co-op members can manage the shared station with the normal permission rules.
- Independent businesses keep separate materials, employees, station work and output.
- Competing staff/stock/output claims are atomic.
- Stale edits return a useful retry/block reason without partial state.
- Late join restores the actual recipe, WIP, two worker identities, active weld points and progress.
- Financial/order details remain private to the company even when nearby workers/products are visible.
- Clients derive animation from authoritative state and bounded clock interpolation; they do not finish products locally.
- Keep bounded updates and reuse presentation resources; do not add one replicated actor for every bolt or weld bead.

## 13. Persistence and lifecycle

Add station records to the next explicit compatible save schema during implementation. Do not claim this plan already changes the currently deployed schema.

Persist:

- Station ID, company/plant, transform, side-swap and footprint configuration.
- Recipe, remaining quantity and current unit identity.
- Welder A/B assignments by stable employee ID.
- Exact shared progress/stage and installed part counts.
- Local buffer contents, reserved supplies and incoming transport reservations.
- Output cradle identity, finished product, collection job and booking links.
- Preferred storage links.

Loading reconstructs visuals and collision from validated data. Do not save generated mesh-component handles as network identities.

Move/delete restrictions for the first implementation:

- Station may move/rotate only while empty, paused, unstaffed and free of transport claims.
- A station with WIP, supplies, booked stock or active collection cannot be casually deleted.
- Return or relocate real inventory through a validated operation before demolition.
- Disconnecting a co-op player must not delete the company's station or reset its progress.
- Keep normal player saves separate from automation fixtures.

## 14. Implementation boundaries

| Domain | Responsibility |
| --- | --- |
| Construction | Station placement, footprint/clearance validation, rotation, paint and demolition rules. |
| Production | Assembly recipes, stages, worker-rate curve, shared WIP and output completion. |
| Inventory | Part IDs, buffers, reservations, installed material and output ownership. |
| Employees | Two distinct station jobs, walking/access, presence and safe reassignment. |
| Logistics / Pathing | IN/OUT dock claims, delivery/collection, valid lanes and no port deadlock. |
| UI | Station panel and ERP links; display state without owning it. |
| Networking | Validated commands, ownership, conflict/replay rules and relevant replication. |
| SaveSystem | Versioned composition/migration, not a second copy of production rules. |

Reuse existing hiring, worker appearance, pallet transport, company authority and recipe definitions. Do not build a separate fake bench simulation alongside the existing authoritative factory model.

## 15. Implementation sequence

### Phase 1 — Station definition and placement

Build one 16 ft x 12 ft station definition using the independent bench/fixture assets. Add paint, two work points, IN/OUT ports, rotation and side swapping. Validate real product fit and player/employee/forklift clearance.

### Phase 2 — Recipe and supply buffers

Add the four-category mapping, purchased frame/light definitions and one rack recipe. Connect automatic deficits, single-type deliveries, internal unload, bounded buffers and one output cradle. Prove all four ingredients arrive through the single IN port without deadlock.

Implemented 29 October 2026 (schema 11):

- **Part mapping:**
  - Steel Plates = `MountingPlate`.
  - Steel Brackets = the `Bracket` blank.
  - Frame Sections = new bought `FrameSection`: 16 lb and $24.00 each, 12 per bundle.
  - Light Kits = new bought `LightKit`: 4.5 lb and $38.00 each, 8 per crate.
  - `HeadacheRack` = the recipe output: 1 per OUT cradle, not yet produced or sold.
- **Recipe:** `FFoundrylineFactoryCatalog::Recipe(1)` holds the 2 / 4 / 6 / 2 bill. The station stores only a recipe id and the batch quantity.
- **Internal bins** (revised 29 October 2026 at Ben's request, "Parts come from the input pallets. Not the floor"): there is no parts-bay corner. The station's supply stays on two pallets in the blue IN bay, on the same 2 ft fork line as OUT.
  - Bought parts (frame sections, light kits) share one pallet; cut parts (plates, brackets) share the other.
  - A delivery is set down on its pallet, on top of any stock there, and its 8 s unload moves the parts onto that pallet. The delivery pallet then despawns; its return is abstracted.
  - The welders take each kit straight from these pallets, so the stock visibly shrinks. A stock pallet is gone once it is empty, so the next delivery lands on a clear spot.
  - Each IN pallet is delivered to from the stop straight in front of it, and placement checks that both stops are reachable.
  - The hologram outlines both IN pallets in the blue bay, like OUT's.
  - Bin capacity is 4 / 8 / 12 / 4, which is two kits.
  - The buffer target is two kits, or the remaining batch if smaller, capped by the bins.
- **IN port:** IN is a transfer port. Only one delivery may claim it at a time: a pallet on IN, or a forklift job heading there.
  - The planner picks the category with the largest relative shortfall and sends one single-type pallet.
  - An 8 s unload empties the pallet into its bin; empty-pallet return is abstracted. Then the port clears for the next category.
- **Where parts come from:**
  - Plates and brackets come from the business's own racks. Accepted customer orders keep first claim on that rack stock.
  - Frame sections and light kits are bought from the station panel through the normal paid supplier booking. Each purchase is limited to what the bins still lack (counting stock already booked), and the truck's pallet goes straight to that station's IN.
- **OUT cradle:** a steel cradle in the OUT port takes at most one finished product. (Replaced by the two OUT pallets; see Finished output.)

Test: `Foundryline.Factory.WeldingStation.Supplies` covers:

- All four part types arrive through IN, never more than one pallet or one claim at a time.
- Exact rack and bin quantities, paid parts and freight, and material mass conserved.
- A mid-delivery save restores and completes the same supplies.
- Order priority on rack stock, stopping the batch, invalid records, and migration from schema 10.


### OUT pallets (19 ft 6 in station)

Implemented 29 October 2026 (schema 14), on `welding/out-pallets`:

- **Layout:** the station is 19 ft 6 in x 13 ft. OUT is 6 ft 6 in x 8 ft and IN stays 4 ft x 8 ft. The core's spacing to IN is unchanged; the core shifts 2 ft 6 in when OUT is on the left.
- **Pallets:** OUT has two pallets on the 2 ft fork line, centred 4.5 ft and 8.5 ft across the station, always drawn: when a loaded pallet is taken, an empty one takes its place.
- **Stacking:** a finished rack lands on the first pallet with room, one stack per pallet, up to 5 racks, flat on their backs. When both pallets are full (10 racks), no new unit starts; the station panel reads "OUT full (10 on 2 pallets) / awaiting collection". The stage-4 lift goes to the top of the stack it joins.
- **Hologram:** shows the green OUT bay with both pallet outlines in white.
- **Saves:** schema 14. A schema 13 station grows at its OUT end only (its centre moves 1 ft 3 in toward OUT), so its bench, bins, machines, welders and IN stay where they were. A rack on the old cradle moves onto pallet 1. A schema 13 plant whose longer station no longer fits its spot is rejected by validation, not shrunk.
- **Starter shop:** the reorganised starter still fits, with its 4 ft access ring and both forklift bays.
  - The station is centred at (-1768.02, 1823.12), 1 ft 3 in east of the 17 ft station's centre, and stays flush against the west fork lane.
  - The aisle crossing moves 9.4 in east, from x -1394 to x -1370 cm. At the old position the station overlaps the crossing by 23.16 cm (9.1 in).
  - The untouched starter shop has no valid pose, as before; it needs the fork lane, walkway and crossing painted first.

Test: `Foundryline.Factory.WeldingStation.OutPallets` covers:

- First pallet fills first, the second starts at its own position, and the station waits at 10.
- A collected pallet frees its position and the next rack starts it again.
- Every rack carries one kit's mass, and both stacks survive a save.
- Invalid stacks are rejected, and a schema 13 save migrates.

`Layout` also checks the pallets, stacks, bay and machine clearances in every turn and swap.

### Phase 3 — One-welder assembly

Assign one employee, wait for attendance, reserve a kit, show fit-up/hologram-to-solid progress and complete one product exactly once. Prove pause/save/resume and missing parts behavior.

### Phase 4 — Second welder

Add the opposite slot, exclusive claims and configurable 170% total rate. Preserve the same WIP while either worker arrives/leaves. Distinct guns and weld points must fit actual Handyman reaches.

### Phase 5 — Commercial integration

Enable eligible headache-rack demand, finished-product inventory, storage/collection, carrier footprint/weight validation, delivery and payment. Keep direct bracket/plate sales working.

Implemented 29 October 2026, on `welding/phase5-commercial`:

- **Demand:**
  - One headache-rack contract is offered at a time, only while the business has a welding station. It is 2 or 3 racks at $450.00 each and allows 72 game hours.
  - Rack offers use their own serial, so the cut-part offer rotation is unchanged.
  - Accepting a rack order without a station is refused: "This order needs a welding station to build headache racks".
- **Collection from OUT:**
  - A forklift always takes a whole OUT pallet, from the stop in front of that pallet. An empty pallet takes its place. A pallet the forklift has been sent for takes no more racks.
  - A pallet is collected when it is full (5), when its station has no batch and no unit on the bench, or when an accepted rack order needs more than finished-goods storage holds free.
  - If the loading-bay order still needs every rack on the pallet, it goes straight to the pickup zone. Otherwise it goes to a finished-goods rack slot, either one already holding racks with room or an empty, non-supply slot. Racks are stored only where the shelf above clears the stack: 130 cm shelf pitch takes 4, the top level any number.
  - Only stations on a forklift route's sources are collected. New stations join every route, and schema 14 adds saved ones.
  - Placement checks that a forklift can reach the stops for both OUT pallets.
- **Carriers:**
  - A 6 ft 6 in rack needs a deck at least that long. The pallet van (160 cm) is refused; the flatbed, rigid truck and semi take it.
  - A rack pallet takes 2 deck spaces. The pallet weight is the racks' kit mass plus 60 lb per space.
  - The loading tablet says why a van can't take a rack pallet, and the server refuses the booking anyway.
- **Delivery and payment:** these use the existing booking, manifest and invoicing rules; a rack order is paid once, on delivery or per delivery as its terms say.
- **UI:**
  - **Overview**, for a rack order, steps through:
    1. Assign a welder to the station (and a forklift driver).
    2. "Set Welding Station 01: make N headache racks". The *Set the station* button sends the normal validated `ConfigureWeldingStation` command for what the order still needs, after racks already made or in progress.
    3. "Welding racks: N left".
    4. "Book a pickup truck: flatbed or larger".
  - The Overview also flags a held-up station in its own status words.
  - **Orders:** the order detail shows "Set welding station" instead of "Program plasma".
  - **Stock:** a separate FINISHED PRODUCTS card shows racks on OUT, stored, at the bay and on trucks. Pallets on a station show as "Welding station IN/OUT".
  - **Production:** the welding station appears with its status, what it is making, and racks on OUT.

Tests:

- **`Foundryline.Factory.WeldingStation.Commercial`** covers:
  - no rack offer without a station; an offer once there is one;
  - the Overview steps;
  - a 2- or 3-rack order going from OUT straight to the pickup zone;
  - the van refused, the flatbed's two spaces, the rigid truck booked;
  - a mid-delivery save;
  - payment exactly once with no racks left behind;
  - 6 racks for stock stored as 5 + 1, each under enough headroom.
- **`Foundryline.Commerce.TwoRecipeRepeatSales`** still passes, so bracket and plate sales are unchanged.
- **The multiplayer validator** (`validate_welding_station_network.py`) adds client-driven Phase 5 steps:
  - two clients race to accept the rack order; it is accepted once;
  - a client sets the station from the order;
  - every peer sees the racks staged after the forklift collects from OUT;
  - a client's van choice is refused;
  - a client books the rigid truck;
  - every peer sees the order paid exactly once, with the same cash.
- **The rendered review** (`review_welding_phase5.py`) shows the same flow in PIE.

### Phase 6 — Focused native and multiplayer acceptance

Run the actual player flow, relevant authority/save tests, a five-player/independent-company check and clear visual/animation review. Do not repeatedly rerun unrelated broad suites without a changed dependency or failure.

#### Phase 6 review report (29 October 2026, `welding/phase6-acceptance`)

**Player flow from the small cut-parts shop, with no stock fixtures.** Both the automation test and the rendered PIE run follow the same steps:

1. Paint the fork lane, walkway and crossing, then place the station.
2. Accept a headache-rack order on Sales and send it to the loading bay.
3. Hire a welder and assign them to Welder A.
4. Set the station from the order.
5. Buy frame sections and light kits from the station panel.
6. Buy steel and program the plasma for the plates and brackets.
7. Let the forklift supply IN, the station unload, the welder build, and the forklift take the OUT pallet to the bay.
8. Book the rigid truck and get paid.
9. Hire a second welder for Welder B, take the next rack order and repeat.

- **`Foundryline.Factory.WeldingStation.PlayerFlow`** (simulation, player commands only): passes.
  - 2 + 3 racks are shipped, and each order is paid exactly once.
  - One welder takes 138 plant seconds of welding per rack; two take 80 (×0.58, the configured 1/1.7).
  - Cash goes from $4,000.00 to $4,418.26, and the plant validates throughout.
- **Rendered PIE** (`Tools/Unreal/review_welding_phase6.py`, real controller commands): 8 of 9 checks pass.
  - Both orders are paid exactly once ($900.00 and $1,350.00), Welder B is assigned, and the host ledger is valid.
  - The "made money" check fails: this run ends at $3,540.65. The automation test of the same flow ends at $4,418.26. The difference was not broken down; see the open items.
  - Screenshots are in `Phase6/` on `review/welding-screenshots`.
- **Independent companies** (`Tools/Unreal/validate_welding_companies_network.py`, five players on one listen server): 19 of 19 checks pass.
  - Two players found businesses on their own plots, and a third joins A; the host and the fifth player stay in the starter co-op.
  - A and B each place their own station, set its batch, hire and assign their own welder, and build a rack in their own plant.
  - Every member sees their own company's station; the starter plant stays empty.
  - B's remove, add and configure commands aimed at A's real plant are refused without change ("That welding station belongs to another plant"), and so is the co-op player's configure. B's attempt to put its new hire on A's station is refused.
  - Members of each company agree on its cash.
- **Co-op multiplayer** (`validate_welding_station_network.py`): 41 of 41 checks pass (Phases 1–5, with late joiners).
- **Automation:** `Foundryline.Factory`, `Employees`, `Commerce`, `ERP` and `UI` together pass 63 of 63.

**Visual review.** What reads well:

- The bench is the station's only bench. At Ben's request the small parts-bay table was removed, and the kit stock now sits on the two IN supply pallets, not the floor.
- Two OUT floor pallets carry flat stacks of finished racks.
- The blue IN and green OUT paint and the hologram (IN/OUT bays and pallet outlines) read clearly.
- Each welder wears a hood and works with their own machine, gun and lead. With two welders, they stand on opposite sides of the rack.
- The forklift takes whole OUT pallets, and stored stacks sit under clear headroom.
- The office Home page walks a rack order step by step and flags a held-up station in its own words, for example "Missing steel plates / cut more on the plasma table".

Open items found in this review (not fixed here):

- **Rendered run's cash:** the PIE run ends $459.35 below its starting cash although the automation test of the same flow makes $418.26. I did not capture its cost breakdown; the likely suspects are staff wages over the longer real-time run and leftover kit stock. Check the Money page before tuning rack prices.
- **Hidden station panel:** in Construction view the station panel is hidden, so the panel's BUY FRAMES / BUY LIGHT KITS buttons do not show in the Phase 6 shots. They are shown in the Phase 2 and Phase 5 shots.
- **Wide shots:** the overview and eye-level shots are framed wide; a closer first-person pass with a player walking the station was not done.

### Acceptance checklist results (29 October 2026)

Every item below has an automated check, a multiplayer check or a rendered review behind it. See the Phase 1–6 notes and PRs #6–#18 and the Phase 6 PR.

## 16. Acceptance checklist

### Physical and visual

- [x] Bench is a separate 8 ft x 4 ft x 36 in asset, with no permanently embedded rack.
- [x] Orange border, blue IN and green OUT are floor-aligned and legible in first person.
- [x] Both 4 ft worker spaces and outside dock approaches remain clear after rotation/side swap.
- [x] Full-size rack fits the fixture with actual contacts and mount clearance.
- [x] Both Handyman workers reach their weld points without intersecting each other or the product. (Checked visually in the rendered reviews only; there is no automated reach check.)
- [x] Guns, cables, sparks and light follow real active weld locations.
- [x] Full-size finished rack fits OUT; its collision is removed when collected. (The fit is tested; collision removal follows the pallet record and is not separately tested.)

### Inventory and production

- [x] Four single-type supplies pass through IN without the first pallet blocking all other ingredients.
- [x] No part type is duplicated, lost or silently substituted.
- [x] Missing parts produce a clear status/action, not partial free completion.
- [x] One worker completes a rack; two finish faster at the configured shared rate.
- [x] One bill produces one unit with either worker count.
- [x] Leaving/pausing preserves progress and installed material.
- [x] Full OUT prevents another unit from starting; existing WIP is not destroyed.
- [x] Collector moves the actual output record to storage/shipping.
- [x] Finished deliveries pay once, and the next batch/order can run.

### Authority and persistence

- [x] Two co-op players racing for the same employee slot have one winner.
- [x] A person cannot occupy both slots or another active job.
- [x] Foreign-company staff, stock and station commands are rejected without mutation.
- [x] Save/load during delivery, partial assembly and output waiting restores correct state. (Tested in memory; no normal save slot was written.)
- [x] Fifth late join sees the same workers, product progress and company-authorized data.
- [x] Existing cut-part orders, names, forklift cargo, property saves and larger prototype zones retain their behavior.

### Actual playtest completion

Starting from a small shop, a tester can place the station, acquire its parts, hire/assign one welder, make and ship a rack, then assign a second welder and complete another batch faster. They can identify missing supplies or blocked output from normal UI without developer intervention. Record real balances, materials, progress and elapsed simulation work separately from cosmetic preview timings.

## 17. Existing foundations and references

The bench/rack art and current factory/employee systems are inputs to this plan, not proof the new station loop exists already.

- [Architecture](../ARCHITECTURE.md)
- [Starter bracket/plate sales](../Architecture/STARTER_BRACKETS_AND_PLATES.md)
- [Generic hire and job assignments](../Architecture/EMPLOYEE_JOBS.md)
- [Handyman character integration](../Architecture/HANDYMAN_EMPLOYEES.md)
- [First-order inventory/freight](../Architecture/FIRST_ORDER_LOOP.md)
- [Physical collision](../Architecture/PLAYER_COLLISION.md)
- [Public asset/content boundary](../REPOSITORY_CONTENT.md)

Local art references:

- `SourceAssets/Production/Equipment/WeldingBench/Foundryline_StandardWeldingBench.blend`
- `SourceAssets/Production/Equipment/WeldingBench/StandardWeldingBench_SeparateComponents.fbx`
- `SourceAssets/Production/Equipment/WeldingBench/Foundryline_WeldingFixtureKit.blend`
- `SourceAssets/Production/Equipment/WeldingBench/WeldingFixtureKit_SeparateComponents.fbx`
- `SourceAssets/Products/HeadacheRack/`

Some art remains local/ignored because public-repository publication requires provenance review. This planning document may be published without changing those asset rules.
