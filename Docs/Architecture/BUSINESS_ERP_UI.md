# Workshop computer menu

## Current game-facing layout (30 October 2026)

The office computer now follows the build catalogue's presentation: a centered large popup, charcoal header and navigation rail, off-white workspace, graphite text and foundry-orange selected categories and primary actions. The title is Workshop. Company name, date, cash and Close are the only header controls. Time speed and Pause/Resume sit together in the rail. Planner is a large, office-gated action at the bottom.

Home has one Next Move card driven by the existing read-only starter guide, followed by six large shortcuts: Orders, Buy steel, Production, Stock & trucks, Team and Money. The former six-step checklist, eight statistic tiles, upcoming-trucks panel, navigation subtitles and repeated global tablet/plasma shortcuts are removed. Their underlying information/functions remain in the corresponding section. Production and freight controls remain available from Production and Stock & trucks.

Home uses the same transparent isometric asset previews as construction where relevant, with simple orange pictograms for people, orders and money. The guide preview follows cutting versus welding. Every shortcut uses the existing operations navigation; purchases, hiring, assignments, freight and orders still go through the existing company-scoped validated commands. Navigation and hover remain local to each player.

Money now shows four primary totals, with receivables, advances and payroll on a secondary line instead of seven competing tiles. Successful commands use a brief footer message rather than a large overlay. Server refusals remain visible in the error footer. Hover help is retained, with orange hover/press feedback throughout. Customer/supplier company marks and semantic warning/success colors retain their meanings.

The physical desk monitors use matching charcoal, grey, off-white and orange framing and plain Workshop language. The central monitor shows a simple read-only Home preview with an E prompt; opening it uses the same first-person proximity/ownership gate as before. Side screens keep live production, freight and employee summaries.

Implementation: `Private/UI/FoundrylineBusinessDesktop.cpp` and `FoundrylineOperationsTerminal.cpp`. No inventory, currency, recipes, worker permissions or server authority rules are changed by the redesign.

## Historical layouts and evidence

The following sections describe previous ERP iterations and their evidence. The Workshop layout above supersedes their presentation details.

### Original office ERP presentation

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


## Simpler office computer (28 October 2026, branch ui/simpler-office-computer)

The office desktop was redesigned for first-time players. Seven plain sections
(Home, Orders, Buy steel, Production, Stock & trucks, Team, Money) replace the
eight-module rail, dock and document tabs. Home shows one "What to do next" card
that follows the starter loop (accept order -> buy steel -> staff plasma & forklift
-> program plasma -> book freight -> get paid), names exactly what is missing with a
Fix button, a six-step checklist and eight at-a-glance tiles.

`FFoundrylineERPViewModel::Guide` is a pure, read-only function of one plant's
snapshot, so every co-op player sees identical guidance. It counts free finished
stock and parts already cutting before recommending more cutting or steel.

Every number and button has hover help. Clickable items get a blue hover outline
(orange is reserved for selection and primary actions), a hand cursor and a click
flash. A green DONE banner appears only after the server accepts a command, or
after the replicated snapshot shows a hire/assignment/time change; refusals show a
red NOT DONE banner. Guided shortcuts (BuySheets<n>, AssignOrder<id>,
ProgramOrder<id>) only pre-fill local state or call the existing validated commands.

Evidence: `Saved/Logs/UIRedesignBuild3.log`, 19/19 tests in
`Saved/Logs/UIRedesignAutomationFinal.log` (new: StarterGuideFollowsLoop,
GuideShortcutsStayLocal), and 38 before / 41 after native starter-loop captures in
`Saved/UIRedesign` produced by `Tools/Unreal/review_office_desktop_loop.py`.
The loading tablet keeps its previous layout (it gained only the DONE banner).


## Workshop menu verification (30 October 2026)

- Final editor Win64 Development build: `Saved/Logs/WorkshopComputerReleaseBuild.log`.
- Focused commerce automation: 13 passed, zero failures, about 16 seconds (`WorkshopMenuCommerceCheck.log`, `Focused-Commerce.log`). These exercise business state/view models; rendered input is separate.
- Four-player native mouse review: all 13 checks passed with zero errors (`WorkshopMenuInputQA.json`, `WorkshopMenuInputReview.log`). Covered Home cards, all main navigation sections, independent peer navigation, actual Planner click, matching build catalogue access, peer screens staying open, restored first-person input/roofs and no cash spent while browsing.
- Native review screenshots include Home, Team, Orders, Production, Steel, Money, Freight, Planning and the physical desk. Final single-player captures follow the last label/badge cleanup (`Workshop_Computer_Home_Final`, `Workshop_Computer_Desk_Final`).
- The older starter-loop capture was exercised through order acceptance, steel booking and batch setup. Its production/receiving wait did not finish within this UI review; it is explicitly recorded as incomplete in `Saved/UIRedesign/WorkshopMenuFinal/capture_report.json`. It is not end-to-end shipping/payment acceptance. The wrapper now bounds simulation waits and retains state for diagnosis instead of running unattended for fifteen minutes.
- Review changes were unsaved. Normal editor play settings were restored before the final single-player capture. No multiplayer player-limit or performance claim is made.

For fast follow-up UI changes, use `review_workshop_menu_input.py` for actual mouse navigation/co-op independence, or `review_workshop_menu_final.py` for two quick rendered captures. `review_workshop_menu_flow.py` is the broader simulation/capture path and should be treated separately from UI acceptance. Updated thumbnail rendering uses isolated Blender and complete model exports, excluding studio floors; the steel-sheet illustration is a menu asset, not new gameplay inventory.


### Illustrated game-menu polish

Home now uses a charcoal Next Job panel with a six-segment progress strip derived from the existing guide's actual completed steps. Every section card has a larger transparent isometric illustration: orders clipboard, steel sheets, plasma table, forklift, employee badge/helmet and money box. The three new illustrative props are original menu artwork rendered by `Tools/Blender/render_workshop_menu_icons.py`; they introduce no inventory or purchasable equipment. The palette remains logo orange, charcoal and off-white.

Card descriptions move to hover help; titles and one live figure remain visible beneath the illustration. Hover transitions darken the label area, emphasize the orange border and slightly lift the image without shifting click bounds. The extra issue shortcut is shown only when a guide order exists and uses a more specific label where possible. Selected section tabs have orange fills. Page introductions are shorter, with the original explanation retained as hover help. Production's pickup summary is shortened to avoid clipping.

Illustrated-menu verification: Win64 Development build passed (WorkshopMenuGamePolishBuild.log). All 13 four-player native input checks passed again with zero errors (WorkshopMenuGamePolishReview.log / WorkshopMenuInputQA.json), including the resized Home cards, local peer navigation, planner handoff and restored first-person controls. New illustration alpha/import checks passed. Inspected Workshop_Menu_Home00001.png. Normal single-player editor preferences were restored after the review exited.


### Illustrated side navigation

The computer rail is slightly wider to accommodate transparent isometric thumbnails for Home, Orders, Buy steel, Production, Stock & trucks, Team and Money. Labels wrap rather than clip when a count badge needs room. Selected rows use a full orange background; idle rows retain charcoal framing, a light thumbnail well, and animated graphite/orange hover. Counts are presented in small pill-shaped rectangular badges; freight counts open bookings and Team counts actual employees.

Time speed controls use larger evenly spaced buttons, with the current speed orange. Pause/Resume remains the same authoritative business command. The bottom Planner/Office action has a matching building preview and a short readable label. The central content panel, its existing operations and business ownership validation are unchanged; the wider rail only changes layout and local presentation. Native review coordinates are derived from the new rail width.

Illustrated-side-menu verification: final Win64 Development build passed (WorkshopSideMenuFinalBuild.log). All 13 four-player native input checks passed with zero errors after the rail-width change (WorkshopSideMenuReview.log / WorkshopMenuInputQA.json). Inspected Workshop_Menu_Home00002.png for label fit, selected state, thumbnail placement and grouped time controls. Normal editor play preferences restored after review exit.


### Shared button and surface quality

`Private/UI/FoundrylineUISurface.h` provides bounded Canvas geometry for softly rounded panels, a thin antialiased fringe, a restrained vertical color gradient, edge strokes and layered soft shadows. It uses the engine white texture rather than external image assets or materials. Both the computer and build catalogue use this shared surface treatment.

Computer controls distinguish primary, secondary, selected/tab and disabled states. Hover slightly raises/highlights the face; the existing local click flash now produces a short press depression and darker face. Keyboard hints, action names and hit bounds remain unchanged. Static information cards, Home cards, navigation rows, status chips and progress bars receive consistent edges. Catalogue cards, category rows and controls receive the same skin and local press feedback.

The style does not change prices, command dispatch, business ownership or replicated state. Rounded corner/shadow pixels are cosmetic; hit testing still uses each control's original rectangle. This revision does not establish a frame-rate gain or performance acceptance.

Surface-quality verification: Win64 Development build passed (UIQualityFinalBuild.log). All 13 four-player native input checks passed with zero errors (UIQualityInputReview.log / WorkshopMenuInputQA.json). Inspected Home, Team and the planning toolbar with the shared skin; card/control bounds and planner/first-person transitions remained functional. Normal editor play preferences restored after review exit.


## Visual contracts and company identities

The Orders section is presented as Contracts. Available and active contracts use larger product cards with a rendered part/assembly cutout, actual quantity, customer identity, total payout, payment schedule, deadline and status. The selected contract shows a larger preview, per-unit value, material/process summary, workshop readiness and the existing accepted-order fulfillment controls. The accepted contract opens only after a successful server acknowledgement, and only if the player is still viewing that offer; browsing elsewhere or a rejection does not force a navigation change.

Sixteen transparent product cutouts are rendered from game-owned exported model geometry by `Tools/Blender/render_contract_products.py`; steel sheets reuse their existing illustration. Coverage includes the four starter plasma parts, bent brackets, channels, intermediate tanker components and the complete pickup headache rack. UI previews are illustrative, not physical placement scale. The frame-section preview uses an actual rack tube member; the light-kit preview uses its real lamp component rather than the older generic kit-crate placeholder.

Fourteen original company marks cover existing part customers, the steel supplier, freight companies and all three assembly customers. Source SVG and matching antialiased PNG assets are built with `Tools/UI/build_company_marks.py`, then imported under `/Game/Foundryline/UI/Companies` with `Tools/Unreal/import_contract_art.py`. These are fictional game-owned symbols, not third-party logos. Unknown/custom company names keep a neutral initial fallback instead of receiving another company's identity. Logo colors belong to those companies; the surrounding UI retains Foundryline's palette.

Other pages use the same art: steel purchasing has a sheet preview, machine/station status panels and equipment offers show actual equipment, and pallet inventory rows show their contained part. Existing customer/supplier/carrier marks used in finance and freight resolve to the new identity assets. Prices, capacities and server validation remain authoritative and unchanged.

### Material estimation before installation

The read-only starter guide previously used only installed machine programs to estimate steel demand. An empty shop therefore displayed zero demand for an accepted cutting contract. It now falls back to standard recipe data when no installed program exists, while keeping the existing finished-stock, reserved work and incoming-delivery accounting. Contract detail estimates also use standard recipe data. The new `Foundryline.UI.ERP.MaterialPlanningBeforeMachineInstallation` regression checks real material demand before installation, stock reducing demand, incoming sheets preventing duplicate recommendations, and snapshot immutability. No machine, steel or money is created by the estimate.

### Review procedure

`Tools/Unreal/review_contracts_ui.py` uses four unsaved PIE peers, actual product-card and Accept-button clicks, and the existing validated workstation purchase path. It checks local navigation, shared accepted-contract state, successful acknowledgement navigation and normal assembly-offer unlocking. Assembly selection uses the real paginated offer list. Captures cover cut-part offers, active contracts, headache-rack offers, steel purchasing, production and equipment. Purchases and accepted contracts exist only in the review session and are never saved to the authored map.

Visual-contract verification: Win64 Development release build passed (ContractsReleaseBuild.log). Seven ERP/view-model tests passed, including the new pre-installation material estimate regression (Saved/Automation/ContractsERP/index.json); 13 focused commerce checks also passed (ContractsCommerceCheck.log). All 13 four-player native contract checks passed with zero errors in 65.58 seconds (ContractsReleaseReview.log / ContractsUIQA.json): actual product-card and Accept clicks, acknowledgement-only active-contract navigation, shared acceptance, real replicated workstation purchase, normal assembly-offer unlocking and actual paginated assembly selection. Inspected the final cut-part, active-contract, assembly, supplier, production and equipment captures. Fourteen company marks and sixteen product previews passed alpha/import checks. Review changes were unsaved; normal editor play preferences restored after review exit. This does not claim a new performance result or an end-to-end live freight run.
