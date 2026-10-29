# Starter material handling

The starter shop now contains a dedicated forklift, sheet storage rack, two sheet loads and a three-roll plate roller. Press **K** to enter or leave the forklift test mode. This is a playable material-handling prototype; employee scheduling, purchasing, general equipment placement and property-save integration remain future work. Restarting Play resets this cell.

## Controls

| Input | Action |
| --- | --- |
| K | Enter / leave material handling |
| WASD | Drive / rear-wheel steering |
| Q / E | Lower / raise forks |
| Space | Pick up a nearby load / set it on the floor |
| 1 / 2 / 3 | Store the carried load on the corresponding rack level |
| 4 | Place the carried load at the roller input |
| R | Roll one sheet from the input load |

Approach the long side of the paired pallets, face the load and press Space. Fork height aligns during transfer. Move near the rack or input before using the destination button. Occupied destinations reject transfers. Lifted loads reduce driving speed. Only the current driver can operate the forklift; leave with K to hand it to another player.

## Scale and inventory

One stock size: **60 × 120 × 1/8 inch** (152.4 × 304.8 × 0.3175 cm), carbon steel at 7,850 kg/m³. The thickness is the starter assumption, kept in a shared definition. Each sheet weighs approximately 255 lb; the UI reports actual stock weight. Two original-size pallets support a load rather than stretching the vendor pallet.

Each initial load contains ten sheets. At most three sheet instances represent a load; their heights reflect the full stack. Stock IDs, quantities and locations are independent of those instances. The roller reserves exactly one sheet when a job starts and produces one curved blank after six seconds. It is a prototype forming operation, not a complete tanker recipe or industrial machine simulation.

## Asset organization

- `SourceAssets/MaterialHandling/Authored/Foundryline_MaterialHandling.blend`: dedicated animated review scene.
- `SourceAssets/MaterialHandling/manifest.json`: pivots, dimensions, source FBX files and Unreal destinations.
- `/Game/Foundryline/Vehicles/Forklift/FL01`: forklift body, mast, carriage and four wheels.
- `/Game/Foundryline/Inventory/Storage`: textured pallets and sheet racking.
- `/Game/Foundryline/Inventory/Materials/Steel`: flat stock sheet.
- `/Game/Foundryline/Production/Equipment/SheetRoller`: machine body and three independent rollers.
- `/Game/Foundryline/Products/Components/Steel`: curved sheet output.
- `/Game/Foundryline/Shared/Materials/MaterialHandling`: game-owned materials and texture maps.

Factory source assets remain unchanged. The derivative forklift has Foundryline paint, rubber tires, detailed hubs/treads, lighting details and separate moving parts. Pallets use authored wood grain; stock uses brushed steel; racks and machinery use powdercoat and steel finishes.

The roller shafts are 3.52 m long, doubled from the original 1.76 m. End frames and drive components are moved outward without stretching their proportions. A separate operator computer stand has a monitor, keyboard, mouse, emergency stop and modeled control/monitor cables. Bearing caps, grease fittings, frame ribs, anchors, guard vents and warning plates add machine detail. The computer display is currently visual; the material-handling HUD operates the prototype.

## Multiplayer and validation

The server owns driver claims, stock transfers and roller jobs. Forklift movement uses bounded fixed steps; presentation interpolates replicated poses. Quantities and output replicate to late joiners. This uses kinematic prototype driving, not a physics vehicle or autonomous warehouse traffic.

Validation artifacts are under `Saved/Logs`: `MaterialHandlingAutomation.log`, `MaterialHandlingNetworkQA.json`, `MaterialHandlingBlenderQA.json` and `MaterialHandlingImport.json`. The network fixture relocates the forklift between transfer stations to isolate rack/input tests; it does not demonstrate autonomous navigation. Four initial players plus a fifth late joiner were checked for movement, exclusive control, pickup, rack transfer, roller consumption, output and driver handoff.
