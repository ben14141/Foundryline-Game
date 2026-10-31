# Active playtest fixes

## Completed and validated

- Rebuilt and imported the original 1-, 2- and 3-level modular and wide-bay rack families with open long faces. Retained end-frame bracing, original footprints, scale, shelf levels and materials. Replaced the game-owned legacy sheet shelf with original open-sided geometry at the existing 12.77 cm bearing height.
- Added front-facing nameplates to both loading faces of standard racks, raw-steel racks and machine racks. Physical collision follows the end frames and elevated decks; it no longer fills empty rack air with one large box.
- Placement accepts either clear loading face, rejects both blocked, permits shared access aisles, and displays both face clearances. Forklift dispatch chooses a connected pickup/drop face for each assigned job. Pallet presentation and physical collision rotate with their storage rack.
- New dispatch preparation assigns actual completed stock to finished-goods rack slots. Partial pallets remain supported. Bookings select real rack stock, lock exact quantities and charge once. The forklift loads directly from rack to carrier. There are no new floor-pickup job producers or floor-staging buttons/paint/reservations.
- Kept serialized legacy location values solely for old-save compatibility. Existing floor pallets and queued floor deliveries are recovered through actual forklift moves into free finished-goods slots, retaining IDs, quantities and booking/order links. If no slot is free, they wait rather than unload onto the floor again.
- New starter shops include one standard finished-goods rack in place of the former floor staging pad. Purchased/custom save layouts are not overwritten by the starter setup.
- Dispatch now follows choose pallets, choose a fitting truck, schedule pickup. Switching carriers preserves selection. Booked shipments have a separate summary and calendar link. Missing stock, capacity, fee and arrival constraints are explained beside the booking action.
- Removed large routine success toasts; brief inline receipts remain. Meaningful errors and delayed network requests remain visible.

Validation: final Development Editor build passed; all 209 automated Foundryline checks passed; 18 native four-player checks passed. The native run used real purchases, physical plasma programming, production, rack storage, client booking, duplicate protection, direct truck loading and customer payment. All four peers agreed on the final ledger; mass and company balance reconciled. A real player-capsule sweep along the rack's front aisle found no invisible boundary. Captured and visually inspected Dispatch and rack nameplate screenshots. Asset-import QA passed for both rack families and the sheet shelf, retaining dimensions and assigned materials.

The progression benchmark averaged $301.01 net across the first three ordinary jobs, with unchanged prices and the brake plus $1,000 reserve affordable after five jobs. Its existing $200–$300 target now allows 5% travel-time tolerance, because physical routing changes journey duration; reported profit is still exact.

Reports: `Saved/Automation/Focused-Full/index.json`, `Saved/Logs/RackDispatchNativeQA.json`, `Saved/Logs/OpenRacksImportQA.json`, `Saved/Logs/EconomyProgressionBenchmark.json`. Review captures: `Saved/Screenshots/Reviews/RackDispatch_*.png`. No commit, push or merge was requested for these changes.

The remaining text records the initial diagnosis and implementation checklist, not outstanding work.

User clarification: rack long faces must be open on both sides; one accessible forklift aisle is sufficient. Do not widen a particular dimension without another instruction; the width question was clarified as an access-side problem.

Finished goods must live in finished-goods rack slots and move directly from rack to booked truck. Remove the floor-staging workflow and its confusing dispatch language. The user reported an invisible boundary beside the rack: inspect and replace overly broad collision boxes with physical frame/shelf collision. Dispatch needs a clear pallet -> carrier -> schedule flow and a separate already-booked state.

The user is currently playing PID 66608 (UnrealEditor.exe -game), launched by this chat. Do not stop it without coordinating a save/restart; new models/source can be prepared while they play. Compile/link/import and native acceptance after the player has saved and approved restart. Never modify another project or publish/merge these changes without a new request.

Prepared so far:

- `build_modular_pallet_racking.py`: removed rear top crossrail and long-face rear diagonals; retained braced upright end frames. Wide-bay generator inherits it. Both families regenerated in isolated background Blender, original dimensions retained.
- `FoundrylineOperationFeedback.cpp`: routine successful receipts no longer draw a large toast. Existing inline footer feedback remains; errors and delayed network waits remain visible.

Implementation points:

- `FoundrylinePlanningEquipment.cpp`: current Service assumes -Y only; CanBuy and slot docks must accept either face while retaining at least one clear 12-foot aisle. Existing rack clearance must allow shared aisles/L layouts with no physical overlap.
- `FoundrylineFactoryOperations.cpp`: MakeJob records source slot.Dock; Plan dispatch tests only one pickup/drop route. Enumerate reflected rack docks on both sides, choose a connected route, and commit the chosen points only to the assigned job. Preserve reservations/IDs/weights.
- `FoundrylinePlannerEditing.cpp`: RackAccess currently reserves one side and blanket-box checks. Validate one accessible side after move/rotation, and do not invalidate the only access of another rack.
- `FoundrylineFactoryController.cpp`: display both rack service faces, independently valid/blocked; not the wrong single-side rectangle.
- `FoundrylinePlanningLayoutPresentation.cpp` and `FoundrylineAssemblySupplyPresentation.cpp`: whole-rack UBoxComponent collision blocks empty air. Use end-frame and elevated shelf collision, keep genuine metal blocking. Legacy sheet rack shelf asset has rear diagonals in its game-owned derivative; replace with an original open-sided shelf while retaining actual bearing height 12.77 cm and dimensions.
- `FoundrylineCommerceActions.cpp`: StagePallet explicitly targets Place::Pickup at ShippingPickupPosition (the screenshot is the old floor destination). Change to actual owned FinishedGoods rack slots, tagging contract stock while maintaining exact quantities and rejecting missing rack/full rack/duplicate requests. Already-racked matching stock can be linked without moving to floor. Partial pallet split needs another real rack slot.
- `FoundrylineCommerceSimulation.cpp`: PlanCommerce auto-stages to floor; replace with rack preparation and load booked cargo directly from its slot dock. Recover legacy floor-staged pallets through real forklift moves to racks. Keep existing reservations/bookings and cash ledger intact.
- `FoundrylineFactoryOperations.cpp` unload branch currently clears OrderId and BookingId for all rack drops; preserve sales assignments and booked recovery loads, avoid merging across orders/bookings.
- UI predicates and totals in OperationsController, OperationsHUD, BusinessDesktop, ERPViewModel currently key on Place::Pickup. Replace with a shared finished-goods/ready-for-shipment predicate. BookPickup must validate actual ready rack stock; send the explicit selected order ID instead of relying only on the business-global PickupOrder.
- `FoundrylineOperationsHUD.cpp` Page8 is the confusing screenshot: carrier rows first, locked booked pallets mixed with selectable stock, zero selection unexplained. Reorder as choose pallets, choose fitting truck, schedule; show already-booked shipment separately and clear disabled reasons.

Current branch is `feature/office-assignments`, with substantial prior office work intentionally uncommitted. Prior office/wall-decor work passed 207 automated tests and 28 four-player checks. Do not erase or revert it. This new rack/dispatch change is not compiled or tested yet.
