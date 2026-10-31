# Foundryline title screen

The title screen is isolated in `FoundrylineMainMenu.h/.cpp` and `L_MainMenu`.
It does not instantiate the gameplay game mode, production simulation or shop.
The supplied factory artwork is a background only; buttons are live Canvas hit
boxes. The title logo uses the user's transparent white/orange PNG, imported as
`T_MainMenuLogo` with UI compression and no mipmaps. It is drawn as a texture;
the earlier Canvas material conversion is no longer used.

Button text uses Bebas Neue Regular with tracking, replacing Roboto Bold Condensed.
The font and SIL Open Font License are bundled under `Content/Foundryline/UI/Fonts`
and explicitly staged as UFS for packaged builds. Source:
https://github.com/google/fonts/tree/main/ofl/bebasneue
Hover text is sampled from the supplied reference (`#F0AF6D`), with the darker
orange selection line (`#E9974A`) and translucent fading highlight behind it.

## Setup and launch

1. Build FoundrylineEditor.
2. Run `Tools/Unreal/create_main_menu.py` with Unreal's Python runner to import
   `SourceAssets/UI/MainMenu/MainMenuReference.png` and create `L_MainMenu`.
3. Run `Tools/Launch-MainMenu.ps1`.

New Game opens `/Game/Foundryline/Maps/L_FirstShopStarter` with the normal
Foundryline game mode. Host Multiplayer Shop opens the same map as a listen
server. It does not implement session discovery, Steam invites or a join browser.
Each action travels with absolute options so the menu game mode cannot leak into
the playable shop. Settings offers windowed 1280x720 or borderless fullscreen.
Quit exits the standalone game.

Continue and Load Game are visibly disabled until complete save restoration is
connected to this front end. Starting a new shop does not delete existing saves.
The existing direct first-shop launcher is retained.

The editor's startup map remains the first shop, so opening the editor still
supports the existing development workflow.

Normal game startup uses `L_MainMenu` through `GameDefaultMap`; both title and
first-shop maps plus the menu/brand art are explicitly included for cooking.

## Local validation, October 30, 2026

FoundrylineEditor compiled successfully. The isolated map/art creation commandlet
completed with zero errors or warnings. A rendered standalone menu was inspected;
clicking its New Game button loaded `L_FirstShopStarter` in first person at the
office desk with the normal gameplay HUD and interaction prompt. Multiplayer
hosting is wired but no additional peer-connection test was performed here.

Live acceptance repeated October 30 at 22:36 local: the real New Game hit box was
clicked through Windows computer use. The standalone process opened the starter
shop in first person with the office-computer interaction prompt and normal
gameplay HUD. Menu and starter screenshots are recorded in
Saved/Logs/MainMenuNativeAcceptance.json. The playable shop was left open.
Only the three temporarily modified editor play preferences were restored to
Standalone, one client and run under one process. No menu source change was needed.
