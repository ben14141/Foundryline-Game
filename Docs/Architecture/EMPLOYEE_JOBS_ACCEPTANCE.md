# Employee jobs acceptance

The delivered workflow is Hire employee, select the named person, choose a job, and choose specific available equipment or a product-zone welding station. Hires are generic, free and unassigned. Current jobs, occupancy and activity are real simulation data; names and IDs survive role changes. The testing headcount limit is twelve per business.

## Verification

- EmployeeJobsFullFinal.log: 111/111 automated cases passed, including generic person plasma→forklift→welder work and save/load, exclusive claims, safe handoffs, migration and actual gradual passing on wide painted personnel lanes.
- EmployeeJobsFinalLabelBuild.log: final read-only CurrentJob label clarification compiled successfully. No simulation change followed the green full suite.
- EmployeeJobsNetworkQA.json: 62/62 live checks passed with four players and a fifth late join. Competitive plasma claims produced one winner; losing and foreign-company requests were rejected. Generic hiring/replay, persistent names, real 40-part manufacture/shipping/payment, independent businesses and company-private state passed. Editor play settings restored byte-for-byte.
- EmployeeJobsNativeQA.json: 28/28 native checks passed. Three generic hires appeared on foot, an assigned hire cut ten real brackets, safely changed to the existing forklift with the same name/ID, and twelve named staff paginated correctly. No extra forklift or hiring charge.
- EmployeeJobVisual_starter.json: six/six presentation checks passed, including stable component identity across generic→operator→driver and seated collision behavior. EmployeeJobsVisualProbe.json separately verifies six/six distinct-body/waiting/assignment checks.
- EmployeeWelderJobNativeQA.json: eight/eight functional checks passed in the transient larger stocked-workshop fixture. EmployeeJobVisual_welder.json: eight/eight visual checks across 824 samples, with walking, real product work, gun-tip sparks, holstered idle gun and measured glove-to-gun error below 0.000005 cm.
- EmployeeWelderVisualReview.json: two/two follow-up checks confirm the actual job chooser and a clearer actual working welder. The close-up uses only a transient review-camera exposure adjustment (+5 EV); normal game lighting was not edited.
- EmployeeJobsManualQA.json: nine manually observed native keyboard/mouse checks passed. E opened ERP; mouse clicks hired unassigned Avery Cooper at unchanged $4000, released Morgan Sullivan's idle plasma job and assigned Avery to the real plasma table. Close returned to first person. This is manual visual evidence, not a reflection-sampler claim.

## Reviewed images

Primary UI: EmployeeJobs_RecruitmentDesk00000.png and EmployeeJobs_AssignPlasmaJob00000.png. Specific forklift and later named role are visible in ChooseNewForkliftJob00000 and NamedEmployeeNewJob00000. Welder target/current occupancy is verified in WeldingJobChooserClear00000; WelderAtProductClear00000 shows actual welding in the explicitly stocked larger test workshop. Early welder chooser captures showed the desk and are excluded as UI evidence. Roster pages1–7 and8–12 were visually checked.

## Limits and final state

Prototype art remains replaceable; forklift boarding is immediate attachment. Narrow personnel spurs yield rather than promising unrestricted crowd passing. No payroll, packaged-build, large-crowd or FPS claim. The larger workshop's test inventory is its existing seeded fixture, not a claim of starter free stock or active finance. Normal player saves and Spellscape were untouched; nothing pushed or merged. Final playtest is reset to the cut-parts starter in first person, paused with $4000 and no stock.

Final fresh session verified: first-person at the office computer; 2 original named employees, cash 400000 cents, zero pallets, simulation paused. Read-only sampler: EmployeeJobsManualState.json. Editor left in Play for review.
