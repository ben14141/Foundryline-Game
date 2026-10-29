# ERP and local playtest acceptance — 28 October 2026

The office computer now opens Foundryline ERP in a near-fullscreen light workspace,
with orange/grey accents, live business modules, document details and eleven stable
customer, supplier and carrier vector identities. Overview links to Sales, Purchasing,
Production and Dispatch. Inventory shows real physical pallets; Accounts uses live
cash, delivered revenue and total spending. Mail has separate selectable records
and a reading pane. All visible actions are connected to existing game commands.

## Verification

| Evidence | Result | Artifact in Saved/Logs |
| --- | --- | --- |
| Final C++ editor build | Succeeded | ERPFinalBuild.log |
| Unreal automation | 93/93 passed | ERPFullAcceptance.log |
| Live multiplayer, four initial players plus fifth late join | 39/39 passed | CommerceNetworkQA.json / ERPNetworkReadiness.log |
| Rendered first-person production/shipping and ERP flow | 29/29 passed | CommerceNativeQA.json / ERPNative.log |
| Actual mouse and E/Escape keyboard interaction | 12/12 passed | ERPManualUIQA.json |
| Native equipment/material/mast/motion observer | 6/6 passed, 1302 samples | PlaytestWorldVisualProbe.json |
| Read-only cash/material/reservation and motion observer | 5/5 passed, 1041 samples | BackendPlaytestObserver.json |
| Native world views | 10 captured, light/material checks passed | PlaytestWorldCameraQA.json |

The rendered flow accepts a 40-bracket order, buys one steel sheet, receives and
racks stock, starts the real plasma batch, stages and inspects its pallet, books
freight, loads the actual flatbed, delivers and receives the exact $440 payment.
It then resets to a fresh paused shop with $4,000, no stock and no running batch.
ERP pages, populated Inventory/Production, E inspection, office-only Property View,
return-to-office position and roofs were checked. Manual mouse checks covered the
Overview shortcut, Sales tabs, changing emails, quote quantity updates, Production
and actual machine control. Manual changes were drafts only.

Multiplayer checks cover simultaneous commands, one-time booking fees, company
cash/state separation, foreign-target rejection, mixed walking/Property View and
late joining. The test waits on private-state readiness rather than assuming a
fixed wall-clock delay; an unrelated company's private actor fails immediately.
The final late join became ready after 3.5 seconds. This is local PIE evidence.

## Playtest fixes

The audit fixed stale Sales record selection, overlapping mail content and duplicate
Canvas shortcut hitboxes. A missing driveway connection was restored and widened;
measured wheel-contact sweeps keep the carrier on authored pavement throughout
arrival, docking and departure. Carrier cab/chassis/bed and primary plasma, brake
and roller bodies now block the actual player capsule. Collision tests verify
blocked body sweeps, clear space outside each body and unaffected dynamic helpers.
The forklift root stays at floor height while its mast lifts. Plasma motion and
visible sparks were observed in the real production run.

## Review state and limits

Foundryline is left open in a fresh first-person starter shop beside the office
computer. Press E to open ERP. Property View remains office-gated; returning
restores the walking view and roof visibility. Normal user saves were not
replaced. Spellscape was not changed. No push or merge was performed.

This is local editor/PIE acceptance, not a packaged build, hosted internet session,
a production player-cap test or an FPS guarantee. Combined independent-business
world saving, physical loose-pallet simulation and pedestrian/AI yielding remain
future systems. Larger carrier classes retain prototype visuals. These limits do
not prevent the tested starter cut-parts order loop.
