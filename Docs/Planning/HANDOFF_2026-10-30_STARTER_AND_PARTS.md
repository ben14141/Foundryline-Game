# Foundryline handoff — October 30, 2026

## User decisions

- Start players in first person in a clean shop with an attached office and computer. The plasma table is the only initial production machine. No fabrication bench, press brake, roller or tanker is initially installed.
- Provide a forklift, raw-steel rack, cut-parts rack, parking, paved shipping access, one tier-1 small-truck loading bay and a separate forklift garage. Personnel and office connection doors must have real walking clearance.
- Limit the active product range to the headache-rack chain for now. Keep steel sheets, bracket blanks, formed brackets, mounting plates, frame sections, light kits and finished headache racks. Other parts are disabled in the current recipes/contracts, while historical IDs and asset files remain for later.
- The user explicitly selected **Require formed brackets**: one headache rack needs **2 mounting plates, 4 formed brackets, 6 frame sections and 2 light kits**. Plasma cuts bracket blanks; the press brake forms them before welding assembly. Any press-brake output must say **Formed** in its name.
- Parts must have default weights, including sheet metal. Pallets must have a maximum cargo weight as well as physical quantity limits.
- Light-kit cartons need a clear visual identity. Keep the large LIGHT KIT labels, lamp pictogram, orange bands and LK-01 end labels. The user subsequently requested **removing the lamp from the top**: the final carton has a sealed flat lid. Do not restore the sample lamp.
- Multiplayer remains mandatory for gameplay and business ownership; UI/camera/roof preferences remain local. Keep other projects and other users' Unreal/Blender sessions separate.

## Current implementation

Default map: `/Game/Foundryline/Maps/L_FirstShopStarter`. Launch with `Tools/Launch-FirstShop.ps1`. Older empty-shell and larger factory maps remain separate. Construction uses normal editable, replicated records. The shipping yard uses plain asphalt so the loading preset supplies the intended bay markings without the old overlapping yard stripes.

Default unit weights: sheets approximately 255 lb (60 × 120 × 1/8 inches, calculated from dimensions); bracket blanks and formed brackets 5 lb; mounting plates 2 lb; frame sections 16 lb; light kits 4.5 lb; headache racks 129 lb. A standard pallet supports 1,500 lb of cargo. Oversized sheet/headache-rack loads already treated as two-pallet loads have a combined 3,000 lb cargo limit. Freight includes 60 lb per supporting wooden pallet. Existing physical stack/count caps remain in force.

Weight checks cover stock creation, forklift reservations and destination combinations, shipping bookings, and saved pallets/manifests. The stock inspector shows unit weight, current/maximum cargo weight and gross freight weight. Small-part pallets render their actual quantity with instanced meshes: up to 50 bracket blanks, 50 formed brackets, 100 plates and 8 light-kit cartons. Sheet stacks retain the earlier representative three-sheet display while quantities/weight remain exact.

Light-kit source: `SourceAssets/Products/Components/LightKit/LightKitBox.blend`; export: `SM_LightKitBox.fbx`. Runtime mesh: `/Game/Foundryline/Products/Components/LightKit/SM_LightKitBox`; thumbnail: `/Game/Foundryline/UI/BuildCatalogue/T_Catalog_Part15`. Final dimensions approximately 41.786 × 28.475 × 27.43 cm; four standalone materials. Supply bins/cart contents also use identifiable carton meshes, with representative quantities there. Build/import tools: `Tools/Blender/build_light_kit_box.py` and `Tools/Unreal/import_light_kit_box.py`.

## Validation and limits

- Latest code build: Win64 Development succeeded; all **162 automation checks passed**, including the complete plasma → forming → headache-rack assembly → shipment flow. That assembly-flow test uses an expanded upgrade shop, not an initial bench installed in the starter.
- Starter review previously passed 23 checks across four players, including matching construction, desk access and door positions. Door capsule sweeps use actual snapped opening centres.
- Current-parts native review passed **16 checks across four players**, including restricted starter offers, replicated weights/counts and exact rendered pallet quantities. Evidence: `Saved/Logs/CurrentPartsQA.json`, `CurrentPartsReview.log`, `Saved/Automation/Focused-Full/index.json` and `FocusedBuild.log`.
- The subsequent removal of the sample lamp was a visual-only revision. Its rebuilt mesh and thumbnail imported cleanly; bounds and material checks passed in `LightKitImportQA.json` and `LightKitFlatLidImport.log`. The 162-test/16-check runs preceded that visual revision and were not repeated for it.
- `CurrentParts_FullPallets00000.png` and `LightKits_InGame00000.png` are gameplay screenshots of the earlier carton with the sample lamp. The current flat-lid model preview is `SourceAssets/Products/Components/LightKit/LightKitBox_Review.png`. Do not present the older in-game picture as the final box.
- Normal one-player standalone editor preferences were restored after native reviews. The starter game is open for playtesting. Review stock and purchases were unsaved.
- No new FPS, packaged-build or production multiplayer-cap claim was made.

## Git and workspace

Actual checkout: `C:/Users/curly/Documents/Foundryline Game`; the chat's OneDrive working directory is stale. Use the actual checkout explicitly.

PR #23 was pushed and merged into GitHub main at commit `963e8f03fbb017a314b2a04e0e21107a72537839` before the starter/parts/weight changes above. The current branch is main with these newer changes still local and uncommitted. No later push or merge was performed. Obtain explicit permission before another merge; do not infer it from the earlier completed request.

The GitHub repository is public. Its existing policy excludes purchased Fab content, most Content/SourceAssets, maps, caches and local saves. Do not blindly force-add whole asset folders. Original model sources/import tools are locally available; a source-only clone still needs an authorized local content backup.

Additional details: `Docs/Architecture/FIRST_SHOP_START.md` and `Docs/Architecture/CURRENT_PARTS_AND_LIGHT_KITS.md`.

## Pending bugs — user requested notes only; changes halted

The user explicitly said: "stop changing, just take notes". Do not continue implementation, asset imports, compilation, launches or testing for these issues until the user resumes the work.

1. Starter flatbed truck: its upper side rails must fold down. Existing loading-side gate animation does not satisfy the reported behavior; inspect which rail geometry remains baked into the body before modifying it.
2. Trucks appear to float above the ground. Diagnose actual tyre contact height and road/pavement alignment for the affected carriers. No fix has been made. Read-only inspection found carrier route poses hard-coded at Z=20 and a route through X=4200, whereas the newer starter's shipping pavement ends at X=3600 and its access driveway is centred at X=2400. This is a diagnostic lead, not a confirmed root cause; also measure mesh/wheel pivots and offsets in-engine.
3. Placing the decorative manufacturing sign reportedly removed curbs. Sign placement should not remove unrelated pavement/curb construction. Investigate placement ownership, overlap handling and edit/replace selection before proposing a fix.
4. The decorative sign's reserved placement grid/hologram footprint is much too large. Reduce it to an appropriate actual sign footprint and necessary access clearance, without changing its real-world model scale.

Only read-only source inspection was performed for the truck request before the halt. No truck/sign implementation changes, Blender work, asset imports, builds or validation runs were performed for these reported issues. Relevant inspection pointers: Source/Foundryline/Private/Logistics/FoundrylineCarrierSceneActor.cpp; FoundrylineFlatbedCarrier.cpp; Tools/Blender/convert_starter_flatbed.py; Tools/Unreal/import_starter_hauler.py; SourceAssets/StarterHauler/manifest.json. Preserve the existing local starter/catalogue/weight/model work and do not push or merge without a new explicit request.
## Additional production-zone notes — implementation remains paused

User screenshot Screenshot 2026-10-30 204736.png shows a plasma output pallet sitting over the forklift-lane paint rather than in dedicated in-zone storage. Requested behavior:

- Plasma output is an actual pallet rack inside the plasma work zone. It automatically builds pallets of the produced parts; no loose output pallet should appear in a travel lane.
- This changes the plasma zone's footprint, paint, placement hologram and clearances. Include the machine/operator station, sheet input storage, output rack and forklift access in the layout; keep forklift lanes clear.
- Earlier agreed design: one dedicated sheet-input rack automatically supplies the plasma; forklifts replenish it. Standard output rack has four distinct pallet positions, two at floor level and two on the upper level. Each storage pallet holds one part type, allowing different cut parts to occupy separate slots. Reuse the standard racking style. Storage upgrades can come later.
- Retain the current part weights and pallet cargo/physical limits. Finished output should accumulate visibly in its assigned rack position; clarify full-rack behavior and partial pallets during implementation, rather than silently spawning overflow outside the zone.
- The planning-computer production catalogue currently does not let the user purchase/place a plasma table, press brake or sheet roller. Add these as purchasable, placeable equipment/workspace packages with clear holograms, collision/access checks and authoritative business ownership/cost validation. An existing press-brake ERP upgrade button is not a substitute for the requested planning placement options.
- Keep the starter's initial production equipment at one plasma only. Making other machines purchasable does not mean seeding them in the first shop. The roller should be purchasable without re-enabling unrelated retired products or inventing a new active recipe.

The preceding explicit instruction was stop changing, just take notes; these requirements were recorded without resuming implementation. Confirm that the user wants work resumed before changing the build. No new edits to gameplay, models, imports or tests were made for this request.
## Forklift-lane planner controls — additional notes while paused

User screenshot Screenshot 2026-10-30 205012.png reports that forklift lanes cannot be selected or removed. The bottom planner bar shows Build [Q], Select, Move, Resize and Remove; the user wants all of these controls to work. The visible message is "Lane overlaps machinery or racking" while Forklift lane remains the active tool.

When implementation resumes:
- Ensure existing painted forklift lanes participate in planner hover highlighting and selection, including their painted edges and overlapping/joined junctions. Selecting must not try to paint a new lane or get intercepted by the building floor underneath.
- Verify Build, Select, Move, Resize and Remove through the actual visible buttons and mouse interactions. Confirm the intended tool becomes active and the old painting tool releases input; clear stale placement errors when switching tools.
- Support selecting/removing existing lanes, moving and resizing them accurately, and normal cancel/undo behavior. Check joined segments/intersections so the affected lane is understandable before removal.
- Keep actual machinery/rack overlap protection. Diagnose whether the screenshot is a real overlap, an oversized reserved footprint, or an incorrect active-tool/input state; do not simply suppress the warning.
- Test edits against machine/rack clearance, work zones and neighboring lanes. Changes must retain server validation, business ownership and consistent replication across more than two players; pathing should refresh after successful lane edits.
- Recheck the same control bar across machines, racks, zones, doors, signs, buildings and paving so it does not silently cover only some object types.

Recorded only under the user's existing stop-changing/notes-only instruction. No lane or planner fixes, builds or tests have been performed for this report.
### Forklift-lane rotation

The user also reports that rotating a forklift lane is difficult or unreliable. When work resumes, check both rotation during placement and rotation of an existing selected lane. Verify the rotation control, visible direction/angle feedback, pivot, grid snapping, preview matching the placed result, overlap validation and joined-lane behavior. Include rotation in the planner control acceptance checks alongside selection, move, resize and removal. Exact failure behavior is not yet diagnosed. Notes only; no changes or tests performed.
### Rotate while moving existing items

The user reports they cannot rotate items while moving them. This applies to planner items generally, not just forklift lanes. When implementation resumes, verify rotation during an active move, with a clear control hint, consistent pivot/snapping, preview-to-placement agreement, collision/access validation and server-authoritative replication. Include cancel and undo so a moved/rotated item can be restored correctly. Do not silently restrict rotation to initial placement. Notes only; no implementation or tests performed.
### Planner and logistics HUD styling

User screenshot Screenshot 2026-10-30 205247.png: update all visible planner/logistics buttons to match the established build-catalogue popup and office-computer UI. Scope includes Run/status controls, Office F, Roofs toggle, Draw Route, Select Node, Move Node, Test Route, Save, and the bottom Build Q / Select / Move / Resize / Remove bar, plus the associated contextual instruction panel.

Use the established Foundryline logo palette and shared button treatment: charcoal/graphite, warm off-white and orange accents; appropriate icons, consistent typography/spacing/sizing, restrained borders/shadows, hover animation, pressed/selected/disabled states and clear tool feedback. Organize related actions and keep them usable at different viewport sizes; preserve the existing improved popup/computer appearance as the visual reference. Test actual hitboxes and tool switching as part of the previously noted control audit, rather than applying only a visual reskin. Notes only while changes remain paused.
### Completed pallet does not move to loading bay

User reports completing a pallet of parts, selecting Move to loading bay, and seeing no response from the forklift driver. Record this as a shipping-flow blocker; root cause is not diagnosed.

When work resumes, reproduce through the actual player UI and inspect the command result, selected pallet and destination, business ownership, order/stock eligibility, reservations, free destination capacity, driver assignment/priorities, reachable pickup/drop points and lane connectivity. Determine whether the button never sends a request, the server rejects it without useful feedback, the transfer job is not created/dispatched, or the assigned driver cannot reach it. Do not assume the driver or lane is the cause before tracing the request.

Acceptance: a valid completed pallet gets a transfer job and is picked up and delivered to the chosen loading bay by an available assigned forklift driver. Show a clear waiting/blocked reason when transport cannot proceed. Preserve quantity/weight, order assignment and reservations; avoid duplicate jobs on repeated clicks; verify consistent progress for co-op players. Notes only; no fix, build or test performed under the existing pause.
#### Additional evidence: pallet not assigned to a shipment

The user reports the pallet message says it is not assigned to a shipment. This may explain the failed Move to loading bay action, but is not yet a confirmed root cause. Distinguish customer-order assignment, loading-bay assignment and carrier-booking assignment; do not assume all three must exist before staging a pallet. Earlier agreed flow lets players stage stock, select partial/full pallets on the tablet and schedule a suitable carrier afterward. When investigating, determine which assignment is actually required and whether the action should perform that assignment, request it clearly, or permit unassigned stock staging. A missing assignment must not cause a silent no-op. This is additional diagnostic evidence for the existing blocker, not a tested finding or a new implementation. Work remains paused.
### Equipment menu found; support additional machines

User clarification: they found the equipment. Do not continue treating the earlier report as proof that the purchase menu is missing. Verify the existing controls before adding duplicate menu entries.

The requirement is that players can buy additional copies of machines, including extra plasma tables, press brakes and rollers, rather than being limited to one fixed machine of each type or a one-time upgrade. Each purchased machine/workspace needs a stable unique identity, independent placement/rotation and appropriate input/output storage, recipes, queues, operator assignment, running state and logistics connections. Reuse the current active recipe restrictions; extra rollers do not imply restoring retired products. Purchases must charge the owning business correctly, validate space/access and ownership on the server, persist through save/load and replicate across co-op players without replacing or controlling another machine of the same type. Preserve the plasma-only starter loadout. Work remains paused; these are notes, not implemented or tested changes.
## Sales / Dispatch source audit and tablet UI update

The user requested double-checking Sales versus Dispatch organization and updating Dispatch pages to the new UI standard. Read-only source inspection confirmed:

- `FoundrylineOperationsHUD.cpp` routes only OperationsScreen==1 to the modern `DrawBusinessDesktop`. Screen 2 (dispatch tablet) and Screen 3 (pallet inspector) still use the older renderer and button treatment.
- `FoundrylineOperationsController.cpp::OpenOperations` initializes Screen 2 on OperationsPage 2.
- In the legacy renderer, Page<=2 unconditionally renders Marketplace, Customer Mail and Accepted Orders tabs. This applies even on the dispatch tablet. The shared order detail can render ACCEPT FIXED TERMS for offered contracts. Thus sales content in Dispatch is a confirmed source-level UI grouping issue, not merely a mislabeled page.
- Tablet navigation is Assigned Job (page 2), Pickup Manifest (8), Dispatch Calendar (4). Page 8 asks for an assigned customer order and sends the player to Page 2 when missing; clarify that step without exposing the sales marketplace.
- The office-computer Stock & trucks/calendar pages already use the newer BusinessDesktop design. The tablet has not received that same conversion.

When implementation resumes, keep contract discovery, customer mail, pricing and acceptance in Sales/Contracts on the office computer. Dispatch can reference accepted orders and show shipping progress, bay assignment, ready/staged pallets, quantities/weights/capacities, carrier choice, fees, booking time and receiving/pickup calendar; it should not expose marketplace or contract acceptance tabs. A clear link back to office Sales is appropriate when an order must be accepted first.

Update all three Dispatch pages (Assigned Job, Pickup Manifest, Dispatch Calendar) to the established build-popup/computer style: Foundryline charcoal/graphite/off-white/orange palette, shared button surfaces, illustrated products/company marks where useful, consistent hierarchy and spacing, clear selected/disabled/hover states and responsive layouts. Include the pallet inspector in the consistency review so weight and shipment-assignment messages match the new standard. Keep freight actions and their ownership validation intact. Trace the previously reported Move to loading bay / not assigned to shipment blocker alongside these flows.

Acceptance should include actual tablet navigation with no Sales offer tabs, assignment/staging/manifest/booking progression, partial pallets, payload/space constraints, blocked/waiting explanations, scheduling changes, and independent per-player UI with replicated authoritative state. This audit inspected source only. No UI/code changes or live playtests were performed; implementation remains paused under the preceding notes-only instruction.
### Plasma-table computer UI

User requests updating the plasma-table computer screen to the same current UI standard as the build popup and office computer. Include the physical-machine interaction popup, recipe/part selection, product previews, batch quantity, per-part material pounds and processing time, available sheet stock, active/queued production progress, operator/machine status and clear start/pause/control feedback. Use the established Foundryline charcoal/graphite, warm off-white and orange treatment, shared button surfaces, consistent spacing/typography and hover/selected/disabled states. Keep the reduced active catalogue and exact weight/time values intact. Review press-brake and roller screens for consistency when their purchase/extra-machine work resumes. Notes only; no UI/code changes or testing performed under the existing pause.
### Load active jobs and queue work from the plasma computer

User proposes that the plasma-table computer show all active jobs, with click-to-load and the ability to queue more work. Interpret active jobs as accepted customer orders and relevant production demand, clearly distinguished from machine transport jobs; verify terminology during implementation.

When work resumes:
- Show active jobs with customer/reference, part image/name, ordered quantity, already completed/in production/queued quantities, remaining requirement and deadline where applicable.
- Let the player select a job and load the compatible plasma part/program and a sensible remaining quantity into the production controls. Let the player review/adjust before starting or adding it to the queue.
- Permit multiple queued batches/jobs behind the current operation, with clear current versus upcoming work. Preserve the ability to change recipes mid-sheet and consume the remaining sheet pounds normally.
- Do not duplicate requirements when the same job is selected repeatedly or when multiple machines/players already have quantities allocated. Include existing stock and allocated/queued production when showing the remaining quantity, and distinguish stock production from a customer-linked batch.
- Formed-bracket jobs need plasma bracket blanks first, then forming; headache-rack assembly demand can expose its required compatible cut components without pretending the plasma makes the finished assembly. Bought-in frame sections and light kits are not plasma recipes.
- Keep queue additions/cancellations and allocations authoritative per business and per unique machine, persisted and replicated for co-op. Provide clear material/operator/output-storage blocked states and respect full output racks and pallet limits.

This is an additional proposed machine-computer flow recorded under notes-only mode. No queue/UI implementation or tests were performed.
### Racking model quality pass

User requests improving the racking model. When work resumes, refine the standard rack family rather than introducing many specialized rack types. Preserve one-, two- and three-level variants, real-world scale, pallet support heights and forklift clearances. Improve upright/channel profiles, bracing, beam connections, locking hardware, feet/anchor plates, shelf/pallet supports, protective details and material finish. Maintain clear Parts versus Finished Goods identification and the agreed plasma output-rack layout. Check pallet seating, alignment, shelf spacing, collision and model appearance with actual stocked pallets at each level. Keep reusable components/materials and reasonable rendering cost. These detail choices are the proposed quality-pass scope, not newly approved dimensions or a completed design. Notes only; no model edits/imports/tests performed under the existing pause.
#### Plasma output rack: additional screenshot evidence

Screenshot 2026-10-30 210236.png shows the rack marked CUT PARTS / PALLETS empty, while a completed cut-parts pallet sits separately beside the plasma machine, outside the rack's painted storage area. The user explicitly links this to the earlier request for output racking inside the plasma zone.

Treat this as one underlying output-layout/flow issue rather than adding another detached staging patch. When resumed, align the machine's output spawn/accumulation position, assigned rack slot, physical pallet support height, floor markings and forklift pickup dock with the same in-zone output-rack definition. New pallets should visibly build in the appropriate rack slot, not appear on the floor elsewhere and wait for a separate storage transfer. Keep the four-position standard (two floor, two upper), one part type per pallet, full-rack blocking and weight/physical limits. Moving or rotating the machine workspace must transform all these positions together. Investigate current behavior before claiming a diagnosed cause. Notes only; no implementation/tests performed while paused.
### Personnel-door visual quality and fit

User screenshot Screenshot 2026-10-30 210411.png reports that the doors look terrible. Visible concerns include large apparent side gaps between the leaf and frame and crude-looking window surround, hardware and finishes. The screenshot alone does not establish whether the door is fully closed or partway through its swing, so check closed/open positions before diagnosing a scaling or pivot defect.

When work resumes, perform a visual and functional pass on office/shop personnel doors: correct 1:1 leaf dimensions and thickness, matching opening/frame/reveal, sensible threshold and hinge pivot, proper lever/push hardware, believable vision-panel glass and trim, and coherent material colors/roughness consistent with the building. Keep office and industrial door variants appropriate without stretching source meshes. Validate appearance from both sides while closed, opening and fully open; include E interaction, collision/walking clearance and synchronized co-op state. Existing clearance tests do not establish acceptable visual quality. Notes only; no model/code edits, imports or tests performed under the existing pause.
### Program Plasma opens Dispatch

User reports that selecting Program Plasma on the office computer navigates to Dispatch instead of the plasma production/programming controls. Root cause is not diagnosed. When work resumes, trace the actual button action through page/screen routing and selected-machine state. The action should open the appropriate owned plasma machine's recipe/batch/job-queue screen, carrying the selected job/compatible part and remaining quantity when launched from an order. With multiple plasma tables, provide a clear machine choice rather than routing to an unrelated screen or assuming one fixed machine. Check return/back navigation and keep player UI local while production commands remain authoritative. Include this in the office/Dispatch/machine-computer navigation audit. Notes only; no fix or tests performed under the existing pause.
### Correction: program plasma only at the machine computer

Latest user decision supersedes the earlier proposed Program Plasma navigation fix: players must program the plasma only from the plasma machine's own computer, not from the office computer. Remove the office Program Plasma action rather than redirecting it to remote machine controls. Office pages can show contracts, production requirements and status, but must not expose plasma recipe/batch/queue programming. Keep the active-jobs list, click-to-load compatible recipes/remaining quantities, and queue-more-work flow on the plasma computer itself. For multiple plasma machines, the player interacts with the actual machine's computer and controls that machine's independent queue. Apply appropriate server-side interaction/ownership validation, rather than enforcing this rule only by hiding a button. The reported office button routing to Dispatch remains evidence of a bad existing action, but repairing that remote-programming route is no longer the intended solution. Notes only; work remains paused.
### Duplicate/overlapping forklift driver visuals

Screenshot 2026-10-30 210611.png shows a seated forklift driver plus another small head/upper body visible through the lap/seat area. User flagged the visual humorously. Visible result is overlapping character geometry; the cause is not confirmed.

When work resumes, check for a retained built-in/placeholder driver mesh alongside the animated employee, duplicate spawned employee visuals, incorrect modular character parts, and seat attachment/pivot/scale. Render exactly one correctly sized employee per occupied forklift seat with proper seated posture and hand/foot alignment. Preserve the employee's identity, job ownership and shared multiplayer state. Check empty versus occupied forklifts, entering/leaving, reassignment and client replication so old occupant visuals are removed. Do not remove the real assigned employee to hide the symptom. Notes only; no edits or tests performed while paused.
### Employee inspection and assignment through E

User requests walking up to an employee and pressing E to see their information and assigned jobs, then edit assignments directly there. Include a clear look-at interaction prompt and an employee information panel matching the current Foundryline popup/computer UI.

Show the employee's name, employing business, role, assigned machine/vehicle/workstation, current task/status and useful blocked/waiting reason. Allow the owning business to assign/reassign jobs and the relevant equipment/area from this panel, including existing forklift work priorities where applicable. Reuse the established hiring/assignment rules and ERP actions rather than creating a second, inconsistent system. Update office roster and employee panel consistently after a change.

Validate E targeting, range and line of sight; resolve competing prompts for employees in/near machines or forklifts. Preserve normal vehicle and machine safety rules during reassignment, clearly explaining when a change must wait for the current task or load to finish. Respect business ownership and server authority; another player must not gain control of someone else's employees through a local UI. Test assignment persistence, live co-op updates, occupied vehicles, idle/walking employees and multiple machines of the same type. This is a requested future interaction recorded under the current notes-only pause; no implementation/tests performed.