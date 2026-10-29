# Employee foundation acceptance

Temporary articulated employees now consume one company/plant-scoped roster.
Installed machine operators are separate saved records; existing welders and
forklift drivers retain their existing IDs and jobs. There is no duplicate second
simulation for the presentation. Operators walk from a waiting point to their
workstation, face the controls, run real machine cycles only after arrival, and
return when work is finished. Pausing stops movement and work poses. Forklift
routes avoid staff footprints, with a close-range stop on job and return trips.

The office Team page shows the requesting business's roster, current assignment,
activity and waiting reason. It retains forklift priorities in a separate tab.
Assignment commands use existing server RPCs with an explicit target plant;
company membership/permissions and assignment targets are checked on the server.
Co-op players share a roster. Independent businesses retain separate identities.

## Current evidence

- `Saved/Logs/EmployeeBuild.log`: C++ editor build succeeded.
- `EmployeeFullFinal.log`: all 103 Unreal automation cases passed.
- New cases cover staffed production and unassignment, actual mid-walk memory
  save/restore and schema migration, blocked/restored paths and forklift detours,
  company-scoped commands, grounded walking poses, stopped poses, seated collision
  and bounded presentation components. The full rotated tanker production case
  also completes with employee safety enabled.
- Live multiplayer and native visual evidence are recorded below when complete.

## Presentation and model replacement

The current person is a temporary 180 cm articulated procedural model. Gait is
sampled from actual interpolated travel distance, and work poses use authoritative
cycle progress. Seated drivers attach to the animated forklift root at the measured
seat offset. Standing employees have Pawn-only query capsules, including on a
server without rendering. The optional EmployeeAppearance data asset provides a
skeletal mesh and in-place activity clips for later character models; no Fab
purchase or final character import was performed.

This is an operator/locomotion foundation, not hiring, payroll, skills, schedules,
full crowd simulation or a population/FPS benchmark. Local editor evidence does
not establish a packaged dedicated server or hosted internet session.

## Final live and visual evidence

- `EmployeeNetworkQA.json` / `EmployeeNetwork.log`: 48/48 checks passed with four
  initial players and a fifth late join. This includes shared co-op assignments,
  three independent business rosters, foreign-target assignment rejection without
  changing any roster, owned late-join data, and the real 40-part freight/payment
  regression. Per-project PIE settings were restored to one standalone player.
- Final `EmployeeFinalBuild.log`: build succeeded. `EmployeeFinalTargeted.log`:
  6/6 employee cases passed after the isolated v6 paint-preservation guard and
  measured forklift seat correction; the earlier full 103-case pass is separate.
- `EmployeeNativeQA.json` / `EmployeeNativeFinal.log`: 13/13 native workflow checks,
  no errors. Real steel was bought and delivered, off-duty staff prevented cutting,
  reassignment caused walking, exact arrival enabled ten actual brackets, and the
  employee returned to the waiting point. The world reset fresh afterward.
- `EmployeeVisualProbe.json`: 8/8 checks over 428 samples, no errors. Observed
  articulated leg movement and floor contact, stopped/paused gait, moving work
  arms, keyboard reach, seated driver pose and unique plant employee keys.
- `EmployeeManualQA.json`: 6/6 final physical mouse/E checks passed. Actual Team,
  Off duty, Plasma and Close X buttons changed/restored assignments and returned
  to first person while cash stayed $4,000 and stock stayed empty.

Approved native images: `Employees_WalkingToPlasma00001.png`,
`Employees_PlasmaOperator00001.png`, `Employees_ForkliftDriver00001.png`, and
`Employees_CompanyTeam00002.png` under `Saved/Screenshots/Reviews`. Earlier walking
and seat stills are superseded. The final clear console angle shows gloves meeting
its keyboard; the corrected driver clears the actual seatback and overhead guard.

Editable artist source is separate from runtime acceptance:
`SourceAssets/Employees/Prototype/Foundryline_PrototypeEmployee.blend` contains
Idle/Walk/Operate/Seated actions and a pose-switch script. Its evaluated hierarchy
checks pass measured 180 cm and planted walking feet. The runtime uses the tested
procedural adapter until final models and animation clips are supplied.

Unreal remains open in a fresh paused first-person starter shop near the office.
Machine operator and forklift driver are assigned; no steel or free batch exists.
Normal user saves and Spellscape were untouched. No push or merge was performed.
