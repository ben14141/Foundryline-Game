# Cut-parts starter shop

Open `/Game/Foundryline/Maps/L_CutPartsStarter`. It is now the editor and game
startup map. `/Game/Foundryline/Maps/L_ConstructionSite` retains the larger tanker
workshop with its original game mode. Both maps share the established flat grass,
foliage, warm daylight and Lumen environment.

The starter has a 28 x 22 m production slab (about 92 x 72 ft), an attached
8 x 10 m office annex with two private offices and a corridor, space for office furniture,
manual windows, personnel doors, one ground-level freight door, parking, sidewalks,
a connected street/driveway and a generous box-semi loading/turning apron. Road,
apron and slab keep the existing common surface height. There is no loading ramp.

Only the plasma/burn table is installed. There are two physical pallet racks:
raw 60 x 120 inch steel sheets and three assigned cut-part shelf positions. One
forklift/driver supplies the table and stores completed pallets, using the existing
authoritative transport/pathing system. Floor paint identifies machine clearance,
forklift circulation, personnel access, raw receiving, outbound pallet staging and
the box-trailer apron. Equipment and shelves remain at actual asset scale.

Press P for the simplified starter production panel, then open the burn-table
computer to select a predetermined part and batch quantity. The seed is paused,
with ten raw sheets, no finished goods, no tanker orders and one ten-bracket
manual batch ready. Run the plant to start. Bracket, rail and shell blanks retain
the existing fixed cutting times/steel weights; there is no bending or rolling.
Finished parts are automatically stored in their assigned output rack.

This is the starter production/location foundation. Customer contracts, prices,
sales payments, outbound pallet dispatch, an animated box trailer and its loading
mechanic are not implemented by this map. The staging area reserves their space;
it does not pretend to ship or sell stock. No tanker assembly/welding systems,
additional machines or tanker vehicle spawn in this level.

The normal plant commands use server validation and replicate inventory and
production to peers. The starter saves to `FoundrylineCutParts01`, separately from
`FoundrylineFactory01`; construction-only default saves use `CutPartsProperty01`.
Plant loads reject a save from another scenario. The legacy map/environment data
and imported vendor content remain in their existing folders.

Validation: CutPartsStarterBuild.log succeeded; CutPartsStarterTests.log records
78/78 automation cases passing, including two-office/external/internal door layout
and a raw-sheet-to-ten-cut-parts forklift/rack production cycle with steel mass
conserved. CutPartsStarterNetworkQA.json has all ten checks true, covering four
players, client operation, actual batch completion, matching inventory, separate
save/load and a fifth late join. Native rendered scene review is tracked separately
in CutPartsStarterVisualQA.json with CutPartsStarter_* screenshots.


Final native review: all five checks passed with no errors. The two-office and shop screenshots were visually inspected. The new starter is saved with ten raw sheets and no finished parts, paused, and left open with the normal controllable top-down pawn restored. Offices are currently empty; the existing large-office furniture preset intentionally does not populate these smaller rooms.
