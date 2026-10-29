# Welding hood integration candidate — NOT LIVE

This folder is a source handoff, deliberately outside live `Source/`. No Unreal editor, commandlet, build, import, live employee code, save schema or network contract was changed to prepare it. Claude's active Unreal/code work was left alone.

## Intended behavior

- An employee assigned to the Welder role wears the harness/hood. Walking, waiting, idle and paused welders keep the visor raised.
- The visor closes only for actual `Activity=Welding && bWorking && !bMoving && !bSeated`; an assigned job alone is insufficient. A short cosmetic hinge transition follows that target without changing production progress or VFX authority.
- Operator, forklift driver and unassigned roles hide the hood and restore the employee's original normal outfit. No separate employee or head actor is created.
- Existing baseball caps cannot fit a realistic raised/closed shell. Build at most three shared transient no-hat meshes from the same original `Appearance.Outfits` parts, filtering ONLY `SK_Hat_*`. Keep the original head, beard, clothing, ID, variant index, animation sampler and company key. No vendor head or entire merged body is hidden to remove the hat.
- Headgear follows the actual evaluated poseable `head` bone after `CommitHandymanPose`, including character turn/lean. It does not follow a hat socket or a fixed employee-root offset.
- Dedicated servers never load or instantiate the cosmetic hood. Standing capsule, jobs, inventory, wages, ownership and save data remain unchanged. Each visible hood is one non-ticking scene component and two non-colliding static-mesh children.

## Candidate files

`FoundrylineWeldingHoodComponent.h/.cpp` are reviewable source copies. `employee_hook_candidate.diff` contains those two new files and four small existing-file integration changes: a transient shared outfit cache, employee hood pointer, body selection/head update, and accurate outfit reporting. `git apply --check` passes against main at commit0d166f0929af95dd21c51b74d1c021e47331cc1e as inspected. This check does NOT compile or execute the code.

Do not apply the patch automatically while another branch is active. Claude's `welding/phase5-commercial` had no diff/dirty changes in the visual hook files at inspection, but `polish/visual-tidy` DOES modify `FoundrylineHandymanPresentation.cpp`. Reconcile the hook with that final presentation code rather than overwriting its pose changes.

## Art contract

Expected original meshes:

- `/Game/Foundryline/Employees/Equipment/WeldingHood/SM_WeldingHood_Harness`
- `/Game/Foundryline/Employees/Equipment/WeldingHood/SM_WeldingHood_Visor`

Both mesh origins are the ear hinge. Neutral native UE axes: front+Y, up+Z. Head-relative root translation in centimetres is `(14.55806393,-1.85629664,0.00112296)`; relative quaternion `(x,y,z,w)=(-0.02512077,-0.70663461,0.02516714,-0.70668457)`. Visor pivots at local0 around native+X: closed0°, raised105°. These values must agree with the art agent's final attachment metadata before implementation.

Missing assets or failed no-hat assembly leave the normal body intact and the hood hidden; they never remove the person's head. Runtime skeletal merges must not be saved directly as editor meshes because their editor ImportedModel is absent.

## Required next acceptance when an Unreal lease is authorized

Compile the integrated patch; import only original hood assets. Test all three stable outfit indices, hat removal without head/beard loss, original outfit restored on release/reassignment, actual head following while walking/leaning/turning, visor closed on real welding, visor raised during wait/pause, no duplicate body, no collision/nav changes, dedicated cosmetic skip and same cosmetic state on co-op/late join. Update the old test that asserted one identical mesh pointer across all roles: the welder intentionally selects a no-cap version while preserving the same person/variant; leaving the role must restore the exact original mesh.

At this delivery there is NO claim that employees already wear the hood inside Unreal. The original Blender fitted review and asset-fit measurements establish model readiness; native attachment/animation acceptance remains pending a safe engine window.
