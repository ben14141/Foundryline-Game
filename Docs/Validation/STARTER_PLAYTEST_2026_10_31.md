# Starter business playtest — October 31, 2026

> Payroll and profit figures below are historical. The later daily-payroll change
> and current validation are documented in [Daily payroll and profit](DAILY_PAYROLL_AND_PROFIT_2026_10_31.md).

Scope: the current local Foundryline checkout, starting from main at
`d32f15a17b541adda782da69a9db34e19bfa8cdc`. Changes are on
`fix/starter-playtest-save-flow`; this playtest does not push or merge them.

## Fixes found during play

- Continue and Load Game were permanently disabled. They now recognise the
  combined starter save and reopen it through the authoritative factory loader.
  A rejected restore returns to the menu and preserves the saved file.
- The planner already saved factory state as well as property, but wrote a
  redundant property-only save first. Factory sessions now use one combined save
  and report success only after the server confirms it. Construction-only scenes
  keep their property save.
- The factory Load command attempted a second company-cash transaction after
  LoadFactory had already restored cash and property atomically. It now skips
  that duplicate transaction.
- Buy Steel incorrectly described incoming sheets unloading into finished-goods
  racking. Guidance now describes raw-steel storage, forklift restocking of the
  plasma input rack, and automatic consumption by the table.
- Remaining stock/preparation labels now refer to finished-goods racks rather
  than the removed floor-staging flow.

## Build and automated tests

- FoundrylineEditor Win64 Development built successfully.
- All 209 Unreal automation cases passed, including the added assertions for
  readable starter saves, missing saves, wrong scenarios, and missing property.
- Existing persistence checks cover invalid-property rejection, account rollback,
  and restoring purchased equipment and navigation blockers together.

Build/test warnings remain in existing project and engine code. A passing test
run is not a warning-free build.

## Actual UI checks

A separate `-UserDir` under `Saved/PlaytestProfiles/2026-10-31` protected the normal
player save slots. Through the actual game window:

1. Started New Game in first person and used E on the office computer.
2. Accepted the 40-bracket-blank contract at $1,200.
3. Used the recommended one-sheet quantity and paid exactly $280.24 including
   delivery; the UI showed the receipt and incoming truck.
4. Advanced time, received the sheet, and observed 255 lb in inventory.
5. Checked office production guidance, planner access and the build catalogue.
   Plasma/brake/roller have model thumbnails; unpurchased machines explain that
   they must first be bought at the office.
6. Saved at Day 1 17:22 with $3,436.22, an active contract, two employees and a
   steel pallet. Restarted, clicked Continue, and verified those values restored
   together. The player returned to first person at the office computer.

## Native runtime acceptance

These runs use fresh, unsaved PIE sessions and normal validated commands. Fixture
positioning near a terminal is distinct from manual walking. Simulation stepping
uses real plant ticks and account synchronisation; it does not grant money or stock.

| Run | Result | Evidence |
| --- | --- | --- |
| Four real orders and earned press-brake purchase | 130 checks passed | `Saved/Logs/StarterBusinessNativeQA.json` |
| Four-player rack-to-truck | 18 checks passed | `Saved/Logs/RackDispatchNativeQA.json` |
| Four-player office assignments | 28 checks passed | `Saved/Logs/OfficeAssignmentsNativeQA.json` |
| Four-player turned office stairs | 28 checks passed | `Saved/Logs/OfficeStoreysNativeQA.json` |
| Four-player straight office stairs | 29 checks passed | `Saved/Logs/OfficeStoreysStraightNativeQA.json` |

Rack tests cover client acceptance/purchases, no phantom unmade stock, real cutting,
exact quantities on rack shelves, shared pallet locks, a single booking fee,
duplicate-booking rejection, direct rack-to-truck loading, payment and ledger
agreement. The front aisle also passes an actual player-capsule sweep.

Office tests cover president/engineer assignments, free testing furniture,
replicated door names/titles, manual interior glass, wall-decoration clearance,
closed/open door collision, E interaction and visibly changing typing poses.
Both stair variants use actual pawn movement up and down, verify authoritative
arrival without falling through floors, and protect occupied stairs from edits.

### Business progression result

The final fresh playthrough completed 40 bracket blanks, 60 mounting plates,
another 60 mounting plates, then the available 50-bracket-blank contract. It used
real offers, physical machine-terminal commands, paid whole steel sheets, assigned
operators/drivers, finished-goods racks and paid carrier bookings.

| Money movement | Amount |
| --- | ---: |
| Starting cash | $4,000.00 |
| Customer revenue from four delivered orders | $4,320.00 |
| Operating expenditure before the upgrade | $2,236.67 |
| Wages included in operating expenditure | $1,269.95 |
| Cash after four orders | $6,083.33 |
| Press-brake purchase | $4,500.00 |
| Cash remaining after purchase | $1,583.33 |

Steel mass discrepancy was zero; the final authoritative ledger was valid and
company cash matched the plant. The brake appeared as owned equipment without
being installed or creating racks before planner placement. This run verifies
earning and buying the upgrade; it does not claim a full live formed-parts batch.

The playthrough also checks insufficient-funds rejection, unmade parts being
unavailable for shipping, an actual running wait for missing steel (time and wages
advance without creating parts), empty-manifest rejection, duplicate-booking
rejection without fees, and new work remaining available after deliveries.

The initial fourth-job fixture assumed another 60-plate non-upfront offer would
exist. It was changed to the actual available 50-bracket offer; no customer orders
or rewards were injected.

The first office run exposed a fixed-delay test race. The room-creation fixture
now waits for replicated room records and matching property digests before the
next edit. It passed on rerun. Stair acceptance no longer reimports model previews
by default; asset refresh requires an explicit flag.

## Repeatable commands

Run one native editor-review runner at a time because they temporarily configure
PIE player settings. Each runner restores the original settings and stops only its
own process on timeout.

```powershell
Tools/Validation/Invoke-FoundrylineCheck.ps1 -Profile Full
Tools/Unreal/run_starter_business_review.ps1
Tools/Unreal/run_rack_dispatch_review.ps1
Tools/Unreal/run_office_assignments_review.ps1
Tools/Unreal/run_office_storeys_review.ps1
Tools/Unreal/run_office_storeys_review.ps1 -Straight
```

Screenshots are under `Saved/Screenshots/Reviews`. JSON reports are local generated
evidence, not source assets.

## Limits of this acceptance

- This is editor/development runtime acceptance, not a packaged-build, WAN, soak,
  production-capacity or 60-FPS certification.
- Four-player shared-business checks do not establish a maximum player count.
- The existing backend explicitly rejects saving a world with multiple independent
  businesses until combined-world persistence is implemented. This limitation was
  not removed by enabling Continue for the supported starter save.
- Continue/Load Game currently reopen the single starter slot; there is no
  multi-slot browser.
