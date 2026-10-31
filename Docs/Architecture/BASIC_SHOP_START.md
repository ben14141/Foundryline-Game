# Basic shop start

Map: `/Game/Foundryline/Maps/L_BasicShopStarter`.

The standard empty shell has an approximately 79 x 59 ft Tier 1 shop, an attached 26 x 33 ft open office, automatic connecting/exterior office doors, one manually authored office window, a shop personnel door, a forklift garage opening and one small-truck shipping bay. Normal roof/skylight/recessed-office-light generation applies.

Outside are public frontage road, parking, its driveway, a paved shipping yard, truck access and pedestrian approaches. Shop floors and pavement retain the existing common grade. The small shipping bay uses the existing integrated door/bollards/apron/paint preset. All ordinary construction remains selectable, editable and replicated through the authoritative site records.

No demonstration machines, racking, vehicles, employees or production furniture are seeded. A three-monitor office computer desk is placed in the existing attached office. Players start in first person at that desk; E opens ERP, and Property View grants the normal server-validated planning access. An empty owned factory backend supplies operations and purchases without seeding machines, inventory, staff or floor-zone paint. The full production starter and road-only start remain separate maps. Default project map settings remain unchanged.

Launch with `Tools/Launch-BasicShop.ps1`.

Startup validation evidence: `Saved/Logs/BasicShopBuild3.log`, `Saved/Logs/BasicShopStartQA.json` and `BasicShopSetup2.log`. Checks cover the shop/office shell count, no equipment actors, one small shipping bay, twelve construction sections/surfaces and identical placement digests across four players. This is scenario startup validation, not a new full regression or performance run.


Desk/planning acceptance: `Tools/Unreal/review_basic_shop_desk.py` exercises office startup, ERP access, planning, client construction/removal and return to the desk across four players. Results are in `Saved/Logs/BasicShopDeskQA.json`; build log: `BasicShopDeskBuild.log`.

Desk acceptance 2026-10-30: all eight native checks passed across four players (`BasicShopDeskQA.json`, `BasicShopDeskReview2.log`): first-person desk spawn, empty shop, replicated physical workstation, ERP opening, validated planning access, client placement/removal and return to first person. The empty machine-less commerce state is now accepted only with no machine recipes and no owned press brake; ordinary fixed-catalog validation remains enforced. A regression test verifies purchasing a rack in that state and rejecting incomplete recipe catalogs.

Final desk build: `BasicShopDeskBuild5.log` succeeded. Commerce regression suite: 13 passed, zero failed, including the empty-business rack purchase and incomplete-recipe rejection test (`Saved/Automation/Focused-Commerce/index.json`). Normal standalone play preferences restored; reopened basic shop in first person beside the office workstation.
