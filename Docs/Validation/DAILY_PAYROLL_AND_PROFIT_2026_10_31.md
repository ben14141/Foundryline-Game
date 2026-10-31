# Daily payroll and profit validation — October 31, 2026

## Implemented policy

All hired employees currently earn $120 per game day, including idle and
unassigned staff. Empty forklifts and the player owner do not draw salary.
Hiring remains free for testing; salary starts when hired. Partial days are
prorated. Payroll accrues with simulation time and is billed once at midnight.
Paused simulation does not advance payroll. A cash shortfall becomes an unpaid
bill rather than silently cancelling wages.

Team and employee inspection show daily rates. Money shows payroll per day,
wages due tonight, and a separate Daily payroll journal entry. Total costs and
profit calculations include accrued wages before payday. Save/load preserves
that liability. Old saves acquire the new optional field prospectively, without
refunds or retrospective charges.

This is a real wage reduction: the former $15/hour rate cost $360 per employee
per 24-hour game day. The new rate is $120. Sale prices were left unchanged.

## Profit and progression results

The starter benchmark averages $635.14 per ordinary order after accrued payroll,
compared with about $461 previously. The press brake is affordable after three
ordinary orders while retaining $1,000 after wages owed.

Native PIE replay used the same earned checkpoint as the previous hourly test.
Earlier checkpoint history remains hourly; the following measured order deltas
use daily payroll. Profit includes material consumed, freight, operation costs,
and wages earned, with remaining steel valued at its purchase cost.

| Order | Previous hourly profit | Daily payroll profit | Game duration |
|---|---:|---:|---:|
| 30 mounting plates, ordinary | $27.88 | $220.70 | 385.5 minutes |
| 30 mounting plates, rush | $202.72 | $386.77 | 369.3 minutes |
| 30 formed brackets | $681.32 | $1,166.66 | 970.7 minutes |
| 30 bracket blanks, rush | $658.74 | $921.97 | 526.3 minutes |
| 75 formed brackets, sequential | $2,325.35 | $3,162.93 | 1,675.0 minutes |
| 75 formed brackets, pipelined | $2,671.88 | $3,278.16 | 1,212.7 minutes |

The controlled 75-part comparison restores the same pre-order save between
branches. Pipelining plasma and brake is approximately 27.6% faster and earns
$115.23 more under daily payroll. Those durations correspond to about 3m29s and
2m32s of simulation at 8x, plus player interaction. Accelerated QA execution
time is not a human playtime measurement. The rush plate order was also run
using actual 8x wall time; tight rush deadlines still need earlier pickup
planning. More profitable does not mean every rush contract is safe.

## Verification

- Successful editor builds, including the final UI changes.
- Full automation suite: 211 passed; subsequent focused Commerce suite: 36 passed.
- Four-player native payroll scenario: 25 checks passed, including an unassigned
  hire, one shared midnight deduction, synchronized cash and liability, and pause.
- Native progression replay: 216 checks passed, including purchases, installation,
  staffing, production, forklift transfers, carrier loading, payment, and the
  controlled sequential/pipelined comparison.
- Visually inspected the rendered Team and Money pages: daily rates, due tonight,
  and the midnight journal entry are visible.

Evidence is in Saved/Logs/DailyPayrollNetworkQA.json,
Saved/Logs/ProgressionTimingNativeQA.json, and the focused automation report.
Screenshots are Saved/Screenshots/Reviews/DailyPayroll_Team00000.png and
DailyPayroll_Money00000.png. The old native comparison is preserved as
Saved/Logs/HourlyPayrollProgressionBaseline.json.

## Limits

Hiring fees and building expansion remain free in the testing economy. Future
rent, financing, research, management salaries, and other progression costs are
not established by this result. This validates current profit and flow rather
than long-term balance. QA uses separate save slots and preserves normal player
saves. Changes are local on fix/starter-playtest-save-flow; no push or merge was
performed for this request.
