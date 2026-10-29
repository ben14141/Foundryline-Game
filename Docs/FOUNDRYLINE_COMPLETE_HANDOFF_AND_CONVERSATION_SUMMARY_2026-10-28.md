# FOUNDRYLINE — Complete Game Handoff and Conversation Summary

**Snapshot date:** 28 October 2026, America/Chicago  
**Purpose:** A detailed record of what currently exists, how the game works, what we decided in conversation, what was tested, and what remains unfinished.  
**Active Unreal project:** `C:\Users\curly\Documents\Foundryline Game\Foundryline.uproject`  
**Engine:** Unreal Engine 5.8; C++ gameplay foundations with Unreal assets, data assets, generated presentation, and editor tooling.  
**GitHub:** <https://github.com/ben14141/Foundryline-Game>  
**Published source baseline:** `main`, commit `3a886e90239cbb3ec695f4cfdeded154dbd4f032`.

This document is a synthesis, not a verbatim chat transcript. It reconciles the conversation with the current source, configuration, and acceptance documents. Older milestone documents sometimes describe earlier behavior; later decisions and current implementation take precedence.

The latest direction is to make **selling brackets and plates genuinely playable first**, while preserving the larger factory, tanker, construction, logistics, and multiplayer foundations for later progression.

---

## Contents

1. Executive overview and status terminology
2. Permanent design requirements
3. Current maps and starting conditions
4. Playable starter loop and player controls
5. Steel, recipes, parts, inventory, and storage
6. Orders, customers, payments, and economy
7. Freight, receiving, shipping, and vehicle capacity
8. Employees, free hiring, and equipment/job assignments
9. Handyman characters and animation integration
10. Office ERP and three-monitor workstation
11. Construction, roads, buildings, offices, roofs, and landscaping
12. Lighting, rendering, scale, and physical collision
13. Logistics routing, floor lanes, and traffic foundations
14. Machinery and authored product assets
15. Tanker manufacturing prototype
16. Multiplayer, companies, authority, and privacy
17. Persistence and save boundaries
18. Code, assets, tools, and repository structure
19. Validation and actual playtest evidence
20. Conversation history and decisions that changed
21. Known gaps, limits, and risks
22. Recommended next work and acceptance criteria
23. Detailed handoff checklist and source references

---

## 1. Executive overview and status terminology

### 1.1 What is playable now

The small-shop starter supports this connected loop:

1. Start in first person near the office computer.
2. Open the ERP and review customer demand.
3. Accept a supported bracket or plate order.
4. Assign that order to the pickup area.
5. Purchase steel sheets and schedule their delivery.
6. Ensure a worker operates the plasma and a driver operates the owned forklift.
7. Program the plasma with the correct fixed recipe and quantity.
8. Receive and rack steel automatically through the forklift system.
9. Cut parts, store them, and stage matching finished pallets.
10. Select a carrier and actual staged pallets on the loading tablet.
11. Pay the freight booking fee and wait for loading, departure, transit, and customer delivery.
12. Receive payment according to the contract.
13. Use the remaining stock and money to fulfil another order.

This was played through for **40 bracket blanks followed by 60 mounting plates**, using ordinary mouse/keyboard UI and the shipped speed controls. Both orders delivered and paid. The second order used retained material instead of resetting the shop.

### 1.2 What exists beyond that loop

- Modular construction, roads, parking, curbs, planting, building expansions, office rooms, doors, windows, trim, roofs, skylights, and lighting.
- A larger manufacturing test shop with plasma, brake, roller, racking, product zones, welders, and tanker assembly.
- Employee hiring and assignment with persistent names and business ownership.
- Handyman skeletal characters replacing the temporary employee bodies.
- A detailed three-monitor office computer with live business displays.
- Separate receiving/shipping bookings, shared-bay queues, named carriers, and partial-pallet shipments.
- General road/logistics route-planning tools and node/turn-radius foundations.
- Independent-company ownership and permission foundations for multiplayer.
- Versioned saves and extensive local automated/native/multiplayer acceptance.

These systems do **not** mean the full manufacturing tycoon progression is complete. Some are prototypes or foundations, and several are not yet connected into the starter's commercial progression.

### 1.3 Status definitions used here

| Label | Meaning |
| --- | --- |
| Playable starter | Connected to the small-shop bracket/plate business and exercised in the current gameplay loop. |
| Implemented foundation | Real code or content exists and has relevant validation, but the complete player-facing progression around it is unfinished. |
| Separate prototype | Available in another map, profile, fixture, or practice workflow; not a claim that it starts installed in the small shop. |
| Planned / deferred | Agreed direction or future work, not a finished feature. |
| Known limit | A concrete gap or unaccepted operating condition. |

---

## 2. Permanent design requirements

### 2.1 Genre and experience

The intended game combines satisfying manufacturing/automation with business management and product diversity. Satisfactory and Game Dev Tycoon were discussed as influences. The game should make factory planning, material flow, product choice, business decisions, and expansion interesting without making every pallet movement tedious.

The player should see meaningful physical activity: deliveries, sheets on racks, moving forklifts, workers walking to stations, machines operating, pallets accumulating, products becoming solid as they are assembled, and trucks collecting output.

### 2.2 Scale and units

- **1 Unreal unit = 1 centimetre.**
- Buildings, vehicles, people, machines, props, furniture, and clearances must stay at believable 1:1 scale.
- The UI was later changed to **feet/inches**, with pounds for material and mph where vehicle speeds are displayed.
- Internal simulation and geometry remain in centimetres.
- Structural snapping: 100 cm.
- Fine snapping: 25 cm.
- Precision snapping: 10 cm and 5 cm.
- Snapping must never resize an asset to make it fit.

### 2.3 Multiplayer is mandatory

- Multiplayer must be considered for every gameplay change.
- More than two players must be possible; no two-player-only design assumption.
- Co-op members share their business, equipment, employees, property, orders, and operations.
- Independent businesses must remain separate.
- The server owns inventory, money, production, assignments, claims, and accepted construction changes.
- Cameras, roof preferences, menu selection, and presentation are per player.
- A player may be in first person while another uses Property View.
- Local four/five-player tests are not a production player-cap or internet-multiplayer guarantee.

### 2.4 Camera direction changed during development

Early manufacturing and construction discussion assumed top-down operation. The later, current decision is:

- **First person is the default and main view.**
- Top-down construction/management is accessed from the office computer as **Property View**.
- The loading tablet does not unlock Property View.
- Free camera toggling must not bypass the office requirement.
- Leaving Property View returns the player to their saved office position.

### 2.5 Visual direction

- Foundryline logo/brand should shape the UI and default building palette.
- Clean light software UI, grey accents, restrained orange actions.
- Real business-software character rather than sparse generic game menus.
- Large readable computer pop-ups for first-person use.
- Midday daylight, slightly warm, using Lumen.
- Flat terrain with rich vegetation close to the player, reduced distant detail.
- Quality at the beginning matters; representative art must still be convincing and usable.
- Performance target is at least 60 FPS, but **no current measured 60 FPS acceptance is claimed**.

### 2.6 Automation philosophy

- Employees do repetitive physical transport and station work.
- Players choose what to make, quantities, assignments, storage/routing, orders, and bookings.
- Inventory is exact in data, approximate in visual instance count.
- Clear zones, good floor paint, and understandable UI should explain the factory without requiring a permanent debug overlay.

---

## 3. Current maps and starting conditions

### 3.1 Default playable map

`/Game/Foundryline/Maps/L_CutPartsStarter`

Both EditorStartupMap and GameDefaultMap currently point to the cut-parts starter.

The small shop is approximately **28 m × 22 m** internally in the authored layout, with an attached office area containing two rooms. It has one shared receiving/shipping opening, a paved freight apron and street connection, plasma equipment, raw storage, finished-pallet storage, a forklift, and a pickup area.

### 3.2 Fresh starter state

- Day 1, approximately 08:00.
- $4,000 working capital.
- Simulation paused for setup.
- No free raw sheets or finished stock.
- No automatically running production batch.
- One owned plasma table.
- One owned forklift.
- Two original named staff, with starter operator/driver assignments.
- Raw-sheet rack and finished-pallet storage.
- Two physical finished-pallet pickup positions.
- Office ERP terminal and loading tablet.
- First-person player near the office computer.

Names are generated/stored by the employee system; a fresh session's specific names are not fixed canonical characters.

### 3.3 Other preserved maps

The local content also includes:

- `L_ConstructionSite`: larger construction/manufacturing practice environment.
- `L_FlatForest_Grass`.
- `L_FlatForest_Clean`.
- `L_FlatForest`.
- `L_StartupSite`.

The larger tanker workshop is a separate profile/prototype. It should not be confused with the empty-stock commercial starter.

### 3.4 World direction

The world is intended to grow into a town supporting multiple player businesses, road traffic, deliveries, and expansion. The current starter is a bounded authored site, not a completed procedural multiplayer town economy.

---

## 4. Playable starter loop and player controls

### 4.1 First-person interactions

| Action | Current interaction |
| --- | --- |
| Walk/look | Standard first-person movement and mouse look. |
| Office computer | Approach and press E. |
| Machine computer | Approach its control screen and press E. |
| Loading tablet | Approach and press E. |
| Personnel door | Press E to swing it open/closed. |
| Inspect pallet | Look toward a nearby pallet and use E; raised rack stock can be selected. |
| Close computer UI | E/Escape or the relevant close control, depending on the active screen. |
| Property View | Open from the office ERP. |
| Return from Property View | Return to Office / F. |
| Simulation speed | Pause/Resume and 1x/4x/8x controls in the office ERP. |

The exact current UI labels take precedence over older prototype control sheets.

### 4.2 Example first order

1. Open Sales in the ERP.
2. Select an eligible basic order, such as 40 bracket blanks.
3. Review quantity, price, payment terms, and deadline.
4. Accept it; any deadline starts immediately.
5. Assign the accepted order to the pickup bay.
6. Purchase enough sheets in Purchasing.
7. Confirm employees are assigned to the plasma and owned forklift.
8. Use **Program this order** to prefill the plasma recipe and outstanding quantity.
9. Explicitly start the batch.
10. Run at the chosen simulation speed.
11. Let incoming freight and the forklift put raw steel on its rack and feed the machine.
12. Let the operator reach the real console before cutting starts.
13. Finished stock is racked and matching pallets are staged for the assigned order.
14. Open the loading tablet, choose a suitable carrier, and select actual staged pallets.
15. Book pickup and pay its fee.
16. Wait for the truck to arrive, dock, load, leave, travel, and deliver.
17. Check delivery and payment in ERP records.
18. Accept another supported order and repeat.

### 4.3 Important behavior

- Starting a valid batch resumes the plant; machine pause and plant pause remain separate concepts.
- A worker being employed is not enough: they must be assigned to the correct equipment.
- A machine operator must physically reach the station before its operation progresses.
- A truck's departure is not customer delivery and does not by itself pay a delivery-based contract.
- Existing finished stock and retained sheet pounds should be used instead of blindly making/buying everything again.
- Guidance distinguishes incoming steel, production needs, and finished goods awaiting pickup.

---

## 5. Steel, recipes, parts, inventory, and storage

### 5.1 One starting steel format

The agreed starting material is carbon-steel sheet, purchased by weight but delivered as whole sheets.

| Property | Current value |
| --- | --- |
| Width | 60 in / 152.4 cm |
| Length | 120 in / 304.8 cm |
| Thickness | 1/8 in / 0.3175 cm |
| Nominal density in calculation | 7,850 kg/m³ |
| Calculated sheet weight | Approximately 255.24 lb; older notes round it to 255.25 lb. |
| Purchase price | $1 per lb, with whole-sheet purchase cost rounded to cents. |
| Supplier freight | $25 per booking. |
| Purchase quantity | 1–10 whole sheets per order. |

### 5.2 No nesting

The current material model intentionally avoids sheet nesting and complex offcut geometry.

- Every part recipe consumes a predetermined number of pounds.
- The plasma deducts those pounds from its loaded material balance.
- The player can change recipes without discarding the remaining sheet weight.
- A queued program change waits for the current cut to finish.
- When more steel is needed, the next sheet adds its weight to any remaining balance.
- Current starter behavior does **not** discard usable remainder as scrap.

Some older tanker/process documents describe remnant scrap. That is historical and should not override the later carry-forward implementation.

### 5.3 Starter saleable recipes

| Part | Steel per piece | Plasma time | Additional operation | Base sale price | Pallet capacity |
| --- | ---: | ---: | --- | ---: | ---: |
| Bracket blank | 5 lb | 8 s | None for basic sale | $11 | 50 |
| Mounting plate | 2 lb | 4 s | None | $5 | 100 |
| Gusset plate | 3 lb | 6 s | None | $7 | 75 |
| Reinforcement plate | 8 lb | 12 s | None for basic sale | $17 | 30 |
| Bent bracket | 5 lb upstream blank | 8 s upstream | Brake: 8 s | $19 | 50 |
| Support channel | 8 lb upstream reinforcement plate | 12 s upstream | Brake: 12 s | $30 | 20 |

Prices and times are initial balancing values. Each actual offer stores its quoted price, which can differ because of its terms. Machine times are simulation recipe values, not real industrial cycle claims.

Bent brackets and support channels are progression recipes. Starter offers for them are unavailable until the required brake is installed and staffed. A stale client request cannot bypass that server-side capability check.

### 5.4 Other prototype part types

The catalog also contains rail blanks, finished frame rails, shell blanks, rolled shells, whole dished tank heads, running-gear kits, and fittings kits for the tanker prototype.

### 5.5 Exact data, representative meshes

- One pallet holds one part type.
- Data tracks exact quantities, weights, reservations, ownership, and booking locks.
- The mesh may display a handful of samples instead of every item.
- Steel stacks commonly show up to three representative sheets while the data may contain ten.
- Finished pallets similarly show a limited number of pieces.
- A pallet containing one bracket is valid and can ship.
- Pallet capacities differ by part; future research-driven capacity increases were discussed but are not implemented.

### 5.6 Storage and machine flow

- Incoming sheets go directly from the truck to designated raw-sheet racking.
- There is no extra receiving-floor staging transfer for that steel.
- Plasma output goes into linked finished/intermediate storage.
- Empty suitable rack positions can be assigned automatically to output types.
- Intermediate brake/roller inputs use linked rack positions in the larger workflow.
- Output batches flush partial pallets when needed, including at the end of a batch.
- Full storage causes work/transport to wait rather than silently deleting or duplicating inventory.
- Whole-pallet transfers preserve pallet identity; split quantities create the appropriate separate records.
- Reservations and booked-manifest locks are separate mechanisms.

---

## 6. Orders, customers, payments, and economy

### 6.1 Demand sources

The business uses a hybrid customer system:

- Public orders/marketplace demand.
- Customer email offers through the office ERP.

Each offer records a customer, document/reference number, part, quantity, unit price, payment terms, and optional completion period. Customer/company marks make those records feel like real business documents.

The offer pool replenishes. Completed history is bounded; the player is not limited to a finite starter campaign of eight jobs.

### 6.2 Acceptance and deadlines

- Acceptance starts the order timer.
- Flexible orders may have no deadline.
- Standard and rush terms exist.
- Rush terms can pay better.
- At most six accepted orders are active at once.
- Late status is tracked.
- Reputation, traits, long-term relationships, and meaningful late penalties remain future systems.

### 6.3 Payment modes

| Mode | Current behavior |
| --- | --- |
| Upfront | Customer pays when the order is accepted; delivery must not pay again. |
| Per delivery | Each delivered load pays for its delivered quantity. |
| On completion | Payment arrives when the full required quantity has been delivered. |

Partial loads are allowed, as long as the order is ultimately fulfilled under its terms. Every delivery is applied exactly once. Bookings cannot over-commit the remaining order quantity.

### 6.4 Company money

- Currency is stored as integer cents.
- Each business has its own account.
- The server registry is the authority for company balances.
- Expected-balance commits prevent a stale transaction from overwriting a changed account.
- Equipment operating costs are distinct from employee payroll.
- Hiring is free; payroll is disabled.
- Existing operating-cost amounts were retained when the old wages label was corrected to equipment operating costs.
- The account journal is bounded, with consecutive operating entries aggregated.
- ERP distinguishes cash, spending, earned revenue, and upfront receipts rather than inventing a single fake profit number.

### 6.5 Verified repeat-sale economics

The normal-input two-order playthrough used one bulk purchase of two sheets:

| Item | Amount |
| --- | ---: |
| Starting cash | $4,000.00 |
| Two sheets plus one supplier delivery | -$535.48 |
| Two flatbed pickups | -$80.00 |
| Actual equipment operation during that run | -$12.97 |
| Forty brackets delivered | +$440.00 |
| Sixty mounting plates delivered | +$300.00 |
| Ending cash | **$4,111.55** |
| Delivered revenue | **$740.00** |
| Remaining steel | Approximately **190.5 lb** |
| New offers available | **8** |

The deterministic backend version buys sheets separately and has different operating timing. It ended at $4,084.93. Those figures are different test purchasing strategies, not conflicting claims about one run.

### 6.6 Initial upgrades

- Press brake: $4,500, with relevant input storage integration.
- Second forklift: $1,200 for equipment; staff are hired separately for free.
- Newly purchased equipment is unstaffed until the player assigns someone.
- The starter has two forklift parking stalls.

Source arithmetic showed that around five eligible plasma-only contracts can fund the brake under the current balancing. One example produces $2,274 revenue and consumes 1,035 lb from five sheets, leaving approximately 241.2 lb. The projected $4,672.80 before operating costs still requires a working-capital reserve. This is a projection, not a separately completed five-order manual playthrough.

---

## 7. Freight, receiving, shipping, and vehicle capacity

### 7.1 Shared bay

The starter begins with one bay used for both incoming steel and outgoing goods. Overlapping bookings are allowed and create queues. The player can inspect the calendar and organize them, or accept the resulting congestion.

- Waiting arrivals use FIFO ordering.
- A full raw rack can hold a supplier at the bay.
- The truck must actually finish docking before material handling starts.
- The current starter carrier reserves about 12 game minutes for docking and 20 for departure movement.
- Larger future properties may add bays; a complete purchase/upgrade progression for that is not part of the focused starter.

### 7.2 Shipping workflow

1. Assign an accepted order to the pickup zone.
2. Matching finished goods stage automatically from racks.
3. Open the loading tablet.
4. Choose the carrier/vehicle first.
5. Select individual actual staged pallets.
6. Check pallet slots and gross weight.
7. Book the pickup and pay its flat fee.
8. Selected pallet IDs and quantities become locked for that booking.
9. Forklift drivers load the selected pallets.
10. The carrier leaves and enters transit.
11. Delivery updates fulfilment and payment.

The pickup zone is assigned to one accepted order at a time. Existing staged stock must be cleared before switching it to another job.

### 7.3 Current carrier catalog

| Carrier | Service | Pallet slots | Payload | Fee | Lead | Transit |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| Swift Van Couriers | Pallet van | 1 | 900 lb | $25 | 15 min | 60 min |
| Foundry Lane Haulage | Two-pallet flatbed | 2 | 2,000 lb | $40 | 20 min | 90 min |
| County Freight | Rigid delivery truck | 6 | 6,000 lb | $90 | 40 min | 120 min |
| Interstate Pallet Lines | Box semi | 20 | 24,000 lb | $180 | 60 min | 240 min |

Long sheet pallets count as two nominal spaces. Gross weight includes pallet/support tare: 60 lb per nominal space in the current catalog.

The commercial capacities are implemented. Only the purchased starter flatbed has the detailed converted presentation described below; other catalog vehicle classes retain temporary visuals.

### 7.4 Booking rules

- Flat pickup fees encourage organized/full loads without banning small partial shipments.
- Rebooking before dispatch costs half the **original** fee each time.
- Once the carrier is travelling, the booking is locked.
- Stale pallet quantities, duplicate selections, already-booked pallets, excess weight/slots, and over-fulfilment are rejected by the server.
- Booking/payment race tests confirm charges and deliveries are not duplicated.

### 7.5 Actual starter truck

The purchased Megapoly.Art Trucks sidewall flatbed was converted from its Unity source using the author prefab transforms, avoiding the raw FBX hierarchy's inconsistent units.

- Original complete bounds: approximately 1,055.83 × 317.21 × 405.67 cm.
- Six separately pivoted wheels.
- Front-only steering, bounded around 35 degrees.
- Wheel rotation derived from actual displayed travel and measured wheel radius.
- Opening/closing side loading gate.
- Corrected cab materials and glass.
- Measured bed height approximately 112.951 cm above tyre ground contact.
- Pallet base is approximately 133 cm world Z on the starter road surface at Z=20.
- Forklift lifts its mast/load to the deck; the forklift root stays at ground height.
- Sheet supports and cut-part samples align with the bed.
- Rounded fixed starter arrival, docking, queue, and departure paths.
- Bounded relevance and a limited pool of active carrier actors per plant.

The Blender source rig has travel, steering, and gate controls plus a 420-frame demonstration action. Unreal drives the actual components from booking movement rather than playing a canned driving animation.

### 7.6 Vehicle boundaries

The current carrier routes are fixed to the starter site. Editing all roads or the property does not automatically regenerate them. The carrier presentation is not a general player-drivable traffic AI vehicle. Kinematic vehicles block a player's own walking but do not yet fully yield to or sweep against a stationary human player.

---

## 8. Employees, free hiring, and equipment/job assignments

### 8.1 Current hiring contract

**Hire a person first. Give them a job afterward.**

- New hires are named, unassigned people.
- Hiring costs $0 for testing.
- Payroll is not enabled.
- The testing headcount limit is 12 per business.
- Names are random first/last names generated once on the server and stored.
- Co-op members see the same company roster.
- Separate businesses own separate employees.
- A person is not hired directly into a permanently fixed profession.

Earlier iterations used separate operator/driver/welder hire cards and role-specific caps. Those were superseded by generic hiring and explicit job assignment.

### 8.2 Assignable jobs

| Job | Specific target |
| --- | --- |
| Machine operator | An installed plasma table, press brake, or sheet roller. |
| Forklift driver | An owned, parked, available forklift. |
| Welder | A specific product work zone and its physical welding station. |
| Unassigned / released | Wait safely off the working lanes until given another job. |

The job does not replace machine programming or product orders. The machine owns its recipe/batch; the forklift dispatcher owns material-handling tasks; the product zone owns assembly work.

### 8.3 Stable identity

An employee keeps their ID, name, company/plant key, and visual identity when changing roles. The implementation moves or binds their backing role record atomically rather than creating a replacement person.

- One roster row per employee.
- One physical employee representation.
- Consistent appearance across role changes and clients.
- No free forklift appears when hiring a driver.
- Vehicle IDs and in-flight cargo remain separate from employee identity.

### 8.4 Exclusivity and safe release

- One assigned worker per machine station, forklift, or new welding-zone claim.
- Competing claims are evaluated server-side; only one wins.
- An employee cannot claim another business's equipment.
- Release the current job before choosing a new one.
- A machine part in progress requires a safe paused handoff; reserved material and remaining time are retained.
- A driver cannot leave during a pallet task or return-to-parking trip.
- Active welding must be paused before release; product reservations and installed progress remain.
- Released foot workers walk back to their safe waiting/home position before another assignment.
- Forklift boarding/unboarding is currently an immediate attachment transition, not a full climbing animation.

### 8.5 Physical attendance

- Operators walk to the actual control station.
- Production waits for arrival, not merely for a role flag.
- Welders only approach the product when a real zone order needs work.
- Idle workers wait off painted traffic lanes where safe space is available.
- Forklifts route around standing employees and use immediate separation checks.
- Wide personnel lanes support bounded physical passing; narrow spurs may still require waiting.
- This is not a complete crowd simulation or human-player yielding system.

### 8.6 Priorities

Forklift drivers can prioritize Truck, Machine, and Storage work. The intention is automatic material movement under player-selected priorities, not manually clicking every transfer.

### 8.7 Staffing by business

The ownership contract is explicitly company/plant scoped. A shared company shares staff. Independent businesses keep their own roster and commands. Public physical people may be visible nearby, while private employment/operations access remains authorized.

The backend supports distinct businesses, but a fully player-facing new-business creation/plot-start menu and combined persistent town save are still unfinished.

---

## 9. Handyman characters and animation integration

### 9.1 Purchased pack

**Handyman Modular low-poly stylized**, by Agor2012, was added through the Fab/Epic library.

Original content stays at `/Game/Handyman`:

- 267 assets.
- 84 skeletal meshes.
- 52 static tools/props.
- 39 animation sequences.
- 74 Blueprints.
- Two skeletons, including the humanoid skeleton and a separate tool skeleton.
- Materials, textures, Control Rigs, and demonstration content.

The native Unreal 5.7 package was added to the Unreal 5.8 project and inspected successfully. Original vendor content remains unmodified.

### 9.2 Game-owned integration

`/Game/Foundryline/Employees/Handyman/DA_HandymanEmployees` supplies the appearance profile.

- Three coherent modular outfit variants.
- Work clothing, boots, caps, and varied heads/facial hair.
- Approximately 182 cm authored character height, with soles at ground level.
- Facing correction adapts native model orientation to simulation forward.
- Stable variant selection from the company/plant/employee key.
- Same appearance after reassignment, pooling, save/load identity, and late join.

### 9.3 Runtime approach

- Original native body parts are merged into three shared runtime outfit resources per profile/process.
- These are cached shared meshes, not a new mesh merge for each employee.
- One visible skinned body represents each employee.
- A hidden native skeletal sampler evaluates included idle/walk clips.
- Bone poses map by bone name into the assembled outfit.
- Actual smoothed travel distance drives walking phase.
- Leg IK handles grounded stance/swing.
- Bounded arm IK fits keyboard, steering wheel, and welding gun targets.
- Standing player collision remains a separate capsule; it is disabled while seated.
- Skeletal animation does not own inventory, root-motion authority, employee position, or job completion.
- Dedicated servers retain required collision/state without loading character render meshes.

### 9.4 Why merged outfits are transient

An attempted saved runtime-merge asset failed independent reload because it lacked the editor imported source model. That approach was rejected. Final profiles reference persistent original parts and rebuild valid shared runtime resources. No invalid saved merged character assets remain as the final integration.

### 9.5 Verified poses

- Grounded walking across the tested gait phase range.
- Idle and paused behavior.
- Plasma operator at actual keyboard controls.
- Forklift seating, boots at cabin floor, hands at steering wheel, head within cab.
- Welder holding the real moved gun, with sparks/light at actual product work.
- Translated/rotated business transforms.
- Stable single body across job changes and peers.

### 9.6 Remaining character-art work

- Specialist welding PPE and bespoke job gestures.
- Climbing into/out of forklifts.
- Additional character variety beyond the current stable variants.
- Roller console layout: its existing screen is too far from the operator's normal reach.
- More complete pedestrian passing/yielding behavior and measured crowd scalability.

The previous articulated prototype remains useful editable source/reference, but it is no longer the default visible employee body when the Handyman profile is active.

---

## 10. Office ERP and three-monitor workstation

### 10.1 UI direction and evolution

The UI went through several revisions because the initial construction HUD was too small, noisy, and insufficiently connected to the Foundryline logo. The office UI then moved from sparse cards to a large desktop and finally a more convincing ERP.

Current direction:

- Near-fullscreen desktop window, approximately 96% of the viewport.
- Light surfaces, layered greys, restrained orange highlights.
- Stronger regular/bold typography.
- Application chrome, navigation rail, taskbar/status context.
- Dense sortable tables.
- Summary, Document, and Activity views where applicable.
- Separate email list and reading pane.
- Distinct company marks; eleven marks were authored for customers/carriers/suppliers.
- Branded offers, purchase orders, carrier records, statements, and receipts.
- Actual recorded business data rather than decorative fake metrics.
- Readable first-person use and clear next actions.

### 10.2 ERP modules

| Module | Main purpose |
| --- | --- |
| Overview | Cash/status, demand, activity, upcoming arrivals, attention/next actions, speed controls. |
| Sales | Public orders, customer mail, accepted/completed work, buyer documents and terms. |
| Purchasing | Sheet quantities, supplier quote, freight timing, actual purchase records. |
| Production | Installed work centres, recipe/batch access, current production state. |
| Inventory | Physical pallet ledger, part/quantity/location/reservation/booking context. |
| Dispatch | Freight bookings, arrivals, shared-bay calendar, carrier records. |
| Accounts | Real cash/spending/revenue data, retained journal chart, statements and payment receipts. |
| Team & logistics | Named staff, hiring, job/equipment assignment, current work, driver priorities. |
| Equipment / Property links | Installed/purchasable equipment and authorized Property View access. |

Some items are linked panels rather than separate operating-system windows. Labels can evolve without changing the underlying authority model.

### 10.3 Document/data integrity

- Document numbers and partner identity derive from actual records.
- Receipts represent retained real payments.
- Missing timestamps are not invented.
- The retained-journal graph does not claim to be unlimited full-history accounting.
- Cash, debt, earned sales, and upfront payment are distinguished.
- Sorting, tabs, and document navigation do not issue business mutations.
- All accept/purchase/book/hire/assign operations retain server validation.

### 10.4 Three-monitor hardware

The single-screen computer was replaced at the user's request.

- Desk footprint: 152 × 76 cm.
- Work surface height: approximately 78 cm.
- Three matching active panels: 52 × 29 cm each.
- Side panels angled inward 25 degrees.
- Overall hardware bounds approximately 162.7 × 76 × 127.8 cm.
- Three-arm clamp mount, reach arms, VESA plates, rear vents/ports.
- Cable grommets, tray, and routed display/USB/power wiring.
- Detailed keyboard keys/legends, mouse, and under-desk tower with vents/ports/screws/feet.
- Foundryline trim and coordinated materials.

### 10.5 Live physical displays

- Centre: company/ERP overview.
- Left: production and sales.
- Right: freight schedule and named team.
- Three distinct 1536 × 864 live render targets.
- Refreshed locally about once per second.
- Private data comes only from the authorized company's state.
- Foreign-business displays do not expose that business's private ERP data.
- E opens the full interactive ERP; the physical screens are readable summaries, not a replacement for the modal controls.

Editable source is retained under `SourceAssets/Operations/OfficeWorkstation`, with dedicated geometry/export/import scripts under Tools.

---

## 11. Construction, roads, buildings, offices, roofs, and landscaping

### 11.1 Placement and editing

The construction practice supports:

- Click-drag rectangle shops and pavement areas.
- Holographic placement previews, blue when valid and red when blocked.
- Preview of the whole building rather than placing each wall individually.
- Structural/fine/precision grid classes.
- Selection, move, resize, rotate, duplicate, delete.
- Doors/windows and opening relocation.
- Property save/load and validation.
- Local selection/hover outlines and quieter success feedback.
- Detailed failure reasons for overlap, bounds, stale edits, clearance, and invalid placement.
- Standalone undo/redo; co-op undo is disabled to avoid reverting another person's work.

The user requested no temporary footprint reservation/lease system. Players can preview the same place; final authoritative commit validation prevents incompatible placement. One request succeeds, the conflicting/stale one is rejected.

### 11.2 Shop tiers

- Tier 1: smaller/simple shop or low-bay uses.
- Tier 2: taller manufacturing space, suitable for later crane/high-clearance uses.
- Tier 2 windows are placed high toward the tops of walls.
- Automatic shop windows and columns distribute evenly along variable wall lengths.
- Preferred column spacing is adjustable, with full-height columns/pilasters.
- Shop windows keep their native scale rather than stretching to arbitrary bay sizes.
- Cap, concrete base, trim, and structural detail were added to improve the initial plain wall appearance.
- Large garage openings were chosen for semis/full-scale vehicles.

### 11.3 Connected buildings

- A new section can snap to an existing section.
- Shared connecting walls are removed/opened rather than leaving a double wall through the interior.
- Tier-height transitions, end infill, columns, and windows received several correction passes.
- Office exterior walls should serve as office walls; nested duplicate wall boxes are not intended.

### 11.4 Offices and rooms

The decision evolved:

1. Offices were initially intended as attached outside annexes rather than rooms placed inside the shop.
2. They needed an outside door and a connection to the shop.
3. Later the user explicitly allowed **separate offices**.
4. Shop-to-office attachment was then required to work in either building order.

Current construction direction supports attached or separate offices, with room placement and manual doors/windows. Office space must preserve real usable dimensions for furniture and circulation.

- Draw the office/open floor area.
- Draw rooms inside it.
- Place doors and windows manually.
- No automatic office windows.
- Office trim/walls should be distinct and clean without duplicated exterior walls.
- Internal office doors now swing inward into rooms to keep the corridor usable.

### 11.5 Colors and roof decisions

- Shop and office interior/exterior wall colors can be customized separately.
- Trim is a secondary color.
- Foundryline branding supplies the defaults.
- Dark roofs and trim were requested later.
- Roofs exist but start hidden in Property View.
- Roofs are always shown in first person.
- Property View has a roof visibility control.
- Skylights were added from available assets.
- Daylight must interact with windows/openings properly; windows are not just opaque decoration.

### 11.6 Roads and exterior infrastructure

- Road surfaces, markings, driveway access, parking areas, sidewalks/curbs where available, and planting areas.
- Pavement uses click-drag sizing rather than repeated size buttons.
- Road and shop slab are at matching practical height, raised/thicker relative to grass without disconnected floating edges.
- No loading ramp in the standard starter concept.
- Flowers, bushes, and small trees in parking landscaping.
- Transitions among roads, sidewalks, curbs, driveways, and floors were reviewed.
- The actual starter truck driveway gap was fixed; its full swept tyres stay on the authored paving.

### 11.7 Major construction boundary

**A larger rendered building does not yet expand the factory simulation's usable production floor/navigation bounds.** Construction is implemented, but a complete expansion purchase/capacity workflow is not connected to the starter business. Do not advertise building expansion as working production progression yet.

---

## 12. Lighting, rendering, scale, and physical collision

### 12.1 Environment and lighting

- Flat grass base, with the stylized forest pack used for vegetation.
- Old unwanted foliage was removed/replaced during scene development.
- Rich nearby 3D grass and reduced distant detail were requested for optimization.
- Midday light, slightly warmer.
- Lumen global illumination/reflections and virtual shadow maps configured.
- Interior hanging ceiling fixtures automatically spaced for shops and office/hall areas.
- Interior brightness was iterated because players will work and walk inside rooms.
- Roof/skylight/window behavior must be considered together.

The existence of these settings and visual checks is not a measured performance guarantee.

### 12.2 Why collision needed a dedicated pass

Many imported visual meshes and instance groups had no usable simple collision, despite looking solid or using a blocking profile. The player could walk through machines, stock, forklifts, and furniture.

The correction uses explicit game-owned query geometry aligned to the actual displayed objects. The test shape is the real walking character capsule: approximately 30 cm radius and 90 cm half-height.

### 12.3 Collision coverage

- Machine bodies and separate consoles/power/extraction units.
- Forklift chassis, mast/cab posts, and roof, without filling the entire open cabin with one invisible box.
- Visible pallets, supports, and representative part stacks.
- Racks and raised stock at their real heights.
- Office desk/computer/furniture fallbacks.
- Welding equipment.
- Carrier chassis, bed, cab, and temporary carrier bodies.
- Building walls, partitions, animated personnel doors, and their openings.
- Nearby birch/broadleaf tree stems.

Tree collision is trunk-only and bounded: up to 4,096 nearby stems within 200 m in the current helper. Bushes, grass, flowers, and canopies remain passable.

### 12.4 Dynamic lifecycle and networking

- Removed/off-site pallets lose their generated collision.
- Hidden, delivered, or in-transit carrier pools do not leave invisible walls.
- Reused vehicles reactivate collision for their actual booking.
- Server and client use compatible object geometry.
- Generated local collision components are not used as unsupported replicated movement bases.
- Low-pallet stepping still collides/steps normally while character movement stays authoritative in world space.
- Real moving platforms retain the normal Unreal based-movement path.

### 12.5 Remaining collision/traffic boundary

Kinematic vehicles block the player's walking input. They do not yet implement a full human-yielding vehicle traffic simulation, swept pushing behavior, or rideable moving-platform system. Collision was intentionally not turned into uncontrolled rigid-body physics for all stock and vehicles.

---

## 13. Logistics routing, floor lanes, and traffic foundations

### 13.1 Property-level planning tools

The logistics overlay evolved from confusing developer lines into a more guided workflow:

- Entrance selection.
- Paint/erase allowed truck pavement.
- Select loading door/staging/parking destination.
- Choose exit.
- Check arrival and departure.
- Show directional route and swept vehicle envelopes.
- Separate Routes, Trips, and Traffic concepts.

Shared permission/route metadata is distinct from each player's local route preview and layer preferences.

### 13.2 Nodes and polylines

- Directed route nodes and connections.
- Branch/reconnect existing nodes.
- Move, unlink, delete, one-way/two-way, and backing modes.
- Minimum semi turn-radius checks.
- Rounded tangent fillets and swept-body clearance.
- Waiting-area markers.
- Bounded node/link counts and strict server metadata validation.
- Graph-aware congestion-cost and right-of-way foundations.

This general planner is not the same as the current commerce truck's fixed starter route. The node tools are planning/coordination foundations, not a fully connected operational fleet scheduler across arbitrary edited properties.

### 13.3 Interior floor lanes and paint

- Forklift lanes join cleanly, removing internal duplicate lines at intersections.
- Personnel routes and crossings are visually distinct.
- Painted markings are shallow/ground-aligned rather than floating debug lines.
- Crossings are derived from actual lane geometry.
- Workers yield at forklift crossings under the supported simulation rules.
- Work zones cannot overlap one another or lanes under the chosen placement policy.
- Filled/hatched zone treatments and wider internal hatch lines improve recognition.
- Overlay adds colors/labels/inventory context without becoming the only way to read the floor.

### 13.4 Pathfinding behavior

- Forklift grid paths can cut suitable corners instead of only following rigid right angles.
- Static equipment and lane geometry constrain routes.
- Standing people affect forklift detours and immediate safety stopping.
- Wide pedestrian lanes support bounded passing tracks with collision/separation checks.
- No teleporting through a traffic conflict to make a test pass.
- Narrow spurs and complex crowds still need more complete behavior.

---

## 14. Machinery and authored product assets

### 14.1 Plasma table

- Requested cutting area: 14 ft × 10 ft.
- Separate table/bed, gantry, carriage, torch, and flexible hose/chain presentation.
- Actual visible gantry/carriage/torch movement and plasma effects.
- Machine computer lets the player select a fixed part recipe and quantity.
- UI shows predetermined steel weight and processing time.
- In the connected factory, motion is driven by real production activity.
- The early standalone demonstration actor/visual test should not be confused with the later inventory-consuming production integration.

### 14.2 Press brake

- Detailed body and moving press components.
- Consistent display styling with the other machines.
- Converts cut blanks/plates into the corresponding formed parts.
- Does not select new raw material for the operation; it consumes the correct upstream item and applies its fixed brake time.
- Preserves the blank's weight.
- Input/output rack integration and an explicit operator assignment.
- $4,500 starter upgrade.

### 14.3 Sheet roller

- Authored in Blender, length increased as requested; current documented roller length is about 352 cm.
- More detailed roller/journal/bearing geometry and a computer stand/cable.
- Rotating rolls and material/process presentation.
- Converts shell blanks into rolled shell sections in the larger prototype.
- Existing operator console is too far away for a normal Handyman reach; this remains a known layout issue.

### 14.4 Welder and effects

- Detailed MIG-style cart/machine, bottle, leads, and gun.
- Gun detail and effect intensity were increased after initial reviews.
- Bright local light and sparks at the tip/work point.
- Standard welder equipment belongs inside every product work-zone preset.
- Idle gun holsters; actual assembly moves the gun/lead to the work point.
- Effects follow real work activity rather than an always-running demonstration.
- Handyman palms are fitted to actual grip/support points.

### 14.5 Forklift, pallets, racking, and sheet models

- Existing asset forklift adapted with materials, moving mast/forks, wheel presentation, seated employee, and collision.
- Pallets/racks given usable materials and stock representation.
- Steel sheets authored at the chosen real dimensions.
- Multiple stored sheets/items are represented with bounded sample meshes.
- Physical stock remains tied to inventory records, not independent decorative duplicates.

### 14.6 Placement areas

Machine definitions include grid-aligned solid and working footprints with operator/material clearance. These are useful definitions/previews, not proof of a finished universal equipment relocation/purchase UI for every machine.

The documented working areas are approximately 6 × 6.5 m for the brake, 7.5 × 8 m for plasma, and 5 × 6.5 m for the roller.

---

## 15. Tanker manufacturing prototype

### 15.1 Why it exists

The first detailed product discussed was a carbon-steel straight-barrel tanker trailer. It was used to establish parts, machine chains, product zones, welding, stock, and logistics requirements. The later commercial starting experience was intentionally reduced to brackets and plates, rather than forcing a new player to build a complete tanker immediately.

### 15.2 Product shape and size

- Barrel diameter approximately 235 cm.
- Tank with heads approximately 1,191 cm long.
- Complete trailer/product footprint approximately 1,330 × 260 cm.
- Largest work-zone preset resized to about 60 ft × 30 ft including its internal supply area.
- Whole dished heads; no partial head pieces.
- ASME-style flanged-and-dished appearance was selected for game art. The game recipe is not an engineering fabrication specification or certification.

### 15.3 Bill of materials

| Final component | Quantity | Upstream work |
| --- | ---: | --- |
| Bent brackets | 24 | Plasma blank: 5 lb / 8 s; brake: 8 s. |
| Frame rails | 12 | Plasma blank: 52 lb / 15 s; brake: 16 s. |
| Rolled shell sections | 48 | Plasma blank: 96 lb / 12 s; roller: 18 s. |
| Whole dished heads | 2 | Purchased complete. |
| Running gear kit | 1 | Purchased. |
| Valves/fittings kit | 1 | Purchased. |

The shell uses eight longitudinal rings with six sectors each. Programmed cut steel for these parts totals 5,352 lb before bought-in components. Current general material carry-forward rules should override obsolete remnant-scrap text in early product notes.

### 15.4 Assembly stages in the catalog

| Stage | Initial balancing time | Main requirements |
| --- | ---: | --- |
| Weld chassis | 60 | 12 rails + 24 bent brackets. |
| Install running gear | 20 | 1 running gear kit. |
| Weld tank sections | 192 | 48 rolled sections. |
| Close tank ends | 40 | 2 whole heads. |
| Fit valves/equipment | 20 | 1 fittings kit. |
| Final inspection | 10 | Previous stages complete. |

These are simulation balancing values, not real manufacturing time estimates.

### 15.5 Work-zone behavior

- Product footprint, worker clearance, standard welder, and part drop areas belong to one zone.
- Part bays are inside the overall boundary, not loose boxes outside it.
- Different parts have different bay dimensions.
- Zone inventory UI identifies all owned supplies.
- Missing parts stop assembly.
- Parts are installed incrementally; solid/hologram visuals show progress.
- Larger tanker zones align with the loading wall and pickup doorway.
- Smaller future product presets can be placed farther inside the shop.
- Two matching tanker bays were authored in the larger test shop, one per major loading door.

### 15.6 Tractor/trailer prototype

Earlier work separated the tanker trailer from its semi/tractor, improved textures and wheel detail, added landing-gear extension/retraction, simplified near-range coupling by auto-snap, and reduced shaky close camera driving.

Product tankers and semis were separated into sensible source/content categories. A tanker is a product to be collected, not merely a permanently attached tractor accessory.

This prototype does not establish a complete commercial tanker-sales/dispatch progression in the small starter. Automatic finished tanker transfer to commercial customer vehicles remains a separate logistics milestone.

---

## 16. Multiplayer, companies, authority, and privacy

### 16.1 Shared and separate businesses

- Default joins use the shared co-op starter company.
- Trusted backend code supports separate company creation and exclusive plots down the street.
- Starter plus three reserved plots exist in the initial ledger geometry; that is not a player cap.
- Owners, members, invitations, acceptance, and removal are represented by the backend.
- Independent company cash and plant identities exist.
- Employees, machines, vehicles, inventory, construction, and commands resolve the appropriate company/plant.

A complete player-facing company/plot setup interface is not yet finished. New independent businesses do not automatically become a fully equipped commercial starter simply because a plot is claimed.

### 16.2 Server command rules

- Connected non-spectator membership is validated.
- Company/plant authority is derived from the actual connection, not trusted client fields.
- Construction revisions, occupancy, bounds, definitions, and ownership are checked.
- Hiring/assignment checks ownership, availability, safety, and duplicate requests.
- Commerce checks actual balance, manifests, counts, capacity, order remainder, and time constraints.
- Failed operations do not partly mutate the state.
- Replay/sequence/rate limits protect relevant commands.

### 16.3 Public versus private state

- Nearby physical workers, equipment, and vehicles can be visible.
- Private orders, bookings, company money, and operation data use company-relevant replication.
- Public physical factory snapshots omit the private commerce payload.
- UI screen data comes from the authorized business.
- Local appearance variants are deterministic from the stable employee key.
- A late join receives the correct own/shared state, not another company's private actor.

### 16.4 Authentication limit

Current anonymous joins use server-issued session identities. A trusted verified-account integration hook exists, but production online authentication and durable reconnect ownership across restarts are not integrated.

### 16.5 Networking scale limit

Tests have exercised four initial players and a fifth late join, including three companies. This verifies specific local flows. It does not establish internet deployment, dedicated-server acceptance, production crowd/fleet throughput, packet-loss behavior, a maximum player count, or FPS.

---

## 17. Persistence and save boundaries

### 17.1 Factory/employee save data

Current factory schema is version 9. Earlier versions added commerce, operator state, names, generic waiting people, and safe role/vehicle bindings in stages.

Saved state includes relevant:

- Programs and machine cycles.
- Material balances and reservations.
- Pallets, stock, booking locks, and in-flight cargo.
- Customer orders, bookings, money, and priorities.
- Employee IDs, names, current job/binding, and walking progress.
- Work-zone/product assembly progress.
- Floor lanes and material routes.
- Construction/property records where composed by the host save flow.

### 17.2 Migration

- Older roles/names migrate in sequence.
- Generic-hire migration preserves employee identity.
- Legacy employee/vehicle ID aliasing is handled.
- Existing custom floor paint is preserved.
- Unsafe waiting homes may be replaced with safe destinations; the worker walks there rather than being teleported.
- Mid-walk save/load resumes actual travel.
- Loading before/after a brake purchase removes/restores its actor, rack, collision, and navigation appropriately.

### 17.3 Atomic restoration

Validation rejects malformed snapshots, invalid identities, bad geometry, and impossible manifests. Company-account changes are coordinated with property restoration and rolled back when restoration fails.

### 17.4 Save slots

The starter uses distinct slots from the larger factory profile. Documented slots include:

- Starter plant: `FoundrylineCutParts01`.
- Larger factory: `FoundrylineFactory01`.
- Starter property profile: `CutPartsProperty01` in the dedicated starter flow.
- Earlier/general property flow: `Foundryline_Property01` / Property01-style host slot.

Exact slot strings should be verified in the current save method before writing migration tooling; older documentation uses names from earlier profile stages. Local files live under `Saved/SaveGames`.

### 17.5 Important unfinished persistence

Combined world saving for multiple independent companies is not complete. Legacy whole-property/single-plant saving is guarded when multiple businesses exist so it cannot erase a neighbour's state.

No cloud save, production online account continuity, or complete town/server-restart persistence is claimed. Automation uses isolated test slots and prior work preserved normal player saves.

---

## 18. Code, assets, tools, and repository structure

### 18.1 Actual project location

The active game lives in:

`C:\Users\curly\Documents\Foundryline Game`

The conversation/chat working directory may still show:

`C:\Users\curly\OneDrive\Documents\ChatGPT\Foundryline Game`

That stale chat location must not be mistaken for the real Unreal project. The user explicitly asked to move game work out of OneDrive. Spellscape is a different project and must remain untouched.

### 18.2 Code layout

`Source/Foundryline/Public/<System>` and `Private/<System>` preserve the planned domain boundaries:

Core, Grid, Construction, Buildings, Zones, Pathing, Employees, Production, Logistics, Inventory, Orders, Economy, Management, Companies, Competition, World, UI, SaveSystem, and Networking.

These are domains inside the runtime module, not nineteen independently implemented engine modules. Some areas remain foundational or reserved. Tests live under `Private/Tests`.

### 18.3 Content and source art

- `Content/Factory`: preserved original Factory assets.
- `Content/Handyman`: preserved original Handyman pack.
- Other vendor roots remain at their native mount paths.
- `Content/Foundryline`: game-owned maps, definitions, materials, wrappers, machines, employee profiles, UI, and derived assets.
- `SourceAssets`: editable Blender, extracted/source packages, assemblies, and art manifests.
- `Tools/Blender`: model generation, export, rigging, measurements, and geometry QA.
- `Tools/Unreal`: imports, setup, read-only reports, native reviews, and multiplayer acceptance launchers.
- `Docs`: design, plans, architecture, acceptance, and this handoff.

### 18.4 Purchased assets used/discussed

| Asset group | Role |
| --- | --- |
| Factory LowPoly | Factory buildings, equipment/props, infrastructure and supporting art. |
| Stylized forest/vegetation | Trees, bushes, grass and environment dressing. |
| Softline UI Sounds | Imported UI sound library. |
| Megapoly.Art truck/trailer content | Tractor/tanker prototypes and later starter flatbed. |
| Handyman Modular low-poly stylized | Current employee character meshes, animations, tools, and modular outfits. |

### 18.5 Public GitHub publication

The user explicitly authorized push and merge to Foundryline main. The GitHub repository was public and empty, so the work was published as an initial `main` commit rather than merging existing branches.

- Commit: `3a886e9`.
- 532 source/config/tool/document files in the initial publication.
- `Content/` and `SourceAssets/` are excluded from the public repository.
- Builds, caches, local saves, logs, screenshots, IDE state, and script caches are ignored.
- Purchased assets and derived art containing licensed source data stay local.
- The public clone alone is not a playable complete project.
- Authorized private asset backups and appropriate licenses are required to restore content.
- `Docs/REPOSITORY_CONTENT.md` explains this boundary.

This handoff is a new local document created after that initial push. It is not automatically committed or pushed by creating it.

---

## 19. Validation and actual playtest evidence

### 19.1 Do not add all test counts together

The suite grew over time. A 15-case focused employee run is not a rerun of every prior construction/economy case. The counts below identify the scope and snapshot of each acceptance stage.

| Milestone | Evidence recorded |
| --- | --- |
| ERP desktop revision | 97 full automated cases; 49 native workflow checks; 14 physical mouse/key checks. |
| Initial company-owned employees | 103 full automated; 48 multiplayer; native functional/animation checks. |
| Free named hiring | 108 full automated; 58 multiplayer; 24 native hiring/workflow. |
| Generic hire → job/equipment | 111 full automated; 62 multiplayer; starter and actual welder native checks. |
| Collision/trunks | 114 full automated before the final base/door fixes; 6 final relevant targeted cases. Not a claim of a complete later 116-case rerun. |
| Collision movement/network | 72 four-peer input/collision checks; 67 five-player business/pallet-step checks. |
| Collision native walk | 23 rendered walkthrough checks, plus final ERP E-open/close. |
| Starter bracket/plate focus | 12 commerce cases; final two-case targeted smoke; ordinary-input two-order playthrough. |
| Final Handyman integration | 15 focused employee cases; 66 five-player/late-join appearance/business checks; actual paid starter order; 9 welder function and 8 visual checks. |

### 19.2 Actual ordinary-input repeat sale

The two-order bracket/plate run used normal mouse/keyboard UI, actual hired/assigned starter staff, paid sheets, actual forklift moves, selected pallet bookings, the shipped 8x control, and customer deliveries.

- FL-0001: 40 brackets, $440 paid, Day 2 at 12:06.
- FL-0002: 60 mounting plates, $300 paid, Day 3 at 12:09.
- Ending cash $4,111.55.
- Approximately 190.5 lb steel remained.
- Eight offers remained available.
- No inventory grants, money grants, manual completion flags, simulation-advance test helper, or user-save writes were used for that playthrough.

### 19.3 Final Handyman starter proof

With the real characters installed, a rendered 40-bracket order was manufactured, staged, loaded, delivered, and paid $440. Recorded remaining cash was $4,112.08 in that separate one-sheet scenario.

### 19.4 Welding evidence boundary

The working Handyman welder was verified in a transient stocked larger-workshop fixture. That fixture uses existing test advancement and authored stock. It proves actual zone work, movement, grip alignment, and effects; it is not a claim that the commercial small starter begins with tanker parts or a complete tanker business.

### 19.5 Multiplayer checks

Specific accepted checks include:

- Four initial clients and a fifth late join.
- Three businesses with separate company keys and private cash/operations.
- Shared co-op employee assignment.
- One winner for competing machine claims.
- Foreign employee/plant/commerce commands denied.
- Stable names, jobs, and Handyman outfit variants across peers.
- No duplicate employee bodies.
- Duplicate accept/book/delivery charges prevented.
- Character collision agrees with server movement.
- Low-pallet movement-base warnings reproduced, corrected, and absent in final rerun.

### 19.6 Acceptance limits

No current result establishes:

- A shipped packaged build.
- Public internet matchmaking/authentication.
- Dedicated-server production readiness.
- A production maximum player/crowd/fleet count.
- A verified 60 FPS result.
- Complete town progression or multi-business persistence.

### 19.7 Useful local images

All are under `Saved/Screenshots/Reviews` and excluded from public Git history:

- `StarterSales_TwoOrdersPaid.jpg`.
- `ERPDesktopV2_FinalOverview.jpg`.
- `FirstOrder_V2_BuyerQuotation00000.png`.
- `Workstation_FinalDesk00000.png`.
- `EmployeeJobs_AssignPlasmaJob00000.png`.
- `WorldQA_ShopInterior00000.png`.
- `FirstOrder_LoadedPickupTruck00002.png` and later review variants.
- `HandymanStarter_OperatingPlasma00000.png`.
- `HandymanStarter_ForkliftDriver00000.png`.
- `EmployeeJobs_WelderAtProduct00001.png`.
- `TreeTrunk_CollisionAlignment00000.png` — includes temporary debug geometry, not a normal gameplay overlay.

Older screenshots sometimes captured an old camera instead of a requested modal or had temporary/obsolete employee models. Prefer the exact final accepted files referenced by the relevant acceptance document.

---

## 20. Conversation history and decisions that changed

This is the detailed chronological decision summary. It records the intended direction and distinguishes it from finished implementation.

### Phase 1 — Remote access and the plan

- The initial discussion was about accessing this thread/project through Codex on a phone and desktop, and how local Work/project connections relate.
- The user supplied `Foundryline_Master_Plan_v1.1.pdf` and asked it to be read.
- The plan's revision emphasized true scale, decorations, modular placement, collision/pathfinding differences, optional furnishing, asset checks, rendering detail reduction, and Unreal placement/save architecture.
- Document content was context, not automatically an instruction to execute everything in it.

### Phase 2 — World and engine foundation

- Start with Factory LowPoly from the Fab library.
- Flat grass map, trees/bushes, world grid, Lumen daylight.
- C++ foundations and 1:1 scale.
- Target 60+ FPS, a town-sized multiplayer/co-op world, broad zoom, grid snapping, hologram previews.
- Cars, semis, and logistics/pathfinding must be considered from the beginning.
- Multiplayer became an explicit permanent requirement, later reinforced as supporting more than two players.

### Phase 3 — Project location and safe separation

- Connect Foundryline to GitHub.
- Create the Unreal project without interfering with another game, Spellscape.
- Move the active game out of OneDrive into the user's local Documents directory.
- Help locate/register the project for Fab additions.
- Early restrictions on launching/building while another Unreal task ran were later explicitly lifted for Foundryline.
- Continue to keep projects, source, settings, and processes separated.

### Phase 4 — Asset import and vegetation

- Import Factory assets and UI sounds.
- Add the stylized forest.
- Replace old foliage.
- Keep the terrain flat but make nearby grass rich and three-dimensional, with distant detail reduction.
- Confirm grid and startup environment before expanding construction.

### Phase 5 — Construction quality

- Initial thin/paper-like walls were unacceptable.
- Roads needed markings and useful options.
- Buildings should be click-drag rectangles with a full blue hologram, concrete floor and real walls.
- Doors/garage openings follow building placement.
- Floors, roads, and pavement should have compatible height and thickness.
- Expansion should snap to an existing building and remove shared walls.
- Sidewalks, curbs, parking, planting and exterior infrastructure belong in the foundation.

### Phase 6 — UI and viewing improvements

- Match the logo more closely.
- Change construction controls to a sidebar.
- Make all placeable items holograms.
- Make grids, pavement sizing, zones, and route tools easier to understand.
- Reduce noise and improve hover/selection feedback.
- Phone screenshots must be actual inline images, not inaccessible references alone.

### Phase 7 — Shop tiers and building detail

- Manufacturing needs taller space than simple offices or light work.
- Keep Tier 1 and introduce a taller Tier 2.
- Put Tier 2 windows high on the walls.
- Add correctly scaled columns, trim, caps, concrete bases, and suitable garage doors.
- Make windows/columns auto-space gracefully across arbitrary wall sizes.
- Fix height-transition gaps and wrongly scaled columns.
- Switch/refine building colors; eventually dark roofs/trim.

### Phase 8 — Offices and interior structure

- Add office areas, click-drag rooms, doors/windows, and realistic desk/decor clearance.
- First require outside-connected office annexes with outside/shop access.
- Avoid office boxes/double walls inside another building.
- Make office windows manual.
- Later explicitly allow separate offices anywhere permitted, then fix attaching shops to offices.
- Separate interior/exterior wall and secondary trim colors.

### Phase 9 — First person, roofs, skylights and lighting

- Add a first-person walkaround button and E-operated personnel doors.
- Roofs exist, hidden in top-down by default, always visible in first person.
- Add roof visibility control and skylights.
- Improve daylight and how light works through windows.
- Add automatically spaced hanging shop and office/hall lights.
- Interior lighting matters because gameplay takes place inside the rooms.

### Phase 10 — Starter lot and loading direction

- Build a practical starting property with shop, office, driveway, parking, and loading provision.
- Initially discuss dock/ramp style, then reject the ramp.
- Keep road and shop level compatible instead of a deep/raised dock arrangement.
- Add parking flowers and small trees.
- Check transitions and snapping.

### Phase 11 — Multiplayer property and editing

- Test co-op and simultaneous first-person/top-down views.
- Validate shared construction and property ownership.
- Handle stale/conflicting changes without noisy success banners.
- The temporary hologram reservation system was removed; final server validation is the conflict authority.
- Think about many moving objects, multiple businesses and more than two players.

### Phase 12 — Driving and logistics planning

- Roads, parking, driveways, loading areas must support believable vehicle movement.
- Use hidden operational routes with optional logistics overlay and editing UI.
- Show parking and useful loading information.
- Move toward node/polyline routes with minimum semi turn radii.
- Allow route connections, branches, waiting/congestion considerations.
- Improve confusing min/max tools, grid alignment, and route/node UI.
- Keep construction customizable for small trucks through semis depending on the product.

### Phase 13 — Vehicle and tanker source work

- Find product models and truck packs.
- Separate tanker from tractor.
- Animate wheel rotation and landing gear.
- Simplify coupling: get close and auto-snap instead of precise difficult alignment.
- Correct tractor textures and close-up driving shake.
- Improve trailer/wheel details.
- Separate product tankers from semi/vehicle folders.

### Phase 14 — Manufacturing concept and work zones

- Use work zones with minimum product sizes.
- Assign welders/equipment to zones.
- Show a product hologram becoming solid as assembly progresses.
- Tanker is complex but useful to establish the chain.
- Initially keep operation possible top-down.
- Later first-person becomes the primary view; the top-down capability remains through the office.

### Phase 15 — Parts, purchased items and machines

- Steel arrives in sheets on pallets/racks; choose one sheet format, not many.
- Fixed pounds and times per part, rather than freeform player-authored specifications.
- Buy valves/fittings and complete dished heads initially.
- Add plasma, press brake and roller.
- Model sheets, roller, welder/gun, press, and plasma in Blender.
- Improve screens, machinery detail, sparks/light, cables, and actual mechanical motion.

### Phase 16 — Automatic internal material handling

- One part type per pallet, representative visual quantities.
- Linked storage racks per machine.
- Forklift pickups/deliveries should be automatic once routes/zones are set.
- Work-zone supply area shows all items assigned to the product.
- Welder consumes parts over time and stops if they are missing.
- Driver priorities and several connected zones create traffic without manual-click tedium.
- Forklift/personnel lanes, paint, overlays, and crossings become part of readable factory planning.

### Phase 17 — Work-zone resizing and organization

- Preset work-zone sizes simplify setup.
- Tanker is the largest preset; it was later reduced to fit better.
- Keep all part bays inside the zone and use actual part-sized footprints.
- Place the tanker zone at its pickup/loading door.
- Add a standard welder to every product zone.
- Make two matching bays and suitable doors in the larger test shop.
- Improve connected lane paint, overlap rules, stripe width, and grid accuracy.

### Phase 18 — Smaller starting business

- Reset the initial commercial experience to a small shop, two offices, plasma, racking, forklift, and palletized cut-part sales.
- Keep the large workshop in another map/profile.
- Design demand, order lists, material costs, processing times, part sale values, and first automation progression.
- No nesting; plasma consumes pounds until replenished, with recipes changeable mid-sheet.

### Phase 19 — Contract and shipping discussion, one decision at a time

- Public order website plus emails.
- Reputation/traits later.
- Flexible, deadline, and rush work; timer begins on acceptance.
- Upfront, per-load delivery, or completed-order payment.
- Partial deliveries allowed.
- Machine chooses part and quantity.
- Linked storage instead of manually handling every output pallet.
- Pickup zone assigned to a job.
- Tablet lists actual pallets, supports selection, and schedules the truck.
- Even a pallet with one bracket is shippable.
- Carrier capacities and flat fees matter.
- Named carriers.
- Shared single receiving/shipping bay; overlapping schedules queue.
- Rebooking fee is half the original booking cost.
- Incoming steel goes directly to raw racks.

### Phase 20 — Overnight implementation and real starter truck

- User approved building the whole agreed first-order loop and modelling needed assets.
- New trucks were downloaded and located in a Fab/VaultCache path on the desktop rather than the initially guessed Downloads path.
- Convert the actual purchased starter flatbed, correct units/materials, animate wheels/steering/gate, and fit cargo.
- The user later explicitly requested a Blender rig like the earlier vehicle work; that editable rig was provided.

### Phase 21 — Office-only Property View

- The user decided top-down should only be available at the office computer.
- First person becomes the starting/main view.
- Property View handles expansion, doors, square footage, and construction.
- Each player's camera remains independent.
- UI must be clean, light, orange/grey, and practical from first person.

### Phase 22 — ERP identity and software quality

- Initial office UI was called boring/plain.
- Make the pop-up much larger.
- Give businesses logos.
- Make it feel like real computer software and then explicitly a real ERP.
- Add real modules, a taskbar/app frame, dense sortable tables, documents, mail, activity, charts and useful next steps.
- Multiple visual/native checks caught missing fonts, hidden hitboxes, wrong screen orientation, dark materials and wrong-camera captures; these were corrected instead of treating code compilation as visual acceptance.

### Phase 23 — Employees before final models

- Set up walking, idle/work states, machine operators, welders, drivers, and replaceable visual models.
- Every business owns its staff; co-op shares the business roster.
- Worker attendance gates actual production.
- Fix traffic conflicts rather than teleporting people or ignoring collision.

### Phase 24 — Free hiring, random names, then jobs

- Add ERP hiring per business.
- Employees are free for testing; payroll later.
- Generate random first/last names and save them.
- Initially role-based hiring, then explicit clarification: **hire first, give a job afterward**.
- Generic named hires now receive specific machine/forklift/zone responsibility afterward.
- Cross-role changes retain identity.
- Hiring a driver must not create free equipment.

### Phase 25 — Improve the office computer

- The phrase improve CPU was clarified as the computer appearance/UI, not CPU performance.
- User requested three monitors.
- Detailed workstation and three distinct live ERP displays were implemented.

### Phase 26 — Full physical collision pass

- User reported walking through many objects.
- Add meaningful collision to machines, accessories, stock, forklifts, furniture, carriers, and nearby trunks.
- Check real player movement on server/client.
- Fix pooled ghost collision, low-pallet network basing, and outward room doors blocking the corridor.
- Keep grass/bushes/canopies passable.

### Phase 27 — Flow audit and scope correction

- Check whether the game can keep going and identify missing links.
- Audit found expansion is not usable production-floor growth, unsupported orders were offered too early, speed controls were hidden, recovery is weak, and long progression is unfinished.
- User narrowed the immediate goal to playable bracket/plate sales.
- Add capability-gated orders, visible ERP speed controls, program-order prefill, and accurate next-step guidance.
- Prove two paid orders through actual player UI.

### Phase 28 — Handyman import and integration

- User purchased Handyman and asked to add it.
- Native pack imported and inventoried.
- User then explicitly authorized model changes and implementation.
- Replace temporary visible workers with fitted Handyman bodies/outfits/animations.
- Preserve all names, jobs, company authority and saves.
- Fix native-pose mapping and runtime merge persistence issues.
- Validate real walking, plasma controls, forklift seating, welding grip, paid production and multiplayer appearances.

### Phase 29 — GitHub publication and handoff

- User explicitly requested push and merge to GitHub main.
- Remote was public and empty.
- Publish source/config/docs/tools as the initial main commit.
- Exclude licensed assets and local art from public distribution.
- User requested this highly detailed Markdown handoff and conversation summary.

### Working preferences reinforced through the conversation

- When the user says stop/halt, stop immediately.
- Continue the existing task when asked, rather than starting over.
- Do not interfere with Spellscape.
- The user authorized agents and later asked for fewer waits/handoffs and faster completion.
- Keep checks focused on changed behavior; do not rerun unrelated broad suites without a reason.
- Use real native screenshots when presenting visuals, especially for phone viewing.
- Do not describe a plan or compiled source as a finished tested feature.
- Do not merge or push without explicit authorization; the later GitHub publication had it.

---

## 21. Known gaps, limits, and risks

### 21.1 Gameplay/progression

1. Building expansion is not yet expanded production/navigation capacity.
2. Full progression beyond the initial brake/second forklift is incomplete.
3. No contract abandonment/cancellation workflow.
4. No emergency loan, bankruptcy recovery, or equipment resale.
5. Working-capital planning is still important; bad decisions can leave the player stuck.
6. Deadline status exists, but reputation penalties/customer traits do not yet complete that system.
7. Research, pallet-capacity upgrades, materials beyond the starter steel format, and broad product diversity remain future work.
8. The commercial tanker progression is not connected end-to-end to the small starter.

### 21.2 Equipment/character fit

1. Roller console needs physical relocation/reach correction.
2. More specialist PPE and bespoke work gestures are needed.
3. Forklift climbing/boarding is immediate.
4. A fully general equipment purchase/move/replace UI is not complete for all machines.

### 21.3 Vehicles and routing

1. Current starter commercial carrier follows fixed authored paths.
2. Property edits do not regenerate that route automatically.
3. Other carrier types have valid capacities/services but temporary visuals.
4. Kinematic vehicles do not fully yield/sweep against human players.
5. General route-node planning is not a complete arbitrary-layout live fleet.
6. Large crowd/fleet behavior and congestion throughput remain unmeasured.

### 21.4 Multiplayer/persistence

1. No complete player-facing separate-business creation/start workflow.
2. Combined independent-business world save is unfinished.
3. Online authentication and durable reconnect ownership are not integrated.
4. Packaged internet multiplayer and production dedicated-server behavior remain unaccepted.

### 21.5 Build/content/backups

1. GitHub is source-only; it is not a backup of the full playable content.
2. A separate authorized private content/source-art backup is essential.
3. Several old docs describe superseded startup stock, temporary models, remnant scrap, or unfinished states. This summary points out the major corrections but does not rewrite every historical document.
4. Some tooling contains local project paths; a clean-machine setup needs path/configuration work.
5. Isolated editor/Persona/D3D startup failures occurred during development and were recovered. They should not be confused with a consistently accepted packaged startup.

### 21.6 Performance

Lumen and rich environments are configured and various pools/relevance/culling bounds exist. **There is no verified 60 FPS measurement or 10% performance-improvement claim for this project.** Profile a representative playable workload before promising performance or player/crowd limits.

---

## 22. Recommended next work and acceptance criteria

These are recommendations from the current handoff, not new completed features or authorization to implement everything at once.

### Priority A — Protect the starter bracket/plate experience

- Conduct short fresh-session playtests with no developer assistance.
- Make it obvious which order to accept, how much steel to buy, which employee/equipment is missing, and when goods are ready to ship.
- Keep accepted work, incoming stock, in-process parts, finished stock, booked freight, and payment state distinguishable.
- Prevent duplicate overproduction and unnecessary material buying through accurate guidance.
- Add a clearly explained recovery/cancellation policy before broadening financial difficulty.

Acceptance: a new tester completes two different paid orders, keeps sufficient stock/cash to continue, and can explain the next useful action without a developer opening hidden tools.

### Priority B — Finish workstation and equipment ergonomics

- Correct roller console reach while updating its collision, screen anchor, and operator station together.
- Check brake operator stance with the final character setup.
- Add appropriate specialist PPE and safe tool resting poses.

Acceptance: actual hands reach controls without stretched arms or moving simulation positions artificially; player and employee paths remain clear.

### Priority C — Functional shop growth

- Connect constructed/expanded floor area to factory usable bounds, machinery/storage placement, and navigation.
- Preserve existing inventory/jobs while rebuilding changed navigation locally.
- Make expansion cost/capacity benefits explicit.

Acceptance: expand the shop, place useful storage/equipment in the new area, assign staff, and complete production/transport there in co-op.

### Priority D — Multi-business persistence and onboarding

- Complete server/world save composition across companies/plants/property.
- Add player-facing create/join/invite/plot flow.
- Integrate trusted online identity before claiming durable ownership.

Acceptance: two independent businesses keep separate money/employees/stock through save/restart/rejoin without one reverting the other's state.

### Priority E — Broader logistics and progression

- Connect operational carriers to edited route networks.
- Add proper van/rigid/semi presentation.
- Add sensible player yielding and blockage recovery.
- Broaden products and demand only after the basic flow is understandable.
- Return to tanker commercial requirements when the smaller part business is stable.

### Priority F — Packaging and measured performance

- Produce a reproducible authorized-content setup/build procedure.
- Run packaged standalone and multiplayer acceptance.
- Profile first-person interiors, grass, multiple workers/forklifts, and trucks under active production.
- Tune based on measured bottlenecks while retaining the chosen quality/scale.

---

## 23. Detailed handoff checklist and source references

### 23.1 Before editing

- Confirm the actual project path in local Documents.
- Read `AGENTS.md`, `Docs/ARCHITECTURE.md`, and startup requirements.
- Inspect Git branch/status; do not assume another checkout or Spellscape is relevant.
- Identify the current editor/PIE state before importing or building.
- Keep native vendor paths intact.
- Coordinate one engine-writing/build/import operation at a time.
- Do not write normal player saves for tests.
- Distinguish public source changes from licensed local content changes.

### 23.2 After gameplay changes

- Compile the relevant target.
- Run meaningful focused tests for changed behavior.
- Exercise the actual player-facing path, not only helper calls.
- Check company ownership, replay/conflicts, and late join when authoritative state changes.
- Inspect real images for material, scale, pose, and camera failures.
- Check doors, path access, and physical collision after model/layout changes.
- Restore intended player/editor settings and leave a useful fresh session.
- State what was not tested.

### 23.3 High-value code locations

| Area | Main source locations |
| --- | --- |
| Factory state/catalog | `Public/Production/FoundrylineFactoryTypes.h`, `Private/Production/FoundrylineFactoryCatalog.cpp` |
| Deterministic simulation | `Public/Production/FoundrylineFactorySimulation.h`, corresponding Production implementations |
| Commerce commands/tick/catalog | `Private/Orders/FoundrylineCommerceActions.cpp`, `FoundrylineCommerceSimulation.cpp`, `FoundrylineCommerceTypes.cpp` |
| Company ledger/registry | `Public/Companies`, `Private/Companies` |
| Employee behavior/presentation | `Public/Employees`, `Private/Employees` |
| Office/tablet UI | `Private/UI/FoundrylineOperationsController.cpp`, `FoundrylineOperationsHUD.cpp` and related UI files |
| Factory actor/save integration | `Private/Production/FoundrylineFactoryPlant.cpp`, `FoundrylineFactoryCommerce.cpp` |
| Starter carrier | `Public/Logistics/FoundrylineCarrierSceneActor.h`, corresponding implementation |
| Player collision/movement | World character and game-owned physical collision helpers |
| Construction/roads | `Private/Construction`, `Private/Pathing`, BuildSite and layout helpers |
| Automated cases | `Source/Foundryline/Private/Tests` |

### 23.4 Primary documentation references

- [Repository content/publication boundary](REPOSITORY_CONTENT.md)
- [Architecture](ARCHITECTURE.md)
- [Starter bracket and plate playtest](Architecture/STARTER_BRACKETS_AND_PLATES.md)
- [First-order simulation](Architecture/FIRST_ORDER_LOOP.md)
- [Play-flow audit](Architecture/PLAY_FLOW_AUDIT.md)
- [Employee job contract](Architecture/EMPLOYEE_JOBS.md)
- [Employee job acceptance](Architecture/EMPLOYEE_JOBS_ACCEPTANCE.md)
- [Employee hiring history](Architecture/EMPLOYEE_HIRING.md)
- [Handyman integration](Architecture/HANDYMAN_EMPLOYEES.md)
- [First-person operations](Architecture/FIRST_PERSON_OPERATIONS.md)
- [ERP desktop revision](Architecture/ERP_DESKTOP_REVISION_ACCEPTANCE.md)
- [ERP data semantics](Architecture/ERP_PRESENTATION.md)
- [Three-monitor workstation](Architecture/OFFICE_WORKSTATION.md)
- [Physical collision](Architecture/PLAYER_COLLISION.md)
- [Collision compatibility](Architecture/PLAYER_COLLISION_COMPATIBILITY.md)
- [Starter truck](Architecture/STARTER_FREIGHT_PRESENTATION.md)
- [Business backend](Architecture/BUSINESS_FOUNDATION.md)
- [Construction/property editing](Architecture/PROPERTY_EDITING.md)
- [General driving/logistics planner](Architecture/DRIVING_AND_LOGISTICS.md)
- [Equipment art foundations — historical milestone](Architecture/EQUIPMENT_VISUALS.md)
- [Tanker requirements — historical prototype with some superseded details](Design/TANKER_PRODUCTION_REQUIREMENTS.md)

### 23.5 Current handoff in one paragraph

Foundryline is a locally playable Unreal 5.8 manufacturing/business prototype whose strongest connected experience is a first-person small shop selling plasma-cut brackets and plates. The player uses a branded ERP on a detailed three-monitor computer, hires free named employees, assigns specific equipment jobs, buys weighted sheet steel, relies on automatic forklift/rack handling, books pallet freight, and receives customer payments. Construction, taller shops/offices, route planning, tanker assembly, employee animation, collision, and independent-company authority foundations exist, with extensive local acceptance. The next work should strengthen the simple commercial loop and recovery/expansion boundaries rather than treating the larger tycoon progression, online persistence, or performance target as already finished.
