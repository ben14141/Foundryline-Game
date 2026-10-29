# First-scene camera and grid interaction

Status: source/configuration written; compilation, Unreal automation tests and
in-editor feel/visual verification are deferred. No map has been authored yet.

FoundrylineGameMode chooses FoundrylineCameraPawn and FoundrylinePlayerController.
DefaultEngine.ini sets that game mode project-wide; map-specific game-mode overrides
can take precedence. No default map points to a nonexistent project asset.

## Controls

| Input | Action |
| --- | --- |
| WASD / arrow keys | Pan relative to camera yaw |
| Q / E | Rotate |
| Hold middle mouse and move horizontally | Rotate |
| Mouse wheel | Smooth zoom |
| G | Toggle bounded development grid preview |
| 1 / 2 / 3 / 4 | Snap at 100 / 25 / 10 / 5 cm |
| Left click | Broadcast selected grid anchor; does not build anything |

Initial input uses native PlayerInput/InputComponent mappings in DefaultInput.ini.
No Enhanced Input integration is claimed. Future remapping/UI can replace this
adapter without changing the authoritative construction APIs.

## Behaviour and limits

- Perspective camera, 55-degree downward pitch, 45-degree initial yaw, 50-degree FOV.
- Default arm distance 80 m, editable 5 m to 1,500 m zoom range. These are prototype
  tuning values, not locked game dimensions. Minimum zoom may need adjustment for
  equipment close-ups during visual testing.
- Movement speed scales with zoom, with normalized diagonal movement.
- Pan limits default to +/-5 km in XY and are editable independently of ownership.
  These camera limits do not create a map or make land buildable.
- First map assumes a flat Z=0 plane. Cursor rays intersect that plane without
  requiring ground collision, so props never displace the construction grid.
- Grid addresses are snapped centimetre coordinates. The local preview draws only
  42 lines around the cursor plus one highlighted square; it is not a town-wide grid.
- Debug drawing is a development aid and may be absent in shipping builds. Production
  grid/hologram materials remain pending.
- Camera and selection are local presentation. No RPC, inventory, spending or object
  placement is triggered by hover, camera movement or clicking a cell.
- The camera pawn is not replicated. A multiplayer adapter must create/possess each
  client's local camera; current game-mode spawning is for standalone prototyping.
- Full UI focus handling, object picking, edge pan, touch/gamepad support and elevated
  terrain are deferred. Verify mouse release/focus behaviour in the editor.

## Acceptance when the build is available

1. Build FoundrylineEditor and run the Foundryline automation group.
2. Create/open the new project map; ensure the map does not override the game mode.
3. Verify controls, smooth zoom extremes and cursor picking at multiple rotations.
4. Confirm camera movement doesn't change placement snapshots or world revision.
5. Check input over future UI panels and test window focus/capture transitions.
6. Replace debug grid drawing with the production material and measure frame time.
