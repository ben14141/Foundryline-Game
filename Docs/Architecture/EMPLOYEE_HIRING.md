# Employee hiring

Current workflow: [Hire a person, then choose a specific job](EMPLOYEE_JOBS.md). New hires are generic and unassigned; the current testing cap is twelve people per business, rather than the earlier four-per-role prototype. Names are still generated once on the server and saved. Hiring and payroll cost zero; equipment purchases and equipment operating costs are unchanged.

The following records the earlier schema 8 hiring implementation and its validation history; job assignment and schema 9 behavior are governed by the current workflow above.

# Earlier schema 8 hiring foundation

The office ERP Team & logistics workspace provides free test hiring for machine operators, welders and forklift drivers. Each server-generated first and last name is stored with the employee once; clients, animation ticks and current-schema loads never generate or reroll names. Names are unique within the current company plant roster. New hires start off duty at a safe waiting position away from equipment, painted walking/forklift lanes and other staff. The initial operator and driver remain part of the roster and now have names too.

Free hiring has a server-enforced testing limit of four employees per role. Existing larger rosters in old saves remain intact. The older zone assignment action reuses an available hired welder before adding someone, and cannot bypass the role limit. Machine upgrades reuse available hired operators. Equipment purchases reuse hired reserve drivers before adding a free crew member when capacity permits. Geometry/equipment does not disappear if no operator can be assigned.

Hiring and payroll cost zero in this testing foundation. The existing flat equipment operating rates and freight/equipment purchase prices are unchanged. Equipment operating costs continue while machinery/forklifts work; these are not employee payroll. New accounting entries use that label while historical journal entries retain their recorded text. A forklift purchase still costs $1,200 for the equipment. Hiring a driver never grants a forklift, parking space, machine or production zone.

## Authority and replay handling

HireEmployee factory commands carry a role, target plant GUID and a new request GUID. The authoritative plant verifies the connected company member and exact target plant before evaluating role capacity and safe waiting space. Co-op members share one roster; other companies have separate records and cannot hire into or assign the foreign roster. The existing per-controller RPC sequence and 20-command-per-second server bound remain in effect. Up to 32 processed hiring request GUIDs are saved; a replay does not allocate another employee, name or charge. With the current bounded roster and no dismissal feature, the receipt bound exceeds the available hiring slots.

## Driver and equipment identity

A Driver record continues to identify an owned forklift with its existing Id. Jobs, reservations and forklift cargo keep using that vehicle ID. EmployeeId and DisplayName bind a person to the vehicle. A new free driver is instead saved in StandbyDrivers and rendered on foot, without creating any vehicle record.

Assigning a driver selects an owned, parked, unoccupied forklift. The vehicle ID, route and dispatch priorities remain stable while the employee retains their own ID and name. Going off duty returns the same person to the waiting roster and leaves an empty parked forklift. Active pallet jobs and the return-to-parking trip block removal or swapping. Empty forklifts cannot dispatch or move themselves. Boarding and leaving a parked forklift are immediate presentation attachment changes for now; a boarding animation is later work.

## Names, saves and migration

Schema 8 stores DisplayName on each existing operator/welder and occupied forklift record, plus standby driver records, employee/vehicle bindings and processed hiring requests. Schema 7 migration fills missing names deterministically from the saved company and employee IDs and preserves existing nonempty names, so repeatedly opening an older save does not reroll its staff. The existing schema 6 walking migration runs first when needed. Current-schema saves retain server-random hire names exactly. No normal user saves are altered by development tests.

## Validation

The shared hiring source compiled successfully, then all 108 Foundryline automation tests passed (`Saved/Logs/HiringFullAcceptance.log`, exit code 0). Five new hiring cases cover free hiring with no balance/ledger change; generated/stored unique names; request replay and capacity rejection; paid forklift purchase reusing reserve staff without changing its $1,200 price; the older zone button respecting the same staff cap; schema 7 naming migration and current-schema memory save/load; owned forklift identity preserved through driver replacement and a real freight job; a hired operator physically walking and completing a real part after a safe workstation handoff; and direct connected-player co-op/foreign-business authority guards.

After that run, only the approved accounting description changed to Equipment operating costs, with unchanged numerical rates and compaction behavior. The final description-only rebuild succeeded in 4.24 seconds. Live multiplayer then passed all 58 checks (`Saved/Logs/HiringNetworkQA.json`): four initial players plus a fifth late joiner, separate businesses, co-op shared names/hires, replay/foreign-target protection and matching stored employee names on clients.

The rendered native hiring workflow passed all 24 functional checks (`Saved/Logs/HiringNativeQA.json`): hiring all three roles without spending cash or creating equipment, binding the same named driver to the existing forklift, replacement-operator walking and actual ten-part production, filling the twelve-employee testing roster and paging its names, and returning to a fresh first-person start. The recruitment desk and both named-roster pages were visually inspected for data accuracy and layout. Correct recruitment UI evidence includes `Saved/Screenshots/Reviews/Hiring_RecruitmentDesk00000.png`; `Hiring_NamedRecruitment00000.png` captured the office view and is not evidence of that panel. Additional manual interaction and final screenshot selection are recorded by UI acceptance. Temporary employee models remain replaceable through the existing visual adapter; wages, payroll, dismissal, skills and shift scheduling are not implemented.
