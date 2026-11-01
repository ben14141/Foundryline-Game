# Windows playtest release — October 31, 2026

## Problem and delivery

The GitHub repository is public and excludes licensed Fab source content and
vendor-dependent maps. A clone can compile while lacking the assets needed to
play. Adding those editable assets to the public repo would not solve this
appropriately. Playtesters now use the compiled Windows game with incorporated
cooked content, distributed as a GitHub Release download.

Play-Foundryline.cmd invokes Tools/Release/Play-Foundryline.ps1. It downloads the
versioned archive and checksum, checks SHA-256 before extraction, installs under
LocalAppData/Foundryline/Playtest, and launches the executable. The installed
release/asset IDs avoid downloading the same archive on subsequent launches.
Editable content restoration remains a separate developer workflow.

## Packaging fixes

- Reordered ERPViewModel and OperationFeedback includes so the corresponding
  header comes first, as required by the non-editor target.
- Replaced the pavement automation test's INFINITY macro with the standard
  numeric_limits infinity value to avoid MSVC constant-overflow failure.
- Added a reproducible Windows packaging entry point. It uses the existing
  editor binary for cooking rather than rebuilding an editor being used by
  another project. Game code is built normally.
- Cooked all local content for this initial playtest because several runtime
  assets are loaded by string path and are not discoverable from map references.
  Future releases can narrow the cook manifest independently.
- Release archive excludes PDB debug symbols and build manifests. It includes
  cooked containers, game/engine runtime files, notices and the x64 Microsoft
  redistributable. No editable uasset, umap, FBX or Blender sources are published.

## Verification

BuildCookRun completed successfully for Development / Win64 with cook, stage,
Pak/IoStore compression and archive. The first standalone game-target build
caught the include-order and constant-overflow issues above; the corrected
game target and full cook succeeded.

Launched the packaged executable with a separate UserDir. Used normal UI input
to start New Game, press E at the office computer and enter Planner. Visually
confirmed the main-menu artwork, first-person office, ERP images, shop/plasma,
racks, forklift, employee and exterior vegetation. No missing game package or
failed game-object-load errors appeared in the smoke-test log.

Build evidence: Saved/Logs/WindowsPlaytestPackage.log.
Runtime evidence: Saved/Logs/PackagedSmoke.log. Normal player saves were not
changed. This is packaged startup/interaction acceptance, not a repeat of the
full production/economy suite or a test on Caleb's hardware. Internet
multiplayer, NAT traversal and an FPS guarantee remain unverified.

The requested employee/plasma/inventory UI notes remain deferred.

The published archive was subsequently downloaded through the actual launcher
under Windows PowerShell 5.1 into a separate installation directory. SHA-256
verification and extraction succeeded. A second invocation reused the existing
installation. The archive contains no editable assets or PDB symbols.

## Editable collaborator handoff

The user clarified that Caleb will also develop in Unreal. Created the private
ben14141/Foundryline-Content companion repository and issued CalebM61 a write
invitation, matching his existing write role in the public source repository.
The complete editable Content snapshot is stored there through Git LFS.
Restore-Unreal-Content.cmd and Sync-FoundrylineContent.ps1 provide the developer
workflow; see COLLABORATOR_SETUP.md. Invitation acceptance must be performed by
Caleb, and access cannot be validated using his credentials from this machine.
