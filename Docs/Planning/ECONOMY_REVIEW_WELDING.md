# Economy review: the welding chain (29 October 2026)

> October 31 payroll policy and current profit measurements: [Daily payroll and profit](../Validation/DAILY_PAYROLL_AND_PROFIT_2026_10_31.md).

> Later collector-era prices supersede this historical review: see [Pricing Collector Update](PRICING_COLLECTOR_UPDATE.md). The later adjustment was not rebuilt or retested.

Ben: "Check pricing. I have not at all. So economy is not set at all." This review reads the current numbers from code, works out what one headache rack really costs and earns, and proposes a coherent price set.

**Status: the full proposed set was applied on 29 October 2026 at Ben's choice** (branch `welding/economy-v1`). The values now live in `Public/Economy/FoundrylineEconomyTuning.h`, and wages are paid by `FFoundrylinePayroll`. Sections 1–2 describe the numbers *before* the change. See "Result after applying" at the end.

Time base: one plant second is one game minute (`TickCommerce` adds the plant step to the commerce clock, which the UI shows as minutes).

## 1. Current numbers (from code)

| Item | Current value | Where |
| --- | --- | --- |
| Starting cash | $4,000.00 | `SeedCutPartsShop` working capital |
| Steel sheet | 255.24 lb (60 x 120 x 1/8 in), $1.00/lb = **$255.24/sheet** | `FFoundrylineSteelSheet`, `BuySteel` |
| Steel delivery | **$25.00** per purchase (1–10 sheets) | `BuySteel` "Supplier freight booking" |
| Plate (mounting) | 2 lb of steel (127 per sheet), 4 game min on the plasma | `DefaultPrograms` |
| Bracket | 5 lb of steel (51 per sheet), 8 game min on the plasma | `DefaultPrograms` |
| Gusset / reinforcement plate | 3 lb / 8 lb, 6 / 12 game min | `DefaultPrograms` |
| Customer prices for cut parts | plate $5, gusset $7, bracket $11, reinforcement $17, bent bracket $19, support channel $30 | `SalePrice` |
| Cut-part order sizes | 30–75 parts | `RefreshCommerceOffers` |
| Frame section (bought) | **$24.00** each, 16 lb, 12 per bundle | `Bought` |
| Light kit (bought) | **$38.00** each, 4.5 lb, 8 per crate | `Bought` |
| Parts delivery | **$25.00** per purchase booking (one part type per booking) | `SupplierFreightCents` |
| Pickup trucks | Pallet van **$25** (1 space, 900 lb, 160 cm deck; refused for racks); flatbed **$40** (2 spaces, 2,000 lb); rigid **$90** (6 spaces, 6,000 lb); semi **$180** (20 spaces, 24,000 lb) | `Carrier` |
| Rack pallet | 2 deck spaces; kit mass 129 lb per rack + 120 lb pallet | `PalletSpaces`, `PalletWeight` |
| Wages | **none**: hiring is free and staff are never paid | no wage system exists |
| Running costs | 1¢ per game minute per working machine (the plasma while cutting); 3¢ per game minute per forklift on a job; the welding station costs nothing to run | `TickCommerce` |
| Equipment | welding station **free** to place; press brake $4,500; second forklift $1,200 | `BuyBrake`, `BuyForklift` |
| Headache rack order | **2 or 3 racks at $450.00**, 72 game hours, pays on completion or per delivery | `RefreshCommerceOffers` |
| Rack build time | 146 work-minutes per rack (30 fit-up, 90 weld, 20 lights, 6 move). Measured welding time is **138 game min (2.3 h) with one welder** and **80 game min (1.33 h) with two** (×1.7) | `StageSeconds`, PlayerFlow test |

## 2. Unit economics per headache rack (current prices)

Each rack uses 2 plates (4 lb) + 4 brackets (20 lb) = 24 lb of steel, 6 frame sections and 2 light kits.

| Per rack | Scripted PlayerFlow (5 racks) | Rendered PIE run (5 racks) | Clean case* |
| --- | --- | --- | --- |
| Steel (1 sheet bought, 120 lb used) | $51.05 | $51.05 | $24.00 + $2.35 delivery |
| Frame sections | $144.00 | $144.00 | $144.00 |
| Light kits | $76.00 | $76.00 | $76.00 |
| Parts and steel deliveries | $35.00 (7 bookings) | $35.00 (7 bookings) | $25.00 (2 per 2-kit refill) |
| Pickup trucks | $54.00 (3 rigid) | $54.00 (3 rigid) | $16.00 (1 flatbed per 2.5-rack order) |
| Running costs | $6.33 | $6.15 | $6.00 |
| Labour | $0.00 (no wages) | $0.00 | $0.00 |
| **Cost per rack** | **$366.38** | **$366.20** | **$293.35** |
| **Gross margin at $450** | **$83.62 (18.6%)** | **$83.80 (18.6%)** | **$156.65 (34.8%)** |
| Net over the run | +$418.11 | +$419.02 | |

\*Clean case: steel and deliveries amortised over full use, and the cheapest truck that takes the pallet (a whole rack order fits one pallet on a flatbed).

Bought-in parts (frames + lights) are $220 per rack: **49% of the $450 price**.

Per game hour of the station: one welder earns $83.62 / 2.3 h = **$36/h**. The plasma cutting brackets for a customer earns about **$45/h** gross: 7.5 brackets an hour at $11, less 5 lb of steel each. **Today, cut parts pay better per hour than racks.**

### Why the two runs first differed, and why they now agree

The first rendered run ended at $3,540.65, $877.71 below the scripted flow. The game economy was not the cause; a review tool was:

- `AdvanceFactoryTest` steps the plant model directly, skipping the company cash sync.
- Order 1's $900.00 payment landed during one of those steps, so the next real tick overwrote the model's cash with the company balance and the payment was lost. So were about $22.29 of running costs: $900.00 − $22.29 = $877.71.
- The same tool also tops up free steel for the old tanker fixture.

The Phase 6 review now advances time with `AdvanceCommerceTest`, which runs real plant ticks with the cash sync. That run ends at +$419.02, within $0.91 of the scripted flow (only the running-cost timing differs). Remaining real differences from the clean case:

- **Steel:** one whole sheet ($255.24 + $25) is bought for 120 lb of use; 135 lb is left on the rack.
- **Trucks:** both runs booked the rigid truck ($90). A rack order fits one pallet, which a flatbed ($40) carries.
- **Deliveries:** the station buys only what its 2-kit bins lack, so a 3-rack order needs an extra partial refill: 7 bookings instead of 5.

## 3. Proposed price set (recommendation; not applied)

Targets:

- 35–45% gross margin on a rack after materials, freight and labour.
- Bought-in parts at 25–35% of the rack price.
- Racks pay better per hour than cut parts, but cut parts stay worth doing.
- Real-world sanity: steel headache racks retail for a few hundred dollars (about $300–700 for steel; aluminium costs more). A shop selling to truck outfitters at about $450–500 is believable.

Recommended set:

| Item | Current | Proposed | Why |
| --- | --- | --- | --- |
| Headache rack price | $450 | **$480** | Within the wholesale band; lifts the margin into target with labour included |
| Rack order size | 2–3 | **2–5** | Fills one OUT pallet (5) and one flatbed |
| Frame section | $24 | **$18** | 6 per rack = $108 |
| Light kit | $38 | **$25** | 2 per rack = $50; bought-in total $158 = **33%** of $480 |
| Parts delivery | $25 | $25 (keep) | |
| Steel | $1.00/lb, $25 delivery | keep | Plates and brackets cost $24 of steel per rack |
| Cut-part prices | plate $5, bracket $11, … | **keep** | ~43% margin on an order, ~$45 per plasma hour |
| Welder wage (new) | none | **$18 per game hour** while assigned | 146 work-minutes per rack (section 5) = 2.43 h = $43.80 with one welder; two welders 2 × 1.43 h = $51.53 |
| Plasma operator and forklift driver wages (new, optional) | none | $15 per game hour each | Only if wages are added for every role; see section 4 |

Per rack with the proposed set (clean case), one welder:

| | Amount |
| --- | --- |
| Steel | $26.35 |
| Bought-in parts | $158.00 |
| Parts deliveries | $25.00 |
| Pickup truck share (flatbed) | $16.00 |
| Running costs | $6.00 |
| Welder labour (146 min at $18/h) | $43.80 |
| Plasma operator labour (2 plates × 4 min + 4 brackets × 8 min = 40 min at $15/h) | $10.00 |
| Forklift driver labour (about 12 min of trips per rack at $15/h) | $3.00 |
| **Cost per rack** | **$288.15** |
| **Margin at $480** | **$191.85 (40%)** |

Labour here comes from the process times in section 5, not a flat guess.

With two welders, welder labour is $51.53 and the margin is 38%, but the station finishes a rack in 1.43 h instead of 2.43 h. That gives about **$79/h of margin with one welder and $129/h with two**, against about $45/h for the plasma cutting customer brackets, so racks are clearly the better work and a second welder pays.

**Levers Ben can turn:**

- Rack price ($400–550).
- Frame and light kit prices (the bought-in share).
- Welder wage (or none).
- The two-welder rate (1.7).
- Rack order sizes.
- Delivery fee per booking.
- Racks per OUT pallet (5).
- Cut-part prices, if racks should pay even more relative to plasma work.

## 4. What a new player experiences (from $4,000)

**Current prices.** First order of 2 racks:

| Step | Cash out | Cash left |
| --- | --- | --- |
| Place the station, hire a welder | $0 | $4,000.00 |
| 1 sheet of steel + delivery | $280.24 | $3,719.76 |
| 12 frame sections + delivery | $313.00 | $3,406.76 |
| 4 light kits + delivery | $177.00 | $3,229.76 |
| Rigid truck | $90.00 | $3,139.76 |
| Payment on completion, about 9–10 game hours after accepting | +$900.00 | about $4,030 |

With no wages, a player can never run short here. The first order still only just breaks even, because the bins buy 2 kits at once and the sheet of steel lasts about 10 racks.

**Proposed prices, with wages for the welder, plasma operator and forklift driver.** Roughly 10 game hours × ($18 + $15 + $15) = $480 of wages before the first payment. The first outlay is about $280 steel + $241 parts + $40 flatbed = $561. So about $1,040 is spent before the first $960 arrives. Cash stays above $2,900 throughout, and later orders run at the margins above: affordable, with the first order roughly break-even and profit from the second.

## 5. Process times (every item in the chain)

1 plant second = 1 game minute. Labour cost uses today's wages ($0, so every item costs $0 in labour today) and the proposed wages ($18/h welder, $15/h plasma operator and forklift driver). Times marked **instant** have no process time today.

### Parts (plasma, brake, roller)

| Step | Current (plant s = game min) | Proposed | Labour now / proposed | Source and reasoning |
| --- | --- | --- | --- | --- |
| Cut a mounting plate | 4 | 4 (keep) | $0 / $1.00 | `DefaultPrograms`. A 2 lb plate is a short cut path |
| Cut a gusset plate | 6 | 6 (keep) | $0 / $1.50 | `DefaultPrograms` |
| Cut a bracket blank | 8 | 8 (keep) | $0 / $2.00 | `DefaultPrograms`. A longer profile with holes |
| Cut a reinforcement plate | 12 | 12 (keep) | $0 / $3.00 | `DefaultPrograms` |
| Cut a rail blank / shell blank (tanker line) | 15 / 12 | keep | $0 / $3.75 / $3.00 | `DefaultPrograms` |
| Load a sheet onto the plasma bed | **instant** | **5** | $0 / $1.25 | `Machines()` moves a sheet from the input slot into `BedLb` at once. A crane or forklift set-down plus squaring takes a few minutes; one sheet feeds about 10 racks' plates and brackets, so it adds little per rack |
| Clear offcuts / unload cut parts | **instant** (parts appear on the output stack) | 0 (keep instant) | — | Folded into the cut times |
| Press brake: bent bracket / frame rail / support channel | 8 / 16 / 12 | keep | $0 / $2.00 / $4.00 / $3.00 | `DefaultPrograms`. Not used by the rack chain |
| Roller: shell section | 18 | keep | $0 / $4.50 | `DefaultPrograms`. Tanker line only |

### Logistics

| Step | Current | Proposed | Labour now / proposed | Source and reasoning |
| --- | --- | --- | --- | --- |
| Forklift pick-up or set-down | 2 each | 2 (keep) | $0 / $0.50 per trip | `J->Handling=2` at each end of a job |
| Forklift travel | 150 cm/s straight, 65 cm/s in turns | keep | about $0.50–1.00 per trip in the starter shop | `DrivingSpeed`. Measured trips are about 1–3 game min |
| Rack put-away (station OUT to storage or the bay) | travel + 2 + 2 | keep | about $0.75 per pallet | One whole OUT pallet per trip |
| Unload a delivery at the station's IN pallets | 8 | 8 (keep) | $0 / $2.00 (driver) | `UnloadSeconds` |
| Take a kit from the IN pallets at the start of a unit | **instant** | 0 (keep): it is part of the 30 min fit-up | — | `AssembleWeldingStations` reserves the kit when fit-up starts |
| Steel delivery lead time | 60 default, at least 30 | keep | — | `BuySteel` ETA |
| Frame or light kit delivery lead time | 60 default, at least 30 | keep | — | `BuyParts` ETA |
| Unloading a supplier truck | forklift trips (2 + travel + 2 each) | keep | about $1 per pallet | `PlanCommerce`, inbound |
| Pickup truck: booking to arrival | at least the carrier's lead: van 15, flatbed 20, rigid 40, semi 60 | keep | — | `Carrier` LeadMinutes |
| Pickup truck: loading | forklift trips from the pickup zone | keep | about $1 per pallet | |
| Truck leaves after loading | 20 | keep | — | `Departing` → `InTransit` |
| Transit to the customer (payment on arrival) | van 60, flatbed 90, rigid 120, semi 240 | keep | — | `Carrier` TransitMinutes |
| Payment on arrival | **instant** | keep instant | — | Invoicing is abstracted |
| Hire a person | **instant** and free | **instant**, wage starts on assignment | — | `HireEmployee` |
| Second forklift / press brake install | **instant** | 30 / 120 (install time) | — | Purchase is immediate today. A short install time would make the upgrades feel real; optional |

### Assembly: headache rack at the welding station

| Step | Current, one welder | Two welders (×1.7) | Labour per rack now / proposed (one welder; two welders) | Source and reasoning |
| --- | --- | --- | --- | --- |
| Fit-up (kit on the jig) | 30 | 17.6 | $0 / $9.00; $10.59 | `StageSeconds(1)` |
| Welding | 90 | 52.9 | $0 / $27.00; $31.76 | `StageSeconds(2)`. About 1.5 h of MIG on a cab rack is believable |
| Light installation | 20 | 11.8 | $0 / $6.00; $7.06 | `StageSeconds(3)` |
| Lift onto the OUT pallet | 6 | 6 (rate does not apply) | $0 / $1.80; $3.60 | `StageSeconds(4)` |
| **Rack total** | **146 min (2.43 h)** | **88.3 min (1.47 h)** | **$0 / $43.80; $53.00** | Measured welding-stage time in PlayerFlow: 138 / 80 min (stages 1–3, which the 1.7 rate applies to) |

Labour per rack at the proposed wages:

| Labour | One welder | Two welders |
| --- | --- | --- |
| Welder | $43.80 | $53.00 |
| Plasma operator (2 plates, 4 brackets) | $10.00 | $10.00 |
| Forklift driver (about 12 min) | $3.00 | $3.00 |
| **Total** | **$56.80** | **$66.00** |

At today's wages ($0) labour adds nothing, which is part of why racks look cheap to make now.

## How these numbers were measured

- `Foundryline.Factory.WeldingStation.PlayerFlow` prints the ledger. In the scripted run the costs were steel $255.24 (1 sheet), frames $720, lights $380, supplier freight $175, trucks $270 (3 rigid) and running $31.65, for a total of $1,831.89; revenue $2,250; net +$418.11; 2,452 game minutes.
- `Tools/Unreal/review_welding_phase6.py` stores the final commerce report. Its costs were steel $255.24, frames $720, lights $380, supplier freight $175, trucks $270 (3 rigid) and running $30.74, for a total of $1,830.98; revenue $2,250; cash $4,419.02.

## Result after applying (29 October 2026)

`Foundryline.Factory.WeldingStation.PlayerFlow` is the same scripted player flow, one order at a time, and it ended at **$3,003.58** (−$996.42):

| Line | Amount |
| --- | --- |
| Revenue | $3,360.00 |
| Steel (1 sheet) | $255.24 |
| Frame sections | $756.00 |
| Light kits | $350.00 |
| Supplier freight | $275.00 |
| Pickup trucks (5 rigid) | $450.00 |
| **Wages** | **$2,227.91** |
| Running costs | $42.27 |
| Game time | 2,772 minutes (46 game hours) for 7 racks |

Racks still earn well over their materials and freight, but **wages are paid for every game hour a person holds a job, waiting included**. In this flow the plasma operator, the forklift driver and the welders mostly wait on supplier deliveries (60 min lead), trucks (40 min lead, 120 min transit) and one order at a time. That idle time costs about $48–66 an hour, and it is what turns the run negative. The per-rack labour estimate in section 3 counted only working minutes.

Ways forward, for Ben:

- (a) pay wages only while a person is working: this matches section 3's estimate, about 40% margin per rack;
- (b) keep hourly wages but lower them;
- (c) keep hourly wages and expect players to overlap orders. A station kept busy with two welders turns about $326 of rack revenue an hour against $66 of payroll.

Nothing beyond the chosen set has been changed.
