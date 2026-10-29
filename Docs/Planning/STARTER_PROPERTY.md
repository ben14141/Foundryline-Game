# Starter property and public street

The runtime scene now starts on Foundry Lane / Lot 01 rather than the large construction showcase. The showcase seed remains available through bLargeShowcase for development tests.

- Property: 100 x 80 m, survey boundary markings, 3 m building setback.
- Shop: 24 x 18 m, 6 m walls, personnel door, garage opening and 5 x 5 m semi opening.
- Car access: 6 m driveway, 22 x 17 m parking area, raised walking strip to the personnel entrance.
- Freight: separate 12 m approach, 50 x 36 m yard, fixed-size 4 x 18 m trailer staging bays and a 30 m clear manoeuvring apron.
- Expansion: unbuilt land remains behind and beside the shop.
- Public frontage: separately owned two-lane road. Property owners cannot select or alter it. The player can build paving within their lot; building shells must also fit the inset building envelope.

The Factory pack has modular surfaces rather than one complete street actor. The public street now uses native SM_Ground_400 tiles at 1:1 scale with their authored material/vertex colors, together with the existing Factory sidewalk and curb modules. Vendor packages remain unchanged. Raised collision substructure and line markings are game-owned construction presentation.

Lot ownership, full bounds and inset building bounds serialize as authoritative build regions. UI and generated boundary markers consume those definitions. Existing saves without a setback keep their previous unrestricted region behavior. Buying lots and network transport are future work.

Truck access and Semi Yard tools are available under Roads. Yard placement requires at least 48 x 24 m; widths reserve an 18 m trailer strip plus 30 m apron. This is a visual/geometry foundation, not a swept-path or traffic simulation acceptance. Dynamic vehicle routing remains future work.

## Loading dock and office

The starter shop has a wall-snapped, grade-level loading bay. Shop, driveway, truck yard and parking share a 20 cm finished surface. There is no loading platform, depressed yard or internal/external loading ramp. The 3.5 x 3.5 m opening has wall bumpers, protective bollards, edge markings and a canopy. LoadingDock remains the saved opening kind for compatibility. The exterior apron must fit the owned lot and remain clear of other buildings.

Loading Dock is available under Doors. It uses wall-edge/metre snap, full hologram preview, ownership validation, relocation, removal and undo. Unloading simulation, truck docking AI and overhead-door animation remain future work.

The 16 x 12 m office annex attaches to the shop's north wall and retains both required entrances. It has an 8 x 8 m furnished private room and open circulation space. Offices and rooms use the existing rectangle-drag placement; room doors/windows remain manual. Office interiors remain off-white. Ceiling lights now automatically cover office rooms, open spaces and corridors.

The pedestrian path is now a saved Walkway construction item rather than starter-only geometry. It snaps to the 1 m grid and can be placed, selected, moved, removed and undone. Lot survey marks represent property geometry rather than buildable props.


## Midday, landscaping and ceiling lights

The construction map uses 75-degree midday sun, 100,000 lux and a gently warm 5400 K sun. Lumen remains enabled. Parking landscaping generates 3 m ornamental birches and 35 cm flower clumps in the existing beds, skips connections and participates in the parking hologram and landscaping toggle. Flower and tree distance culling is configured; performance is not measured.

Shop tiers use native Factory SM_Lamp_03 hanging fixtures at unchanged physical size: low bays use approximately 8 m spacing and 30,000 lumens per fixture; high bays approximately 12 m and 100,000 lumens. Compact 1.2 x 0.3 m office ceiling panels stay close to the 3 m ceiling, approximately 4 m spacing and 5,000 lumens. Office room footprints and remaining free rectangles receive independent grids, including hallways. Shop fixtures use 4500 K, office fixtures 4300 K. Shadow-casting rectangular lights respect walls; fixture and light visibility follows the roof preference and is on in first person. Values are presentation tuning, not verified illuminance or FPS measurements.


Interior review refinements: ceiling fixtures now have luminous diffusers backed by shadow-casting area lights. Office panels mount at approximately 2.6 m above finished floor with short ceiling supports. Room and corridor grids remain separate. Indoor/outdoor histogram exposure replaces the former fixed outdoor camera exposure, with a 3-16 EV100 range. The shared office/shop wall uses the interior finish on both occupied sides. Native rendered eye-level views are captured by Tools/Unreal/review_lit_interiors.py; the optional FoundrylineShowcase launch flag loads the existing Tier 2 demonstration instead of the starter property for review. No screenshots change gameplay lighting or image brightness after capture.
