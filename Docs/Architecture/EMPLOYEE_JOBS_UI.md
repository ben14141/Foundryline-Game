# Hiring and explicit job assignment

The Team application now separates people from jobs. Hire employee creates one randomly named, unassigned person at a safe waiting position. Select that person and choose Machine operator, Forklift driver or Welder, then choose an installed machine, owned forklift or placed product-zone welding station. Hiring supplies no equipment.

The ERP chooser lists specific locations, their current occupancy, the selected employee's live job/activity and a Release job action. An employee must leave the current assignment and return to the waiting area before taking another job. Name, person ID and world visual identity survive role changes. The company testing limit is twelve people in total; payroll remains disabled.

Each target is an exclusive server claim. Active enabled machine cycles must finish or be paused before releasing their operator. Forklift drivers cannot leave active cargo trips or returning vehicles; the forklift must park. Active welding requires paused production before release. Parts, reservations and partial product work survive safe staffing changes. Co-op uses the company's shared roster; other businesses have separate ownership and cannot target foreign staff or equipment.

CurrentJob is a read-only description of actual recipe, forklift task or product-zone work rather than a second assignment model. Schema9 preserves previous employee names/IDs and migrates unassigned drivers into the generic available-person list. Existing assigned starter/migrated employees retain their jobs. New equipment purchases remain unstaffed until the player chooses an employee.

The larger stocked-workshop native fixture exists only for testing actual welding stations; the small cut-parts starter correctly shows no welding target until a product work zone is placed. No normal player save is changed by these checks. Prototype visuals and immediate forklift boarding remain presentation limits.
