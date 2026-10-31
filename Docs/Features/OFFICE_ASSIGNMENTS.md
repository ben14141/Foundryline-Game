# Assigned offices

Each business has its own president and engineering staff. Team > Offices on the physical office computer assigns an owned office room to a person. Engineering is a hireable role; its production, design and research duties will be added later. An engineer without an office remains waiting. An assigned engineer sits at the room's computer with keyboard and mouse animation. Autonomous office travel is not part of this foundation.

The president represents the existing business owner, not another spawned employee. Each person may have one office, and each office may have one occupant. A room requires a personnel door and enough space for the computer, chair and access clearance. An undivided office floor can be assigned as one office; once partitioned, its individual rooms are assigned instead. Both office floors are supported.

## Furnishing

Furnishing is free while testing. Assignment reuses a suitable saved workstation or the existing office terminal. Otherwise it adds a computer desk and chair. It never adds another paid equipment purchase. Furnishing respects the actual room size, door access, staircase and other desks. Decorative pieces are optional when space is limited.

- President: executive storage, bookshelf, company artwork, plant, guest seating, desktop accessories and rug.
- Engineer: flat-file cabinet, blueprint board, drafting table, plant, technical accessories and rug. The engineer wears a collared shirt, trousers and tie.

Reassignment removes the previous role decorations. Unassignment keeps the underlying computer desk for reuse. Unassign first to move an assigned room, building, stair or workstation; this prevents separating the employee from their assigned space.

Additional wall decor includes a detailed analog clock, framed craftsmanship certificate, three-panel industrial artwork, engineering whiteboard with sketches and marker tray, and a project noticeboard with pinned papers. Presidents receive the clock/certificate/artwork set; engineers receive the clock/whiteboard/noticeboard set. Pieces are optional where clear wall space is limited. They avoid existing wall decor, tall storage, doors and glazing, and mount against the actual interior or exterior wall thickness.

Older assigned offices are enriched once through saved furnishing version 2, with no new desk, charge or occupant change. Repeated updates do not duplicate pieces. All the new wall decorations participate in reversible glass-panel hiding.

## Doors and interior glass

Assignment changes the office's access door to the original glass door mesh. Its frosted lettering shows the occupant's name and PRESIDENT or ENGINEER. Lettering is attached to the swinging leaf on both sides. Reassignment updates the name; unassignment restores the ordinary door. Existing door interaction and collision remain in use.

Interior glass partitions are manually placed from Build > Office. They replace a solid interior room-wall span, reach the 9 ft 10 in clear ceiling, and retain collision. They do not create another door or automatically glaze other walls. Panels fit available grid-aligned wall sections around existing doors. Exterior office glazing is a separate tool.

Glass clears artwork or blueprint boards it covers. This is saved as a reversible visibility flag: removing or undoing the glass restores that artwork. Other furniture is preserved. Door clearances are reserved during initial furnishing.

## State and ownership

Hiring and office assignment are server-authoritative, belong to a company and plant, and replicate through the existing factory snapshot. Assigning and releasing an office require access to that business's physical computer. Occupants, desks, decorations and covered-art flags are saved. Duplicate occupants, overlapping room ownership, missing desks and invalid property references are rejected. Local floor visibility remains independent between players.

Original furnishing models live under `/Game/Foundryline/Office/Decor`. The engineer uses the licensed smooth Handyman parts with a private derived palette and an original tie; purchased character source and derived textures are not cleared for public redistribution.

## Scope limits

Engineering currently provides hiring, office assignment and visual presence. It grants no engineering production bonus or research and does not yet have office travel or engineering payroll rules. Free furnishing is a testing rule, not final progression balance. Automated tests and native review results must be reported separately from these design rules.

## Validation — October 31, 2026

- FoundrylineEditor Development build passed.
- Full automated suite: **207 passed**. New office tests cover ownership, duplicate assignments, free furnishing, hiring, save serialization, assigned-space edit guards, upstairs offices, manual glass, paired shared-wall removal, finite wall targeting near adjoining rooms, reversible artwork hiding and idempotent upgrades to existing furnished offices.
- Native four-player review: **28 passed**, through the queued gameplay commands and actual E input. Checks include shared furnishing/door state, free cash behavior, existing-terminal reuse, independent rooms, occupied-room refusal, actual capsule blocking at glass and closed doors, open-door traversal clearance, engineer inspection and changing hand positions while typing, additional wall decor on every peer and clearing new decor behind manually placed glass.
- Visual review corrected buried wall artwork, engineer material usage and reverse-side lettering showing through glass. Native screenshots are under `Saved/Screenshots/Reviews/OfficeAssigned_*`.
- Reports: `Saved/Automation/Focused-Full/index.json`, `Saved/Logs/OfficeAssignmentsNativeQA.json`, and `Saved/Logs/OfficeFurnitureImportQA.json`. The four-player review exercised ground-floor rooms; upstairs assignment/glass were covered by automated tests. This is not a production player-cap or performance measurement.

Re-run with `Tools/Validation/Invoke-FoundrylineCheck.ps1 -Profile Full` and `Tools/Unreal/run_office_assignments_review.ps1`. The native runner restores the project's editor play settings and does not save the review map or player progress.
