# Hire a person, then choose a job

The ERP now separates hiring from assigning work. Hire employee creates one named, unassigned person. It does not choose a profession, claim equipment, start a batch or create a forklift. The testing headcount limit is twelve people per business, shared by co-op members. Hiring and payroll remain free.

Select a person in Team & logistics, then Assign job. Choose one of the installed targets:

- Machine operation: the specific plasma table, press brake or sheet roller that is installed in this plant.
- Forklift driving: a specific owned, parked forklift without another assigned employee.
- Welding: a specific product work zone and its physical zone-mounted welding machine.

The recipe/batch stays on the machine computer; freight tasks stay with the existing automatic dispatch routes; the product/order stays on its work zone. An employee job is responsibility for that specific equipment or station, not a new recipe definition. The displayed current job derives from the real machine recipe, real pickup/delivery task, or real product assembly step. A completed product is shown as awaiting pickup.

## One identity across jobs

Employee ID, stored name and company/plant visual key stay unchanged when a person moves between machine, driving and welding jobs. There is one roster row and one physical employee representation. The simulation stores the active role in its existing operator, walker or vehicle binding record; generic waiting people use UnassignedEmployees. A role change moves that backing record atomically, without generating a replacement ID/name or changing company cash.

An unassigned operator/welder may retain its existing backing record while walking back to the waiting area. Its roster role is Unassigned during that departure. A new job can be chosen once the person reaches home. A driver leaves an owned parked vehicle and becomes an unassigned foot employee; boarding/unboarding remain immediate attachment changes rather than full animations.

## Authority and safe handoff

Assignments validate the connected company member and exact target plant GUID. Other businesses cannot claim this employee or equipment. An operator station, forklift and new welding-zone assignment can have one employee each. Competing requests are evaluated on the server; the second claimant is rejected without converting or duplicating their employee record. Existing saved legacy bindings remain intact.

Release the current job before choosing another. An enabled machine with a part in progress must be paused first; its reserved steel and remaining cycle are preserved. A forklift driver cannot be released during a pallet job or the return-to-parking trip. Active welding requires pausing production before release; reserved product parts and partial work remain in the zone. Replacement workers wait for a departing colleague to clear the access path.

Machine and assembly work require the assigned person to reach the real workstation. Finished paths at a waiting location do not count as attendance. Welders travel to the station for an actual product order and wait at their safe home otherwise. Existing starter/world staff remain assigned on startup; newly hired people never receive an automatic job. Purchased equipment starts unstaffed until a player assigns someone.

## Saves

Schema 9 adds the generic waiting pool and migrates schema 8 standby drivers into it with the same IDs and names. Current assigned staff retain their bindings. Older operator/name migrations still run in sequence. Unsafe older welder waiting homes may be replaced with a safe off-lane destination; the person walks there rather than having its current position teleported. Saved active jobs, machine cycles, product reservations, pallets, balances and forklift vehicle IDs remain separate from employee role changes.

## Validation

The combined implementation compiled, then all 111 Foundryline automation tests passed (`Saved/Logs/EmployeeJobsFullFinal.log`, exit code 0). The same generic person performed real plasma production, a real forklift pallet job and real zone welding with unchanged ID/name/visual key, then saved/loaded its specific welding job. Coverage also includes competing claims, legacy employee IDs equal to vehicle IDs moving into operator and welder records, schema 8 reserve migration, active-operation release denial, paused product reservations, explicit hiring/assignment for a purchased brake, and all prior economy/transport/construction regressions.

The shared-lane passing case advances two real opposing operators for 400 simulation ticks; it checks arrival, at least 52 cm separation, maximum normal 110 cm/s steps and painted clearance, and rejects narrow spurs, nearby forklifts and physical side obstacles. Passing changes the route, never teleports the person or ignores normal traffic checks.

After the full run, only the automatic-machine CurrentJob description was clarified to awaiting demand rather than treating an ignored batch counter as a real manual batch. The final projection-only build succeeded in 4.13 seconds. Live multiplayer then passed all 62 checks (`Saved/Logs/EmployeeJobsNetworkQA.json`): four initial players and a fifth late joiner, a single winner for competing generic employee claims on the plasma, matching employee names/IDs/jobs across co-op clients, independent companies, foreign-target denial, private operations state and the real production/shipping/payment loop.

The standard stocked workshop welding fixture passed all eight rendered functional checks (`Saved/Logs/EmployeeWelderJobNativeQA.json`), including a newly hired generic person assigned to a specific real zone, walking to its station, actual product work and welding presentation, and paused work. This is the existing larger workshop profile with authored stock; it is explicitly not a claim that the small starter begins with tanker inventory or a welding zone. The artifact showing the working person at the product was visually reviewed. One chooser capture displayed the office instead of its modal and is excluded as chooser evidence. Starter job UI/cross-role visual and final manual acceptance are recorded by the UI coordinator. Temporary models, payroll, skills, dismissal and shift scheduling remain outside this change.
