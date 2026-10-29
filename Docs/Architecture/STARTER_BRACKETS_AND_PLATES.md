# Starter brackets and plates playtest

The focused starter loop is steel purchasing, plasma cutting, automatic pallet/rack handling, freight pickup, delivery, payment, and another order. Saleable starter plasma recipes are bracket (5 lb, 8 seconds), mounting plate (2 lb, 4 seconds), gusset plate (3 lb, 6 seconds), and reinforcement plate (8 lb, 12 seconds). Base sale prices are $11 per bracket, $5 per mounting plate, $7 per gusset plate, and $17 per reinforcement plate; each offer records its actual price, including upfront/rush adjustments. Recipe weights and times are fixed; changing the program retains sheet pounds. A new sheet adds its pounds to the remaining machine material balance.

Employees are hired free as named unassigned people, then assigned to specific equipment. Initial starter staff remain available. The plasma requires its operator physically at the station, and the owned forklift requires its driver. New equipment is not automatically staffed. No payroll or new hiring charge is introduced.

Customer offers replenish. Bent brackets and support channels are excluded from offered contracts until the business owns an installed press brake with an assigned operator. The server rejects stale advanced offers too. Existing accepted work is preserved if staffing changes. The starter does not require buying this upgrade.

For each order: accept it, assign the pickup zone, buy needed sheets, program the exact part and quantity, run the simulation, wait for finished pallets to stage, book a carrier that fits the selected pallets, then wait for customer delivery and payment. Select the next order after staged pallets have shipped. Suppliers and pickups share one bay and can queue. Paying a pickup fee does not immediately pay the customer invoice.

The first 40 brackets and 60 mounting plates produce $740 revenue. Two individual sheet purchases including supplier freight cost $560.48; two default flatbed pickups cost $80. Remaining cash gain is $99.52 before actual equipment operating costs, with about 190.48 lb left on the plasma. Parts and steel remain usable between orders.

Focused automated acceptance cases are `Foundryline.Commerce.StarterOfferCapability` and `Foundryline.Commerce.TwoRecipeRepeatSales`. The latter uses actual paid supplies, employee-gated cutting, forklift staging, carrier deliveries, exact payments, and ledger validation across both recipes. Record executed results separately; source presence alone is not a passing test.

This is a starter sales playtest, not completed progression. There is no emergency loan, equipment resale, contract abandonment, implemented reputation consequence, or production-floor expansion beyond the authored layout. Keep cash for material, pickup fees, and operating costs. Research and the larger tanker business are outside this focused loop.

## Executed backend acceptance � 2026-10-28

`Saved/Logs/StarterSalesAcceptance.log` records 12/12 Commerce tests successful, exit code 0, including both new starter cases. The real simulation completed 40 brackets followed by 60 mounting plates: $740 delivered revenue, $655.07 total costs, $4,084.93 ending cash, no unpaid costs, and 190.479 lb remaining on the plasma. The $14.59 above material/freight/pickup costs is actual equipment operation in that deterministic test. This is backend automation evidence; native ERP input acceptance is recorded separately by the UI review.

## Starter UI and ordinary play acceptance — 2026-10-28

The office ERP now exposes Pause/Resume and explicit 1x/4x/8x controls on every
module. Overview shows the next order/steel/production/shipping action. Program
this order opens the plasma computer with the correct recipe and outstanding
quantity, subtracting existing unbooked matching stock. Starting remains explicit.
Incoming deliveries and finished parts get distinct guidance; parts already cut
are directed to pickup instead of another batch.

A fresh rendered starter session was played with ordinary mouse/keyboard UI only:

- Accepted FL-0001: 40 bracket blanks at $11; assigned bay 01.
- Purchased two sheets together for $535.48 including the $25 supplier delivery.
- Clicked ERP 8x and Resume. Programmed and started 40 brackets; forklift received
  steel, supplied plasma, removed output and staged the pallet automatically.
- Selected the actual 40-part pallet on the tablet and booked the $40 flatbed.
  Delivered and paid $440 on Day 2 at 12:06.
- Accepted FL-0002: 60 mounting plates at $5; assigned the same bay. Programmed
  and started 60 plates without discarding the remainder of the first sheet.
- Selected the actual 60-part pallet and booked the second $40 flatbed.
  Delivered and paid $300 on Day 3 at 12:09.
- Final paused balance $4,111.55; delivered revenue $740; total spending $628.45
  ($535.48 steel/delivery + $80 pickups + $12.97 equipment operation). Plasma
  showed 190.5 lb usable steel remaining. Eight fresh offers remained available.

No test time advancement, money grants, inventory grants, manual completion flags,
or user save writes were used for this rendered playthrough. A read-only observer
launch did not attach, so this ordinary-input evidence is the observed UI and its
captured screenshot, separate from deterministic automated reports.

Actual image: `Saved/Screenshots/Reviews/StarterSales_TwoOrdersPaid.jpg`.
Build succeeded (`StarterSalesBuild.log`); all 12 focused commerce tests passed
(`StarterSalesAcceptance.log`), including capability gating and the two-recipe
repeat-sale test. The automated test buys sheets separately, hence its $4,084.93
balance differs from the live bulk purchase. Final guidance-only correction built
in `StarterSalesFinalBuild.log`; targeted smoke recorded separately below.

This is the starter bracket/plate loop. Production-floor expansion, bankruptcy
recovery, and the larger product progression remain separate unfinished systems.

Final smoke: 2/2 targeted commerce cases passed with exit code 0 in `StarterSalesFinalSmoke.log` after the guidance correction. The ERP filter matched no additional cases; no extra UI automation pass is claimed.
