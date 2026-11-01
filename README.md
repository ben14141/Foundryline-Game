# Foundryline

Unreal Engine 5.8 C++ manufacturing/business-management game with first-person office ERP, construction Property View, company-owned employees and cooperative multiplayer foundations.

The current starter loop buys steel sheets, cuts brackets and plates on a plasma table, automatically handles pallets using assigned forklift drivers, schedules freight, and pays customer orders on delivery. Employees are hired free for testing, have persistent random names, and receive specific machine, forklift or welding-zone jobs. Handyman character visuals provide walking and work poses.

## Play on Windows (Caleb / playtesters)

After pulling main, double-click **Play-Foundryline.cmd**. It downloads and
verifies the complete packaged Windows playtest, then starts the game. No Unreal
Editor or private asset backup is needed for this path. See
[Windows playtest instructions](Docs/PLAY_WITHOUT_UNREAL.md).

## Editable source content

**This is a public source repository with a reviewed original-equipment asset subset.** Licensed Fab assets and most Unreal content/local source art are excluded. Original plasma, brake and roller geometry/CNC assets are included, along with [original headache-rack, bench/fixture, cradle and rack-level model packages](Docs/Architecture/ORIGINAL_MODEL_ASSETS.md); see [equipment asset publication](Docs/Architecture/EQUIPMENT_ASSET_PUBLICATION.md). Opening a fresh clone in Unreal requires authorized content restoration; use the packaged download above to play. See [repository content and dependencies](Docs/REPOSITORY_CONTENT.md). Existing local content is preserved.

## Build and play

With Unreal Engine 5.8 and the Visual Studio C++ Unreal toolchain installed, generate project files from `Foundryline.uproject` and build `FoundrylineEditor` for Development / Win64. Restore authorized content before opening the main menu `/Game/Foundryline/Maps/L_MainMenu` or starter map `/Game/Foundryline/Maps/L_FirstShopStarter`.

Play starts in first person near the office computer. Press E to open ERP; Property View is available through that computer. Team supports free hiring and explicit job/equipment assignment. ERP provides 1x/4x/8x simulation controls. Start paused, accept a basic order, buy steel, program the plasma, then book finished pallets on the loading tablet.

## Implementation and acceptance

- [Architecture and boundaries](Docs/ARCHITECTURE.md)
- [Starter brackets and plates](Docs/Architecture/STARTER_BRACKETS_AND_PLATES.md)
- [Employee jobs](Docs/Architecture/EMPLOYEE_JOBS_ACCEPTANCE.md)
- [Handyman employee integration](Docs/Architecture/HANDYMAN_EMPLOYEES.md)
- [Player collision](Docs/Architecture/PLAYER_COLLISION.md)

These documents distinguish source/build checks, local automation, rendered playtesting and multiplayer acceptance. Local editor checks do not establish packaged internet multiplayer, crowd scale or an FPS guarantee. Production expansion, economic recovery, payroll, research, vehicle yielding and roller-console reach remain incomplete.
