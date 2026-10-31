# Playtest backlog implementation

User resumed implementation with “start working.” The earlier notes-only pause is lifted for this backlog. Use the existing detailed handoff for decisions and screenshot evidence. No new push/merge requested.

- [x] Plasma workspace: four-slot in-zone output rack, direct pallet accumulation and full-rack blocking.
- [x] Multiple individually placed/rotated plasma, brake, roller and forklift purchases; persistent independent identity, staff and logistics.
- [x] Planner selection/move/resize/remove/rotation, including forklift lanes and rotation during moves; cancel/undo and co-op validation.
- [x] Loading-bay transfer: actionable order assignment and real forklift dispatch.
- [x] Separate Sales from Dispatch; update tablet/inspector/machine/HUD visuals to shared UI standard.
- [x] Plasma-only machine-local programming with active job loading and a multi-batch queue.
- [x] E employee inspection and reassignment, sharing existing authoritative roster rules.
- [x] Remove overlapping forklift occupant visuals.
- [x] Fold starter-truck side rails and align all carrier tyres to ground/paving.
- [x] Sign footprint and curb preservation.
- [x] Standard rack quality and personnel-door fit/model/animation quality.
- [x] Build, appropriate automated regression, native four-player input/flow checks, rendered inspection and leave starter open.

Preserve the uncommitted starter/catalogue/pallet/light-kit work. A file-level baseline backup is in Saved/Backups/BeforePlaytestBacklog. All gameplay changes must account for multiple companies, more than two players, exact real-world scale and bounded simulation/presentation cost.

## Per-driver task checkboxes and priorities

- Independent Receiving, Production and Shipping categories. Choose priority 1, 2 or 3; clicking its checked priority disables that category. Multiple categories and equal priorities are allowed.
- One driver can handle all three, choosing the highest priority available, reachable task after completing the current trip.
- Receiving covers arrivals; Production covers machine supply and output storage; Shipping covers staging and carrier loading, including parts sold directly.
- Classification follows the transport destination/task rather than the item type.
- Changes use the authoritative business command and replicated snapshot, without requiring current work of that category.
- Saved values are -1 disabled and 0..2 displayed as 1..3. Existing settings remain enabled in their previous order.
- Source changes and focused automation added; build/runtime/visual acceptance recorded separately.

Priority update validation: FoundrylineEditor compiled successfully. Commerce automation passed 17/17, including DriverTaskCheckboxes (independent ranks, disabled tasks, all enabled, invalid settings and destination-based category mapping). Rendered priorities were inspected; four-player checkbox changes replicated correctly in native acceptance. Automatic staging was corrected to let machine-output pallets accumulate before occupying the two shipping positions; partial manual shipments remain supported.

## Integrated implementation - October 30 playtest pass

- Plasma output now accumulates directly in four machine-owned rack positions: two floor pallets and two on the upper shelf. The painted machine workspace includes supply/output storage. A full output rack blocks the next cycle before consuming steel. Saved original plasma layouts migrate their idle output stock without dropping inventory.
- Production catalogue includes separately priced/placed plasma, press brake, roller and forklift instances. Each machine retains its own ID, rotation, available recipes, operator assignment, output positions and program queue. Machine-owned racks move with the workspace; independent removal is blocked. Purchased standard racks remain individually editable.
- Existing lanes can be selected, moved, quarter-turned, resized or removed. R rotates during moves; cancellation leaves the model untouched. Connected lane edits are allowed while paused with active jobs; disconnecting a reserved load or a return route rejects the candidate. Z/Y provide guarded server-side planner undo/redo while the shared model is unchanged.
- Pallet inspection offers matching accepted contracts and schedules a real forklift job. A contract may use a partial pallet; staging reserves only its outstanding quantity. Missing Shipping-enabled staff, occupied bay, changed/reserved stock and missing route produce actionable errors. Booking still locks the selected exact quantities and carrier limits.
- Automatic staging waits for pallet capacity, the contract remainder, or batch end. One-item and partial-contract deliveries remain covered by regression tests.
- Office programming links have been removed. Plasma, brake and roller share a large Foundryline machine UI with recipes, quantities, live state, active accepted jobs, FIFO queued work and removal of pending batches. Program/queue commands require the player to stand at the machine terminal on the server. The current reduced part catalogue intentionally has no roller recipe; buying a roller does not re-enable retired products.
- E targets an owned visible employee and opens their name/status/job controls; assignment changes use the existing authoritative roster actions. The hidden animation sampler is kept hidden when forklift visibility changes, removing the duplicate body/head.
- All planner/route controls, dispatch, inspector and machine panels now use the shared orange/charcoal/offwhite surfaces with hover feedback. Sales offers stay on the office desktop; Dispatch shows accepted work, carrier bookings and stock.
- Imported original plasma rail supports, fitted 36x84 in personnel doors with jambs/hardware, rack hardware for 1/2/3 heights, and both starter-flatbed folding side panels. Carrier wheel contact is grounded using cached actual mesh bounds and bounded static-world sampling.
- Sign footprint is now400x75cm on the25cm snap, rather than a400cm square. Signs no longer suppress adjacent pavement curbs.
- The other chat's isolated main menu, supplied artwork/logo/font and starter-shop travel code are present in the shared checkout. Normal GameDefaultMap is L_MainMenu, EditorStartupMap remains L_FirstShopStarter. Continue/Load are disabled until complete restoration is wired. Multiplayer opens a listen server; session discovery/invites are outside this change.

Scoped imports completed without errors; rack model 25/25 geometry checks and starter gate 6/6 hinge checks passed. Latest build, gameplay and visual evidence is recorded below. No push/merge was requested.


## Final planner corrections and current validation

- Relocating an existing machine into an owned addition uses the same covered-floor validation as buying one. The server commits only a valid candidate. Grass gaps and another business's floor remain invalid.
- Move/resize follows the selected 6/12/24 in grid, with Shift for 1 in precision, instead of an unrelated 10 cm increment.
- One-way forklift lane flow follows all quarter-turn rotations. Tests cover sixteen axis/direction/rotation combinations. Resize cannot narrow a lane below 300 cm; invalid changes are atomic.
- Latest full build and automation after those fixes: 171/171 passed (11.8 s build, 54.59 s checks).
- Native four-player acceptance: 24/24 passed. A client queued the first 40-bracket contract at the physical plasma terminal. Real steel delivery, production, forklift staging of the 40/50-capacity pallet, booking and delivery completed. Delivered 40/40; customer payment $720. All four peer ledgers validated.
- The first native run sampled before production finished. The runner now waits for the requested stock, bounded to twelve additional simulation advances. No inventory, cash or recipes are overridden. Intermediate/final reports: Saved/Logs/PlaytestBacklogNativeQA.json.
- Rendered machine control, driver priorities, Dispatch and pallet inspection have been inspected. Screenshots wait a separate frame interval before changing screens.
- Native employee/machine review: 6/6 passed. Both standing operator and seated forklift driver focus and open their own job information through E. Physical press brake and roller terminals open the shared machine screen. Reports: Saved/Logs/EmployeeMachineNativeQA.json.
- Rendered review caught a misleading vacancy label on an employee's current workstation. It now says Currently assigned to their name. Brake/roller processing also no longer shows a misleading zero-pound steel cost. These two presentation-only adjustments compiled successfully in 9.97 s after gameplay checks.
- Main menu native click-through passed: launch L_MainMenu, click New Game, arrive in the first-person starter office with $4,000 / Day 1 and E to use the office computer. Menu and first-person evidence: Saved/Screenshots/Reviews/Playtest_MainMenu_Acceptance.png and Playtest_NewGame_FirstPerson_Acceptance.png. The game remains open for playtesting.
- Editor review preferences restored to their original Standalone / one client / RunUnderOneProcess=True values. No other project/global settings changed.
- Existing menu Continue/Load remain disabled; session discovery/invites and additional roller recipes are outside this pass. No production FPS measurement is claimed.
- VaultCache desktop-folder lock was released by closing the Epic Games Launcher that held its listing database. Cached files were preserved.
- No commit, push or merge performed.


## Follow-up playtest corrections - late October 30

Current implementation is awaiting the combined build/native checks below; earlier pass results above do not validate these additions.

- Plasma: external raw-steel storage remains separate. The cell now contains matching modular input/output racks on opposite sides. Two sheet positions are stocked by real forklift Rack-to-Rack jobs. The table automatically pulls unreserved sheets from its own input rack, using the fixed loading time and material weight. No extra rack-to-table forklift trip; no borrowing another machine's buffer.
- Placement: remove stale starter-shop bounds from fabrication/work-zone preview and authority, require actual owned building-floor coverage, protect physical equipment/lanes/loading access, and outline the full moved machine workspace. Stored-machine placement displays the full machine/rack hologram with IN/OUT arrows and labels.
- Forklift areas: opposite-corner rectangular painting, minimum 360 cm per axis (two 180 cm forklift widths). Touching/overlapping areas share one traffic network and perimeter; soft floor fill replaces road rows/centre lines. Existing legacy lane data remains readable.
- Equipment: buy plasma, brake, roller or forklift at the office, then place owned unplaced equipment in Planning. Purchase charges once; failed placement keeps ownership. Stored machines cannot be programmed or assigned operators. Forklifts use a paid unplaced count until physically placed.
- Catalogue: model-based transparent isometric images, larger cards and selected-item preview, clearer pricing and Foundryline colours. First-person header separates brand, funds, clock and workshop state.
- Inventory and Trucks are separate office pages. Movement buttons require physical matching stock; accepted contracts alone create no cargo. Purchases and other discrete actions get server-confirmed visible receipts rather than optimistic messages.
- Shipping: remove the obsolete starter outbound floor box when preset loading doors exist. Preparation positions follow the owned loading door; committed cargo freezes its current coordinates. Existing stock stays on pallets in storage/output until selected for shipment.
- Employees: occasional friendly shop-pun names mixed with the normal pool; stable company-scoped identities and existing saved names remain intact.
- Existing SaveGames were backed up to Saved/Backups/BeforeInputRackAndAreaUpdate-20261030-230515 before validation. Validation worlds do not overwrite authored maps or player saves.


## Final follow-up acceptance

- Full FoundrylineEditor build succeeded; **187/187 automation checks passed**, including actual starter-floor docking, paid steel through the input rack into production, connected forklift rectangles, placement in owned additions, stored-equipment ownership, confirmations, save/restore and staff identity.
- Native four-player review: **27/27 passed**. Client office purchase charged once without spawning an unplaced forklift. All peers saw the purchased state/cash, driver priorities and machine queue. An unmade contract could not move cargo. Real input replenishment, production, loading-bay staging, carrier booking, delivery and payment passed with valid peer ledgers.
- Native catalogue/owned-equipment review: **16/16 passed**. Four catalogue images imported. A paid forklift placed without a second charge; stored plasma restored with full machine/rack holograms and solid IN/OUT arrows. Revised office equipment cards, receipts, catalogue and first-person instrument header were rendered and inspected.
- Stations can now be placed on physically valid owned floor before painting transport connections. The preview gives a connection reminder. Actual forklift jobs still require a reachable route; no collision clearance was weakened.
- The known starter raw-rack dock was moved out far enough for physical forklift clearance, and a legal west forklift area connects the opposite-side plasma intake to the cross aisle. Legacy full-workshop rack coordinates remain unchanged. Known old starter layouts receive a scoped intake/access migration; custom cell layouts retain player-defined access.

## Measured starter-business playthrough

Fresh unsaved first-shop business, real $4,000 starting cash, one plasma operator and one forklift driver. No cash, stock or edited contracts were granted. All cuts consumed purchased steel; time advancement used the real plant simulation and account synchronization.

Completed actual customer orders: **40 brackets, 60 plates, then another 60 plates**. The third order reused existing steel instead of buying another sheet. **98 playthrough checks passed**, including unaffordable purchase rejection, missing-material recovery, blocking shipment of unmade parts, empty/duplicate booking rejection without a second charge, completed deliveries/payment, material conservation and company-cash reconciliation.

| Cash ledger item | Measured amount |
| --- | ---: |
| Starting cash | $4,000.00 |
| Customer revenue | $1,680.00 |
| Two steel sheets purchased | $510.48 |
| Inbound + outbound freight | $125.00 |
| Employee wages | $843.65 |
| Machine + forklift operating costs | $16.72 |
| Total cash costs | $1,495.85 |
| Ending cash | $4,184.15 |
| Cash increase | $184.15 |
| Remaining usable steel on plasma | 70.48 lb |
| Remaining steel at its purchase rate | $70.48 |

Cash margin was about 11% of revenue over this short run. Including remaining raw steel at cost gives about $254.63 added cash/inventory value; this is not a full accounting profit calculation (no depreciation or later-game overhead is modeled here).

The first two orders temporarily reduced cash because whole-sheet purchases left usable steel. Reusing that steel on the third contract recovered the working capital and increased cash. Current sale prices were left unchanged: the repeat loop is profitable, while payroll and freight make idle time and tiny shipments costly. Hiring is free; assigned operator and driver wages remain $15 per game hour each, including idle assigned time while the plant clock runs.

Evidence: `Saved/Logs/FocusedCheckTiming.json`, `PlaytestBacklogNativeQA.json`, `OwnedEquipmentUIQA.json`, `StarterBusinessNativeQA.json`; screenshots under `Saved/Screenshots/Reviews`. These are local development/PIE checks, not packaged-build, remote-network or production-FPS acceptance. Continue/Load main-menu persistence remains outside this pass. No commit, push or merge performed.

## Starter economy rebalance — 31 October 2026

The measured playthrough above is the pre-rebalance baseline. The next revision targets ordinary-order progression rather than relying on rush premiums or free advances.

- Ordinary sale prices: bracket blanks $18 → $23; mounting plates $8 → $11; formed brackets $30 → $42. Order modifiers still apply. Material weights and processing times are unchanged.
- Wages remain $15 per game hour for assigned operators/drivers, including idle time. Steel remains $1/lb; supplier and carrier fees and the $4,500 press-brake purchase remain unchanged.
- Unaccepted listings refresh to current balance prices. Accepted contracts retain their signed price, including across save/load migration.
- A working plasma output pallet accumulates up to ten blanks before a downstream supply trip. Smaller remaining demand and completed, disabled or changed recipes release partial batches; generic stocked pallets remain immediately available. This is trip batching, not a full-pallet requirement.
- Process displays explicitly say game minutes; payroll says game hours. Real animation and movement timers are unchanged.
- The repeatable benchmark uses real commerce commands, production, paid steel, payroll, freight, delivery and ledger validation. Its starter cases begin with the actual $4,000. The separate upgrade comparison labels its $10,000 controlled fixture and reports capital and completed offer-generation warmups separately; it compares operating return per game minute including changes in raw-steel value.

### Repeatable simulation results

On the same first three ordinary contracts (40 blanks, 60 plates, 60 plates), serial cash gain rises from **$231.61 to $791.61**, or **$263.87/order**. Keeping those three jobs queued improves the tuned result to **$959.60**, because the assigned staff spend less time waiting. Both cases buy their steel and pay actual inbound/outbound freight and payroll; neither uses rush premiums or customer advances. The difference from the earlier native result includes UI/wait time, which still incurs payroll.

The ordinary-order simulation reaches **$5,737.72 cash after seven jobs**, enough to buy the $4,500 brake and retain **$1,237.72**. This milestone assumes players avoid unrelated purchases. Whole-sheet purchasing still causes temporary cash dips and leaves usable inventory.

In the separate controlled upgrade comparison, inventory-adjusted operating returns are **49.54 cents/game minute for blanks** and **104.43 cents/game minute for formed brackets**, including the additional staffed brake and haul costs. Different genuine order quantities (40 blank / 30 formed) are normalized by game time and raw-steel asset changes. The $4,500 capital purchase and completed warmup orders are reported separately rather than hidden inside operating margin. This is a local simulation comparison, not a promise about every contract or player layout.

### Native in-game confirmation after rebalance

The fresh unsaved starter playthrough completed all **98/98 checks** with no grants, no edited contracts and no player-save writes. The same three ordinary orders paid **$2,240.00** revenue against **$1,495.70** cash costs, including **$843.50 wages**. Cash rose **$4,000.00 → $4,744.30**, a **$744.30** increase (about **$248.10 per order**). Usable steel remains after the final order. UI interaction/wait time explains the lower result versus the repeatable simulation benchmark; idle assigned staff continue to be paid.

Evidence: `Saved/Logs/StarterBusinessNativeQA.json`, `StarterBusinessEconomy.log`, and `Saved/Screenshots/Reviews/Business_ThreeJobs_FinalInventory00001.png`. Earlier native financial evidence was copied to `StarterBusinessPreEconomyQA.json`. The price/batching changes were exercised in native PIE and the real model; no new packaged-build or remote-network acceptance is claimed.

Final rebalance verification: editor build succeeded; **190/190 automation checks passed** (54.43 seconds), including the progression targets, partial downstream batches and signed-price save/load preservation. All eight ordinary benchmark contracts completed; the upgrade reserve target passed at seven. No commit, push or merge performed.

### Additional small profit increase

At the user's request, ordinary prices were increased again: blank brackets **$24**, mounting plates **$11.50**, formed brackets **$44**. Costs, wages, material consumption, process times and freight remain unchanged. Accepted agreements still keep their signed prices.

The repeatable real-model benchmark now returns **$891.61 cash gain over the first three ordinary orders**, averaging **$297.20/order**, versus $263.87 before this small adjustment (about 12.6% more net cash). After five ordinary orders it holds **$5,568.96**, enough for the $4,500 brake plus **$1,068.96** remaining cash. All eight orders complete, including partial pallets and paid transport. This latest result is simulation/automation evidence; the earlier 98-check native playthrough used $23/$11/$42 and has not been rerun for this price-only change. Baseline benchmark preserved in Saved/Logs/EconomyBeforeSmallProfitBump.json.
