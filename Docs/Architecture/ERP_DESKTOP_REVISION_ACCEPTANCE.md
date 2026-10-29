# Desktop ERP revision acceptance - 28 October 2026

This revision replaces the previous sparse ERP cards with a connected office desktop:
application rail, taskbar, actual company workspace, stronger type, dense sortable
record tables, separate inbox and reading pane, Summary/Document/Activity tabs,
purchase registers and customer statements/payment receipts. Prominent stable
company marks appear in partner records and documents. The live Overview adds
buyer demand, a signed retained-journal chart, upcoming arrivals and attention links.

## Evidence

- C++ editor build: succeeded, `Saved/Logs/ERPDesktopV2Build.log`.
- Unreal automation: 97/97 passed, `ERPDesktopV2FullAcceptance.log`.
- Four new ERP cases exercise actual documents and missing timestamps, signed
  cash-less-debt reconstruction, upfront/delivered receipts, and local navigation
  boundaries without commerce mutations.
- Native workflow and actual mouse verification: final evidence added below.

The previous accepted baseline is preserved separately in
`ERP_PLAYTEST_ACCEPTANCE.md` (93 tests, 39 live multiplayer checks and native world
acceptance). Those older results are not described as new runs of this revision.

## Data semantics

No fake company metrics, invoice issuance, timestamps or cash history were added.
Offers are unaccepted buyer documents. Statements aggregate paid amounts; their
receipt tab shows actual retained individual payments. The graph groups retained
journal movements and includes outstanding cost debt, with a true signed zero
baseline. Activity is newest first. Sort, tab and linked-record actions are local;
all business mutations retain the existing server validation.

## Startup recovery

The first final native launch encountered a D3D12 GPU page fault at frame 5, before
PIE began. It was dismissed and the same build restarted without changing renderer
or global settings. The retry result is recorded with the native evidence.

## Completed native pass

The retry completed 49/49 scripted native checks without errors in
`ERPDesktopV2NativeRetry.log` / `CommerceNativeQA.json`. This includes twenty new
ERP sort/tab/workspace and populated-document checks, plus the full first-person
order loop and all eleven office pages. One sheet was actually bought, received
and racked; forty brackets were cut, staged and inspected; freight was booked,
loaded and delivered; the order received exactly $440 once. Final tested cash
$4,112.08 reconciles $4,000 + $440 - $327.92 spending. The session reset fresh.

Read-only observers passed all five backend checks over 957 samples and all six
native equipment checks over 611 samples. Ten camera-only world views completed
with materials and active lights present. The final C++ rebuild only shortens user-facing document labels; no behavior changed.
`ERPDesktopV2FinalBuild.log` confirms the final build succeeded.

Actual rendered review artifacts are in `Saved/Screenshots/Reviews`:

- `FirstOrder_V2_BuyerQuotation00000.png`
- `FirstOrder_V2_CustomerInbox00000.png`
- `FirstOrder_V2_PurchaseOrderDocument00000.png`
- `FirstOrder_V2_CompletedSalesStatement00000.png`
- `FirstOrder_V2_CustomerPaymentReceipts00000.png`
- `FirstOrder_V2_CustomerPaymentHistory00000.png`
- `FirstOrder_V2_CompletedDashboard00000.png`

The offer, supplier purchase, completed statement, payment receipt and dashboard
were visually reviewed; amounts, dates and recent activity agree with the model.
The preceding sparse UI can be compared in `FirstOrder_ERP_App900003.png`.

## Final physical-input acceptance and leave-open state

Actual Sky mouse and E/Escape keyboard checks passed 14/14, recorded in
`ERPDesktopV2ManualQA.json` with read-only native state transitions. They cover
ascending/descending sales value sorting, selecting another buyer, Document and
Activity tabs, Customer Mail, Purchasing, purchase register, Accounts, statements,
taskbar navigation, Escape returning to walk and physical E reopening the office.
Cash remained $4,000, spending/revenue zero and stock empty throughout navigation.
The final screen is closed in first person, paused beside the office computer.

Final GUI process 40808 remained healthy through this pass; the earlier startup
GPU crash did not recur in either retry or final launch. A stale crash-report
window was closed without sending. The oversized restored editor window was
maximized through its native system menu so the complete desktop fits the display.
Final overview capture: `ERPDesktopV2_FinalOverview.jpg`. Normal saves and other
projects were untouched; no merge or push occurred. This remains local editor
acceptance, not a packaged/network-hosted or FPS guarantee.
