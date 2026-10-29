# Hiring acceptance

The ERP Team application has a dedicated Hiring workspace with three free-testing roles, company counts, recent named recruits and actual assignment controls. Random first/last names are assigned once on the server and saved. Co-op shares one roster; independent companies own separate staff.

## Evidence

- HiringFullAcceptance.log: 108/108 automated tests passed, including name persistence, migration, request replay, company authority, free hiring, role limits and driver/equipment separation.
- HiringFinalBuild.log: final label-only build succeeded.
- HiringNetworkQA.json: 58/58 live four-player plus fifth late-join checks passed; no errors. Concurrent hires, same-request replay, foreign-company rejection, stable replicated names and unchanged hiring cash verified.
- HiringNativeQA.json: 24/24 native functional checks passed, no errors. Hired all roles, assigned a new operator, purchased real steel and cut ten parts, maintained names through work, exercised 12-person pagination, then restarted fresh.
- HiredEmployeeVisualProbe.json: six/six visual checks passed across 714 samples. Unique bodies, three foot hires, separated waiting positions, stable operator walking identity, stable driver seating identity and released driver returning on foot.
- EmployeeManualState.json: physical keyboard/mouse opened ERP, navigated Team/Hiring, hired Casey Sullivan ($4000 unchanged), released original operator and assigned Casey to plasma. UI closed and PIE restarted fresh afterward.

## Images

RecruitmentDesk00000 is the verified Hiring panel. NewHiresWaiting00000 shows distinct grounded new employees. PlasmaOperator00000 shows the hired operator at the actual console. RecruitmentAtTestingLimit00000 shows roster page 1; NamedRosterPage200000 shows page 2. Some automatically captured filenames reflect a preceding UI state; do not infer content solely from filenames.

## Scope

Four employees per role is the current testing limit. Hiring never supplies a forklift. Driver boarding is an immediate attachment transition; art remains replaceable prototype employees. Payroll is disabled; existing equipment operating charges retain their prior amounts. No packaged-build, crowd-scale or FPS claim is made. Normal player saves and Spellscape were untouched. Nothing was pushed or merged.
