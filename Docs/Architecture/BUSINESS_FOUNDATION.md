# Independent business backend foundation

No business menus or new scene layout are added. The starter map still opens as
one shared co-op company. Trusted server code can move a member into a separate
business with an exclusive empty plot down the street.

## Data and authority

`Companies/FoundrylineBusinessLedger` owns company, plant and plot IDs; one-company
membership; owner/member invitations and revocation; and independent integer-cent
balances. The authored starter plot plus three free plots are reserved at 120 m
street intervals. These four initial plots are not a multiplayer player cap.
Claims are exclusive. Owners cannot accidentally abandon their company or remove
themselves. Removed members retain an unassigned identity record and must receive
another invitation. Failed operations and invalid snapshot restores are atomic.

`Companies/FoundrylineBusinessRegistry` is one always-relevant server actor per
world. It replicates company names/IDs/plot IDs and active player membership;
private account keys, invitations and money are not broadcast. There is no client
RPC that accepts a company identity, permission flag, money change or plot owner.
Current joins retain automatic shared co-op enrolment for compatibility.

Default identities are random, server-issued session identities. The registry
provides `RegisterVerifiedAccount` for a future trusted authentication adapter,
called before normal session enrolment. That hook must receive a validated server
identity, never a client-provided account claim. An authenticated existing key
resumes its recorded company. Online login/account verification and automatic
reconnect ownership are not integrated. Anonymous session identities do not
establish durable ownership across reconnects or server restarts.

## Backend entry points

- `CreateBusiness(Player, Name, PlotId, Error)` claims a free plot, creates distinct
  company/plant IDs and switches that member atomically. It starts no equipment,
  employees, construction, automatic stock, business orders or money grants.
- `Invite(Owner, Guest, Error)`, `Accept(Guest, Company, Error)` and
  `RemoveMember(Owner, Guest, Error)` implement owner-controlled co-op membership.
- `ConstructionContext(Player, Out)` derives the trusted identity from the actual
  server connection. The existing placement validator checks every object and
  region against that company/plant, including neighbouring plots and public roads.
- `CanOperate(Player, Company, Plant)` protects production/inventory and equipment
  control. Factory commands resolve the player's own plant instead of the first
  plant actor in the world. Membership changes release old vehicle controls.
- `AdjustBalance(Company, DeltaCents, Error)` is trusted server-only plumbing for
  future transactions, with overdraft/overflow checks. It is not a sales economy.

Factory plants, plasma tables, press brakes and manual material/freight vehicles
carry replicated company/plant identities. Equipment discovery matches both IDs,
so one plant cannot animate another company's machines. Existing per-plant
production/inventory/job records remain scoped by those identities. New claimed
businesses have no live plant actor until a later construction/equipment flow
creates and configures one. Multiple live plant actors are used in the network
test fixture to exercise isolation, not added to the startup scene.

## Persistence boundary

The versioned `UFoundrylineBusinessSave` snapshot serializes ownership, membership,
plots, invitations and balances. `CreateSnapshot` is server-only;
`RestoreBeforeWorldBinding` accepts validated snapshots before players/world
bindings exist. Binding then atomically adds claimed regions without rewriting
existing owners. Corrupt identities, duplicate records, forged plot geometry and
invalid balances are rejected.

A combined persistent world save/load coordinator is still required to compose
business records, all plant simulations and all construction in one transaction.
Until then, legacy whole-property and single-plant save/load operations are blocked
when more than one business exists, preventing them from deleting a neighbour's
property or reverting another company's inventory. Normal single-company co-op
saves keep their existing behavior. There is no automatic multi-business disk save
or load in this milestone.

## Validation

See BusinessFoundationBuild.log, BusinessFoundationTests.log and
BusinessFoundationNetworkQA.json under Saved/Logs. The network fixture uses four
players, a shared starter and two independent businesses, real construction and
plant commands, invitations/removal, separate cash and a fifth player joining.
No assets, visual UI, published remote, global engine settings or other game
projects are changed by this backend work.

Final backend evidence: BusinessFoundationBuild.log succeeded; the complete suite
passed 81/81 tests. After the reserved-plot namespace fix, the three company cases
were rerun in BusinessFoundationCompanyTests.log and all passed, including the
protected public-street region regression. BusinessFoundationNetworkQA.json
completed with all 27 checks true and no errors: four players, three companies,
owned construction, neighbour denial, separate inventory/cash, vehicle denial,
real client operation of its own plant, membership changes and a fifth late join.
Native default co-op startup is checked separately in BusinessFoundationStartupQA.json.
Early failed network fixtures were corrected before acceptance; they revealed a
reserved region ID collision with the town street and local test identity handling.

Final normal rendered startup passed both checks: one accessible co-op company and the untouched paused starter with ten raw sheets. Unreal remains open in the normal standalone session, with no new business UI. Editor multiplayer test settings were restored.
