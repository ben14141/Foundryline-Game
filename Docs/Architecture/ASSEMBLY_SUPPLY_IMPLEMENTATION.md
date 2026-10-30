# Assembly supply and planning equipment

## Player flow

Start in first person. Use the office computer to enter Planning View; pause production before buying or placing equipment. Open **Equipment Catalog**, select a fabrication station, blue parts rack, green finished-goods rack or forklift, position its hologram, rotate with R, and place it. Rack heights have one, two or three levels. Invalid placement and insufficient funds leave cash untouched. Rack access previews reserve a 12-foot loading area; access areas can intersect, solid rack bodies cannot block them.

A fabrication station includes its welding bench, welder equipment, worker positions, bulky-frame IN position, Assembly Supply Bin and OUT pallets. The bin is part of the station hologram. Upgrade it from four to six or eight compartments while production is paused. Each compartment holds one part type and up to 100 lb; capacity is 400/600/800 lb overall. New headache-rack output pallets hold four assemblies, while existing five-stack saves remain intact.

Hire a named employee in ERP Team, choose Parts Collector, and assign the assembly station. The collector requests missing recipe ingredients automatically, reserves source stock, pushes a mixed-load cart from blue storage, and fills the bin. Cart limits are 300 lb and 100 units. Frames continue arriving by forklift. One area currently corresponds to one station; multiple collectors can serve it. Removing an assignment finishes safe cargo handling and returns the employee home.

## Architecture and presentation

The server validates business access, purchases, assignments, stock reservations, payloads and bin capacity. Snapshot/save data includes cargo in transit. Material mass counts stock on pallets, carts and in buffers exactly once. Rendering uses bounded representative contents rather than an actor per part.

The original collector cart has independent wheel and caster meshes, hand contact with the handle, walking/pushing presentation and a loading reach. Bins and cart bodies block first-person movement. The cart uses open-floor navigation with clearance, yields to forklifts at crossings, and can retreat from an occupied crossing approach.

Source models are under `SourceAssets/Production/AssemblySupply`; independent wide-bay storage is under `SourceAssets/Inventory/Storage/WideBayPalletRacking`. Runtime assets are under the matching `Content/Foundryline` paths. Rebuild/import scripts are under `Tools/Blender` and `Tools/Unreal`.

## Scope and tuning

Purchase prices and payroll/economy values are provisional. The completed two-order automation demonstrates a solvent production and delivery loop, not balanced profitability. The existing fixed recipe catalog remains; a full data-table migration is separate work. The broader finished-goods rack tablet/direct-shipping redesign remains a separate plan. Multiplayer acceptance does not establish a production player cap or a 60 FPS performance result.

Validation reports and screenshots are in `Saved/Logs/AssemblySupply*` and `Saved/Screenshots/Reviews/AssemblySupply*`. See the detailed [collector plan](../Planning/ASSEMBLY_SUPPLY_COLLECTOR_PLAN.md) for invariants and required coverage.

## Acceptance record — 29 October 2026

- Development Editor compilation passed in the isolated checkout and primary `C:/Users/curly/Documents/Foundryline Game` project.
- Assembly Supply: 8/8 automation tests passed (capacity/upgrades, mixed-trip reservations/delivery, in-transit save/load, migration/ownership, legacy stock, animation grips, crossing retreat, atomic purchases).
- Welding Station: 9/9 regression tests passed after the final mirrored IN layout correction, including two fulfilled orders totalling seven racks, material resupply, two welders and OUT capacity.
- Employees: 15/15 tests passed against the primary project's compiled module.
- Multiplayer: 10/10 runtime checks passed with four players and a fifth late join. Competing assignment produced one collector, foreign-business targeting did not reassign it, all five paused snapshots matched, and bin visuals appeared on all peers.
- Original cart, bin and wide-bay rack imports passed material/scale checks.

These checks do not establish production balance or a performance target. No new changes were pushed or merged.

Rendered review passed with no script errors. The station bin, moving collector and Planning View catalog screenshots were visually inspected after correcting the review cameras.

Pricing was subsequently adjusted in source without rebuilding or retesting, at the user's request. See [collector-era pricing](../Planning/PRICING_COLLECTOR_UPDATE.md). The acceptance results above apply to the pre-adjustment build.
