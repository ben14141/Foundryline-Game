# Cut-parts starter: first order loop

The starter shop now has a server-authoritative purchasing, production and shipping
loop. The larger tanker workshop remains separate. Physical units remain centimetres;
the UI uses feet/inches and pounds. Prices/times are initial balancing values.

## Start and recipes

Start with $4,000, one plasma table and assigned operator, a forklift/driver, one raw-sheet rack, three
finished-pallet rack positions and two pickup positions. No free stock or automatic
batch. Buy sheets at the office and set the machine's part/quantity. Starting an accepted
batch automatically resumes the plant. The assigned operator walks to the plasma console before cutting begins; manage assignments under Team & logistics. An off-duty operator pauses the real machine cycle without losing its reserved steel. Explicit machine and plant pause controls
remain available.

| Part | Steel | Plasma | Brake | Base price | Pallet cap |
| --- | ---: | ---: | ---: | ---: | ---: |
| Mounting plate | 2 lb | 4 s | - | $5 | 100 |
| Gusset plate | 3 lb | 6 s | - | $7 | 75 |
| Bracket blank | 5 lb | 8 s | - | $11 | 50 |
| Reinforcement plate | 8 lb | 12 s | - | $17 | 30 |
| Bent bracket | 5 lb blank | 8 s upstream | 8 s | $19 | 50 |
| Support channel | 8 lb plate | 12 s upstream | 12 s | $30 | 20 |

Sheets are 60 x 120 x 1/8 inches, approximately 255.25 lb, at $1/lb plus a $25
supplier delivery fee. Buy 1-10 whole sheets. There is no nesting. Each cut consumes
fixed pounds; changing recipes retains the sheet balance. A queued recipe change
waits for the current cut. The next sheet adds pounds when the balance is too low.
Crossing sheets or changing recipes does not create scrap.

The $4,500 press-brake upgrade includes linked input rack positions. A second
forklift/driver costs $1,200; the starter has two parking stalls. Working machinery
and forklift costs accrue in cents; consecutive operating costs aggregate in the
bounded account journal.

## Orders and shipping

The office has a public order site and customer emails. Offers fix part, quantity,
price, payment terms and an optional deadline. Acceptance starts the timer. Flexible
orders have no deadline; rush offers pay more. Late status is tracked; reputation,
traits and penalties are later work. At most six accepted orders are active.

Payment is upfront, per delivered load, or on complete fulfillment. Partial pallets,
including a pallet containing one bracket, can ship. Truck loading/departure does not
fulfill orders: customer delivery does. Delivered manifests apply exactly once.
Upfront orders do not pay twice. Booking cannot over-commit an order's remainder.

Assign an accepted order to the pickup area. Matching parts move automatically from
finished racks. Each pallet holds one part type. Select a carrier, then individual
staged pallets. Both pallet slots and gross weight, including pallet tare, must fit.
Booking charges the flat fee immediately and locks exact pallet IDs/quantities.
Duplicate/stale selections, locked pallets and over-fulfillment fail on the server.

| Carrier | Vehicle | Slots | Payload | Fee | Lead | Transit |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| Swift Van Couriers | Pallet van | 1 | 900 lb | $25 | 15 min | 60 min |
| Foundry Lane Haulage | Starter flatbed | 2 | 2,000 lb | $40 | 20 min | 90 min |
| County Freight | Rigid delivery truck | 6 | 6,000 lb | $90 | 40 min | 120 min |
| Interstate Pallet Lines | Box semi | 20 | 24,000 lb | $180 | 60 min | 240 min |

Long sheet pallets use two nominal spaces. The starter staging area physically has
two positions; larger carriers do not add floor space. Calendar bookings may overlap
and queue FIFO at one shared receiving/shipping bay. Full raw racks hold the unloading
truck at the bay. Incoming sheets move directly from truck to raw rack, without a
receiving staging task. Rebooking before dispatch costs half the ORIGINAL fee each
time. Travelling carriers are locked.

One running simulation second currently advances one game minute. Pause and the
existing 1x/4x/8x speed affect both production and the shared calendar. Simulation
seconds on recipe cards are game balancing values rather than real workshop timings.

## Authority and persistence

Operations derive the actual connected player's company/plant permissions. Reliable
controller commands have sequence/rate limits. Company cash stays in the server
registry; account commits use the expected balance. Private orders, bookings and cash
replicate only through a company-relevant operations actor. Public physical factory
records omit this private commerce payload. Screens/cameras remain per-player.

Drivers independently prioritize truck, machine and storage tasks. Reservations and
booking locks are separate. Whole-pallet moves retain identity through forklift and
save/load. Schema version 6 persists commerce, machine origins and priorities.

Host saving is single-company only; multiple businesses still require a combined
world-save milestone. Loading validates factory state, reserves the account change,
and rolls it back if atomic property restoration rejects. Loading before/after an
upgrade removes/restores the brake, its rack collision and navigation obstacles.
Automated save tests use isolated FoundrylineAutomation_Commerce* slots, never the
player's normal save slot.

Online authentication, research, reputation penalties, final economic balance and
large-town/FPS acceptance remain separate milestones. Carrier presentation and
street routing are verified separately from order/inventory correctness.

## Evidence

The ERP playtest-readiness build passed all 97 Unreal automation tests, including
ten commerce tests. `Saved/Logs/ERPDesktopV2FullAcceptance.log` records the full run. Four additional
ERP cases verify real documents, missing timestamps, debt-aware graph values and
local document/sort actions without authoritative mutation.
Coverage includes the complete 40-bracket purchase-to-delivery order, one-bracket
partial pallets with all three payment terms, two drivers shipping two full pallets,
carrier slot rejection, rebooking fees and dispatch locks, recipe changes across
sheet boundaries, FIFO blocking at full raw racking, both brake recipes reaching finished racks,
driver priorities, long-session completed-history trimming,
booked-manifest memory save/resume, forged-state rejection, and isolated world-save
account rollback plus upgrade removal/restoration.

The world-save test cleaned up its isolated automation slots. It did not overwrite
the player's normal save.

The redesigned ERP/native first-person workflow passed 49/49 rendered checks in
`Saved/Logs/CommerceNativeQA.json`. Its independent read-only observer completed
957 live samples with no invariant failures in `BackendPlaytestObserver.json`:
authoritative state validity, company-account synchronization, material conservation,
pallet reservations, forklift ground height and movement, and active machine cycles.
It observed 55 distinct actual plasma torch positions. It changed no gameplay,
camera, input, saves, settings or screenshots.

The ERP multiplayer rerun passed 39/39 live checks in `CommerceNetworkQA.json`,
with four initial players, three isolated companies, competing accept/book requests,
exactly-once delivery/payment, foreign-target denial and an actual fifth late join.
The late client's own private state was ready after 3.5 seconds in the readiness run.
The harness waits for actual registry/plant/private-channel readiness and fails
immediately if another company's private actor appears. An earlier fixed-delay
sample ran before that state was ready; no runtime privacy/backend change was needed.
