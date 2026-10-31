# First-person presentation, employees, interface audio and flatbed freight

## Scope

This update keeps server-owned business, inventory, orders and freight separate from local rendering and interface feedback. It adds no payroll or progression changes. Source and vendor assets remain separate. Licensed Handyman, Softline and Megapoly-derived assets stay in the private local content tree.

## Employees

`Tools/Unreal/activate_handyman.py` now selects the Handyman smooth-edge (`Handyman_SE`) mesh and parts. All three existing outfits retain their identities and the same idle/walk animations, 110 cm stride, facing offset, employee assignments, pose controls and welding hood behaviour. The game-owned `DA_HandymanEmployees` profile changes; vendor packages are untouched. Each merged outfit was checked for nonzero render vertices and a complete skeleton.

## First-person interface

The walking HUD uses a light paper background, graphite type, orange accents and the original logo. Its scale is based on a 1440 x 900 reference, separately from the construction sidebar. The company card shows the balance and date. A small outlined reticle remains visible against dark and bright materials. The bottom control strip gives walking, looking and inspection controls, with an office planning reminder above it.

Doors, the office computer, loading tablets and machine computers use one consistent action card near the lower centre of the screen. The card contains an E key, a fitted action title and a secondary purpose label. It leaves the object under the reticle visible. Existing interaction permissions and camera/roof rules still decide what can be opened.

## Softline feedback

The 50 Softline assets were already installed. The update verifies/imports them without replacing existing assets and connects the Clean Paper theme to the interface:

| Action | Feedback |
| --- | --- |
| Enter an enabled button | Quiet hover |
| Activate a button | Click |
| Change a tab/page/category | Tab switch |
| Open an office, tablet or machine screen | Popup open |
| Close a screen | Cancel/close |
| Receive an accepted commerce command | Confirm |
| Receive a commerce or factory error | Error |

`UFoundrylineUIAudio` belongs to the local player. It skips nonlocal controllers and dedicated servers. Sounds do not replicate. A four-voice concurrency group bounds overlap; hover feedback has a 90 ms interval and stays quiet for 140 ms after a stronger action. Repeat action cues have a 100 ms interval. Sound levels range from 0.12 for hover to 0.28 for clicks/confirmations. They remain independent from spatial manufacturing audio. `PlaybackReport` exposes local counts and resolved asset paths for development checks.

## Flatbed carrier

Carrier ID 4 is **Ironline Flatbed Freight / 53 ft flatbed semi**, added after the four existing choices so saved carrier IDs retain their meaning. Its configured capacity is 26 pallet spaces and 48,000 lb, with a $225 pickup fee, 90-minute lead time and 240-minute transit time. A load still has to match an accepted order, exact staged quantities, available funds, physical product fit, pallet space and payload limits. Starter staging remains small; the new option does not grant extra stock or replace the two-pallet starter hauler.

The native asset uses Claude's separate 1:1 body, four wheel meshes and two landing gear meshes. Its 53 ft deck is 102 inches wide; deck elevation is 147 cm above the vehicle ground plane. The tractor uses the existing modular Megapoly semi. The fifth wheel remains attached to the kingpin; the trailer follows with bounded articulation, rotating wheels and steering. Landing legs stay raised while attached. Cosmetic components are built locally on every peer from replicated carrier/cargo state.

Pallets load from the rear row forward. Each ordinary row holds two pallets; a headache-rack pallet uses the whole row. The server's forklift destination and the displayed deck position use the same scale and rear-first coordinates. Loaded pallets retain their reserved slot while other pallets are still being loaded. Existing bay scheduling, loading jobs, payment terms and save/restore validation remain authoritative. A development-only fixture allows inspecting the visual rig without granting gameplay inventory.

The articulated vehicle is a visual carrier driven by booking routes, not a new player-controlled physics vehicle. This update does not establish swept clearance for every custom street or guarantee capacity for a full 26-pallet load in the starter site's two-pallet staging area. Larger site layouts still need suitable pavement and forklift access.

## Floor markings

Only forklift lanes are baked onto the ground. Personnel lane borders, PEOPLE labels and crossing stripes are removed, and the walkway/crossing paint buttons are hidden. Forklift joins use only forklift geometry, so an invisible walking route does not cut a hole in the yellow line. Work zones, IN/OUT bays and storage paint remain. Existing walking route records remain available to employee navigation and saved games.

## Focused validation

From the repository root:

```powershell
.\Tools\Validation\Invoke-FoundrylineCheck.ps1 -Profile Presentation -Assets
.\Tools\Validation\Invoke-FoundrylineCheck.ps1 -Profile Commerce
.\Tools\Validation\Invoke-FoundrylineCheck.ps1 -Profile Employees
.\Tools\Validation\Invoke-FoundrylineCheck.ps1 -Profile Full
```

The runner builds only when source timestamps are newer than the project DLL, batches test prefixes into one editor process, exports machine-readable results, verifies fresh reports, applies a timeout to its own process and records timings. `-NoBuild` explicitly skips compilation. `-Assets` also performs one combined smooth-model, Softline and flatbed-material audit. Documentation-only edits need no engine run. Changes to networked state additionally use the rendered four-peer review; visual changes use screenshots rather than repeatedly replaying a complete sales cycle.

`Tools/Unreal/review_presentation_update.py` creates an unsaved review fixture, checks four-peer presentation, walking input/roofs, local Softline feedback and hover throttling, then captures the first-person prompt, shipping choice and assembled flatbed. Reports live under `Saved/Logs`; screenshots under `Saved/Screenshots/Reviews`. Review fixtures never save the map.

## Acceptance recorded 29 October 2026

- Primary project compilation succeeded; incremental builds completed successfully.
- Focused employee and commerce/flatbed tests: 27 passed.
- Combined asset audit: 4 checks passed, including all 50 Softline assets and seven material-complete flatbed meshes.
- Four-player rendered review: 11 checks passed, no reported script/runtime errors. Checked local-only UI sound counts and throttling, smooth employee outfits, first-person input/roofs, wheel rotation and connected tractor/trailer geometry.
- First-person action card, employee, floor markings and in-game flatbed screenshots inspected.
- Example measured focused run: build 23.68 seconds plus tests 15.16 seconds, total 38.87 seconds. These are run timings, not a claimed percentage performance gain.
