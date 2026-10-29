# Starter play-flow audit — 28 October 2026

## Actual native player checkpoint

Used the existing running starter, ordinary mouse/keyboard controls and shipped
menus. No money, inventory, material or elapsed-time grants were used.

1. Opened the office ERP with E. Overview correctly identified no order, no raw
   steel and paused production.
2. Accepted FL-0001: 40 bracket blanks for $440, paid on completion, due in 48 game
   hours. The marketplace replenished its offer list after acceptance.
3. Assigned the order to loading bay 01.
4. Purchased the default two standard sheets: $510.48 material plus $25 delivery;
   visible cash changed from $4,000 to $3,464.52.
5. Entered office-gated Property View, opened the older manufacturing sidebar with
   P and used its shipped 1x/4x/8x speed cycle. Started the plant and opened the burn
   table. Selected the bracket recipe and typed a 40-part batch quantity.

The focused audit ended at this checkpoint. **This was not a completed two-order
manual playthrough**: cutting, loading, delivery and second-order payment were not
claimed from this run. Earlier native and multiplayer acceptance proved one full
order loop; those tests are separate evidence, not a substitute for this audit.

## Findings, in priority order

1. **Expansion is visually connected but not a production-floor progression yet.**
   Construction expansion does not update the factory model's seeded floor bounds.
   A player can enlarge the shop shell without obtaining matching usable machine,
   work-zone and routing space.
2. **The marketplace accepts work beyond current equipment capability.** Bent
   brackets/support channels appear before the player owns a press brake. There
   is no clear capability gate or abandonment/recovery path for a bad commitment.
3. **First-person time controls are incomplete.** ERP provides pause/resume but
   speed is only in the older Property View manufacturing sidebar. Requiring
   office → Property View → P just to reach 4x/8x makes normal waiting harder to
   discover and undermines the first-person primary workflow.
4. **Working capital and recovery need guidance.** Display material, freight and
   operating reserves before an upgrade. No loan, resale or contract cancellation
   currently recovers an exhausted cash/inventory position. This is a possible
   bad-decision dead end, not a proven inevitable bankruptcy on the planned path.
5. **Progression presently tops out at the brake and second forklift.** Larger
   product business unlocks, research and reputation consequences are not yet
   connected to this commercial starter loop. A bought brake is deliberately
   unstaffed; hiring and assigning an operator must be an explicit next action.

## Economy/source cross-check (not a manual play result)

Demand refreshes, so the offer pool is not finite. Five eligible plasma-only
contracts (first bracket, mounting plate, gusset, reinforcement and next bracket)
pay $2,274 and consume 1,035 lb. Five sheets leave 241.20 lb. With five separate
$25 inbound fees and five default $40 pickups, projected cash is $4,672.80 before
operating costs; the first four contracts alone reach only $4,443.04 before costs.
Thus the $4,500 brake is roughly a five-order milestone, subject to actual costs
and shipping choices. After purchase, retained steel can cover either the next
30 bent brackets (150 lb) or 30 support channels (240 lb); freight and operating
reserve still matter. Steel remaining on the plasma table carries forward and
can be used after changing recipes.

Recommended next work: connect buildable expansion to usable factory space,
add capability-aware order guidance and visible first-person speed controls,
then make the next profitable order/upgrade/reserve plan explicit. These are
findings and recommendations; this audit did not implement a new progression
system or change recipe prices, times or contracts.
