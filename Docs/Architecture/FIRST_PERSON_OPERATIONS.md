# First-person operations and Property View

The cut-parts starter opens in first person beside the office computer. The normal
HUD is a compact light cash/time panel, a small reticle, and contextual E prompts.
Office, tablet and machine pop-ups use light panels with Foundryline orange and grey.

Press E at the office computer to open Foundryline ERP. Its Overview links to
Sales, Purchasing, Production, Inventory, Dispatch, Accounts, equipment and driver
priorities. See BUSINESS_ERP_UI.md and ERP_PLAYTEST_ACCEPTANCE.md for presentation
and current acceptance evidence. The Property View action on that desktop
opens the top-down construction view. The server checks company/plant access and
that the requesting character is within 270 cm of its office terminal. The loading
tablet cannot grant this view. F cannot freely enter it from first person.

Return to Office [F] exits Property View and restores the character's saved desk
position. Roofs are visible on foot; each Property View session starts cut away.
View authorization is per player and is revoked if their company/plant changes.
Other cooperative players can remain on foot while someone uses Property View.

Machine screens open with E near the control screen. Select a fixed part recipe
and type a batch quantity. Starting a valid batch resumes the live plant; machine
pause and plant pause remain separate controls. The loading tablet assigns accepted
orders, lists staged pallets, selects carrier capacity, books pickups and shows the
shared-bay calendar. Pallet inspection uses look direction so raised rack pallets
can be selected from the floor without always selecting the lowest shelf.

Validation evidence is recorded separately in FirstOrderFullAcceptance.log,
CommerceNetworkQA.json and CommerceNativeQA.json under Saved/Logs. These are local
editor checks, not hosted multiplayer, packaged-build or FPS acceptance.
