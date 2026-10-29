# Read-only ERP records and presentation

`UI/FoundrylineERPViewModel` projects the existing company commerce state into
software-oriented documents, activity, summaries and journal graph groups. It owns
no inventory, money, order lifecycle, save state or networking commands.

## Company identity

`FoundrylineBusinessRegistry::GetCompanyName` reads the existing public replicated
company directory, with an authoritative-ledger fallback only on the server.
A client without a synchronized directory gets no invented company name. Public
company names were already replicated; the accessor exposes no financial data.

## Documents and activity

- Sales documents represent accepted/completed orders. Offers remain offers.
- Purchase records and freight documents refer to actual booking IDs. The initial
  purchase amount uses the game's fixed $1/lb steel contract plus the stored original
  freight fee; rebooking costs are excluded from that initial amount.
- Goods receipt documents appear only once the incoming cargo has actually spawned
  at the bay. They report the retained manifest's received quantity and steel mass.
  They do not claim all material has already reached a rack.
- Customer payment/advance receipts come from actual retained money journal records,
  with their recorded timestamps and source order references. Completing an upfront
  order does not invent another payment.
- Activity uses actual order acceptance, journal, berth, departure and completed
  delivery timestamps. An in-transit estimated delivery is not an activity event.

There is no stored purchase booking creation time or material receipt timestamp.
Those documents explicitly expose `bHasRecordedAt=false`; a scheduled arrival is
never substituted for an actual receipt date. Old order records can leave bounded
history while a payment remains in the journal; its original reference is retained
but no customer identity is fabricated. These records are not invented tax invoices.

Document keys and journal indices are presentation references to current retained
records, not new persistent invoice IDs. Renderer-local tabs/sorting/selection do not
submit RPCs. Actual accept, buy, program and book actions keep the existing validated
owning-controller command paths.

## Ledger graph semantics

The current money journal is bounded to 32 records, and consecutive power/wage costs
may be coalesced. It records full economic movements, including costs accrued when
cash is insufficient and payments first settling outstanding costs. It therefore
cannot support a fabricated per-minute bank-balance history.

The view model reconstructs **cash less outstanding costs** for the retained journal.
It starts from the known current net position and subtracts retained movements to
obtain the earlier retained balance. Each actual record then advances that balance.
Graph bins group adjacent retained records and preserve their actual first/last
record times. There are no synthetic daily/monthly observations, nor a fake timestamp
for history that has already been trimmed. Credits include working capital and
customer receipts; they are not labeled sales revenue. Earned revenue remains the
simulation's delivered-goods value, separate from upfront advances.

## Verification

The ERP view-model tests cover actual purchase/receipt creation, absent timestamps,
upfront payment and delivery timing through a full one-part shipment, debt-aware
retained graph balances, bounded bins, and no fabricated synchronized state. A
controller test covers sort direction/bounds, local tab reset/link behavior, real
company-name lookup, and unchanged authoritative model/snapshot/RPC sequence.
All four ERP cases passed in the complete 97/97 automation run recorded in
`Saved/Logs/ERPDesktopV2FullAcceptance.log` (process exit 0). The populated document/receipt/workspace native run passed 49/49 checks in
`CommerceNativeQA.json`; the read-only observer passed all five checks across
957 samples. Native purchase, completed sales statement, payment receipt and
financial overview pixels were reviewed against the actual amounts and game times.
Only concise user-facing label text changed after that full flow; final compilation
and desktop smoke evidence are recorded by UI acceptance.
