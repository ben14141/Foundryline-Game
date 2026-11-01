# Caleb / Unreal collaborator setup

The C++ source repository is public. The complete editable Unreal Content
directory is in **ben14141/Foundryline-Content**, a private Git LFS repository.
CalebM61 has been invited with write permission. Accept that repository invitation
while signed into that account; existing source-repository access is separate.

1. Install Git LFS, Unreal Engine 5.8 and the Visual Studio C++ Unreal toolchain.
2. Pull main in Foundryline-Game. Close Foundryline if it is running.
3. Double-click **Restore-Unreal-Content.cmd**. Authenticate Git to your authorized
   GitHub account if prompted. This clones/pulls the private content repository
   into a sibling `Foundryline Content` folder and restores real assets into the
   game checkout. The initial download is approximately 3 GB.
4. Generate project files for Foundryline.uproject, build FoundrylineEditor
   Development / Win64, then open it with Unreal 5.8. Main menu:
   `/Game/Foundryline/Maps/L_MainMenu`; starter shop:
   `/Game/Foundryline/Maps/L_FirstShopStarter`.

The restore helper rejects unresolved LFS pointers, refuses to sync while this
project is open, preserves changed destination files under
Saved/ContentSyncBackups, and never deletes destination files. A failed/private
download stops before copying into the game. It does not alter other projects.

## Editing and sharing

Coordinate ownership before editing maps or other binary assets. Do not edit the
same Unreal map simultaneously: binary assets do not merge like C++.

Close Unreal after saving edits. Run:

```powershell
.\Tools\Release\Sync-FoundrylineContent.ps1 -Publish
```

This copies edited Content back into the sibling private repo for review; it
does not commit or push automatically. Review its Git diff/status, then commit
and push there. Commit C++/config/docs separately in Foundryline-Game. Tell the
other developer which source and content commits belong together. Restore from
the private repo after pulling the corresponding source changes.

Sync is intentionally non-deleting. Asset removals and renames need explicit
coordination, Unreal redirector cleanup, and review in both checkouts. Avoid
simultaneous edits or blind restores over your own unpublished work; backups
preserve replaced files but do not resolve semantic conflicts.

Do not force-add licensed Content to the public source repo. This private
content is for working on Foundryline only, under the applicable asset licenses.
Editable Blender source art outside Content remains a separate authoring
handoff; it is not needed to open and edit the game in Unreal.
