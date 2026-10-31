# Road-only play start

Map: `/Game/Foundryline/Maps/L_RoadOnlyStarter`.

A separate copy of the existing flat forest environment starts with the public two-lane frontage road and an empty marked property. No shop, office, paved yard, machines, racks, forklifts, trucks or computer terminals are seeded. The initial view is the controllable planning camera, so the player can begin construction without an existing office. The normal shop map and default map settings are unchanged.

Use `Tools/Launch-RoadOnly.ps1` to reopen this scenario. Each fresh session seeds the empty start; construction may be saved using the existing property tools. Road-only seeding and omission of demonstration equipment happen on the authoritative game mode; replicated construction and business ownership retain the existing rules.

Validated 2026-10-30: editor build passed (`RoadOnlyBuild.log`); native PIE checks found exactly three road section visuals, no roofs/buildings, no factory/equipment, no loading bays, a ready construction site and the planning camera (`Saved/Logs/RoadOnlyStartQA.json`). A separate standalone game was launched for the user. This was a startup acceptance check, not another full gameplay or multiplayer regression run.
