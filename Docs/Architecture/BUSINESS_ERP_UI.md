# Office ERP presentation

The office computer opens Foundryline ERP in a near-fullscreen light workspace.
At desktop aspect ratios its frame uses almost the full viewport; narrower
viewports scale the logical workspace to preserve legible controls and columns.
The opening Overview links to the next useful actions and uses live company data.

Modules are Overview, Sales & Customers (Marketplace, Customer Mail and Accepted
Orders), Purchasing, Production, Inventory, Dispatch, Accounts and Team & Logistics.
Production links to actual installed machine controls and the equipment store.
Inventory lists physical pallet quantities, weights, locations and reservations.
Dispatch and the loading tablet retain exact pallet selection, carrier limits,
booking fees and the shared loading-bay calendar.

Customer, supplier and carrier identities use eleven stable vector marks drawn
by the game-owned Canvas UI. These require no external image download or vendor
asset changes. A common window header, application rail, record numbers, selected
record panel and connection/workday footer make the modules one connected system.
The mail list shows seven records per page beside a separate reading pane, with no
hidden clickable records beneath it. Repeated shortcuts have location-unique
hitbox IDs, decoded to the same existing controller action.

This remains presentation over authoritative commerce state. It adds no free
inventory, fabricated revenue, placeholder accounting totals or unsupported
enterprise controls. Revenue reflects delivered goods; total spending includes
equipment. Completed-order counts describe retained history, not lifetime totals.
Freight delivery times remain estimates and can be delayed by bay queues.

First-person startup and the office-only Property View gate are unchanged. Each
player sees their company's private operations; navigation and drafts are local,
while purchases, orders and bookings use the existing validated server commands.

Current verification evidence is recorded in the acceptance report and separate
build, automation, live multiplayer, rendered workflow and manual UI artifacts.

## Desktop software revision

The office desktop now has an OS-style title bar, application rail and live taskbar,
a company workspace header and measured Roboto regular/bold typography. Sortable,
paginated tables retain selection while their order changes. Summary, Document and
Activity tabs link the same actual record. Purchasing has a draft and a separate
purchase/receipt register; Accounts has a journal and sales statements/receipts.

The Overview includes actual offered demand by part, next scheduled bookings and
condition-based attention links. Its signed liquidity chart reconstructs retained
journal groups from cash less outstanding costs. Negative values extend below the
zero line; this is not an hourly bank-history chart or an operating-profit figure.
Recorded activity is newest first. Dates appear only where the model stores them;
purchase/receipt creation timestamps are not invented. Buyer offers are explicitly
unaccepted. Payment records are actual retained ledger receipts, not newly issued
invoices, and the default sales statement shows aggregate paid amounts.

Sort, document, workspace and linked-record navigation are local presentation only.
They cannot spend money, mutate inventory or bypass server validation. Four ERP
view-model/controller automation cases cover actual documents, missing dates,
retained journal debt, upfront versus delivered receipts and immutable navigation.
The runtime Canvas UFont cache selector is explicitly retained alongside Slate
font info, because UE5.8 otherwise silently omits Canvas text.
