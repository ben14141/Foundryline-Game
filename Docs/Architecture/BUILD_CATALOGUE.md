# Categorized build catalogue

## Player flow

Use the office computer and enter Property View. Press Q, B or Tab, or click BUILD, to open a centered catalogue. P opens its Production category. Pick a category, search by item name, then click a card. The popup closes and the chosen item becomes its real placement hologram. Right-click cancels the active placement. Escape closes the popup. Search supports typing, Backspace and Enter when exactly one result remains.

Categories:

- Production: fabrication workstation and connected forklift-lane painting; owned stored machines also appear here.
- Storage: standard parts and finished-goods racks, configurable to one, two or three levels.
- Office: office addition, interior rooms, doors, manually placed windows and full-height glass walls.
- Expansion: standard/tall shops, personnel and garage doors, semi entrance, integrated loading bay, column spacing.
- Site: rectangular/custom pavement, standard/one-way/truck driveways, parking, sidewalks and useful markings. Loading paint is supplied by the loading-bay preset.
- Logistics: forklift purchase, overlay and route tools.
- Decorations: editable freestanding company sign.

This catalogue exposes implemented placement tools. It does not pretend to sell machines that have no purchase/install path; owned stored machines can be restored from Production. Adding a machine installation/purchase system remains a separate backend task.

## Presentation and interaction

The old always-visible construction sidebar was physically removed from DrawHUD. The prototype production sidebar is bypassed in Property View, and the old radial tool-wheel entry point is replaced by the catalogue. The logistics sidebar is no longer drawn; route drawing, selection, movement and testing use a small contextual toolbar. Machine computer screens, ERP, first-person prompts, relevant pavement settings and selected-object inspectors remain available.

The normal planning view has a small bottom toolbar for Build, Select, Move, Resize and Remove, plus active-item instructions, Save, company run/pause, office return and roof visibility. The catalogue has a logo-free charcoal header and uses the original logo palette throughout: charcoal #1F2327, graphite #4B5258, warm off-white #F7F5F1 and foundry orange #F07A2F. All seven categories share this palette. Cards have orange accents and neutral grey preview panels, animated hover, transparent isometric previews, descriptions and applicable costs. Pagination appears only when needed. Item search crosses categories. Rack-level choice is shown beside rack details.

Popup input is consumed before world tools, so clicking the backdrop cannot place an item behind the popup. Hit-testing uses the same canvas and constrained player viewport as rendering; the retired sidebar's invisible screen rectangles are removed from both construction and factory hit tests. All catalogue state belongs to the local HUD; searches, categories and opening/closing do not replicate or mutate the business.

## Placement fixes included

- The exact Sign build button no longer collides with the Sign-prefixed inspector commands. It now activates single-click placement with accurate instructions.
- Choosing a construction tool clears the previous equipment menu/tool, which otherwise could intercept world clicks.
- Selecting equipment pauses that business through its existing authorized server command. The world and other companies keep running; existing occupancy, physical scale and collision/clearance validation remain in force.
- The first forklift purchase can create a default shop-logistics route from actual owned endpoints, rather than requiring a preexisting hand-authored route before purchase.
- Machine-less shops use their actual floor bounds and occupied equipment, not invisible reserved cells from the original cut-parts demonstration layout. An unused workstation may be placed before a forklift or lane exists. Its footprint/access ring is still validated; employees and transport still need reachable destinations when operating.
- Saved construction, purchases, money and ownership continue through the existing server-authoritative systems. Client catalogue requests do not supply trusted company identity or prices.

## Reference

The organization takes inspiration from Satisfactory's separation of categorized buildables, search and quick access, adapted to Foundryline's business/planning view: https://satisfactory.wiki.gg/wiki/Build_Gun and https://satisfactory.wiki.gg/wiki/HUD . This does not copy its art or claim every Satisfactory feature is implemented.

## Verification

Native review script: `Tools/Unreal/review_build_catalogue.py`. It uses queued real input and card coordinates from the rendered catalogue, rather than directly building the requested objects. It checks four-player menu access, actual sign selection and world placement, equipment pause/preview/purchase, rack-level selection and purchase, retired-sidebar hit testing, global search/Enter selection, and a fifth-player join. Screenshots and evidence are under Saved; the review map is never saved after test purchases.

2026-10-30 acceptance:

- Editor Win64 Development build passed (`Saved/Logs/BuildCatalogBuild6.log`).
- Full automated suite: 158 passed, zero failed (`Saved/Automation/Focused-Full/index.json`, 64.95 s).
- Native input/render review: all 16 checks passed, zero errors, 79.55 s (`Saved/Logs/BuildCatalogQA.json`, `BuildCatalogReview2.log`). Sign selection used the rendered card coordinates and actual world mouse input. Workstation and two-level rack purchases reached all four peers; a fifth player received the same placement and purchase state on joining.
- The first native pass caught old demonstration reservations preventing workstation placement. The final implementation removes those reservations from machine-less shops and permits initial placement before transport exists. Both native purchase previews reported GRID ALIGNED / ready.
- Inspected `Build_Catalogue_Expansion00001.png` and `Build_Catalogue_Production00001.png` under `Saved/Screenshots/Reviews`, including the larger popup, actual logo, cards and category/description layout. Search review image is `Build_Catalogue_Search00001.png`.
- Review purchases were unsaved. Normal editor play preferences were restored after review exited. The basic shop is reopened as a standalone game. No frame-rate or maximum-player acceptance is claimed.


## Catalogue cutouts and color revision

`Tools/Blender/render_build_catalogue.py` runs in an independent background Blender process. It reads source assets without saving over them and renders RGBA PNG cutouts to `SourceAssets/UI/BuildCatalogue`. Rack previews match the selected one-, two- or three-level variant. Forklift, racks, sign, welding workstation components and stored equipment use existing game-owned models. Construction shells, doors, paving and route tools use representative isometric geometry. These are menu illustrations, not placement geometry or a change to physical dimensions. The workstation preview includes its bench, welder, four-compartment input bin and painted perimeter.

`Tools/Unreal/import_build_catalogue.py` imports changed cutouts under `/Game/Foundryline/UI/BuildCatalogue`, with UI texture compression, sRGB and no mipmaps. Every cutout has transparent corner pixels; there is no studio floor or background. Pavement samples depict their own surface. Runtime texture references are retained by strong pointers for the HUD lifetime. The popup no longer loads or draws the brand logo. First-person/ERP branding elsewhere is unchanged.

Visual acceptance uses `Tools/Unreal/review_catalogue_colors.py`, opening each category in unsaved PIE and capturing the rendered popup. This presentation revision does not alter purchase validation, prices, placement permissions or network authority.

Color/cutout verification: Win64 Development compilation passed (CatalogueColorBuildFinal.log). All 39 cutouts imported and passed transparent-corner/nonempty-alpha checks (CatalogueAlphaQA.json). Native PIE review opened and captured all six categories with zero script errors (CatalogueColorReview.json); Expansion and Storage final screenshots were inspected for spacing, color and readable previews. Gameplay purchase/network tests above are prior acceptance and were not rerun for this visual-only revision. No review changes were saved to the map.


### Earlier color experiments (superseded by logo palette)

The popup now has a full burnt-orange header with white title text. Every category button has a colored fill; the selected category uses a dark saturated fill with white text. Item previews sit on larger category-colored panels, with white label areas for readability. The detail panel has a category-tinted body, stronger preview background, and a solid dark category heading. This replaces the previous mostly white panels with thin color strips. Item images retain transparency, the popup remains logo-free, and input/placement behavior is unchanged.

Stronger-color validation: Win64 Development build passed (CatalogueStrongColorBuild.log); native unsaved review captured all six categories without errors (CatalogueStrongColorReview.log / CatalogueColorReview.json). Inspected Catalogue_Color_Expansion00002.png for readable text, saturated header/category fills and unobstructed isometric thumbnails.


### Approved logo palette and Decorations

The user clarified that additional popup color must come from the Foundryline logo, not separate category hues. The original artwork was inspected at `SourceAssets/UI/Brand/Foundryline_Logo_Original.png`; the implementation now uses charcoal/graphite, off-white and foundry orange consistently. The title and detail headers are charcoal, preview panels grey, and dividers/selection accents orange. No logo artwork is drawn in the popup. Item cutouts retain their actual model materials.

The company sign belongs to Decorations rather than Expansion. This category appears in navigation and global search still finds the sign. The seven-category visual review checks its presence in Decorations and absence in Expansion. The existing multiplayer input acceptance script now navigates Decorations before selecting the sign.

Logo-palette verification: Win64 Development build passed (CatalogueBrandBuild.log). Native unsaved review captured all seven categories with zero errors, confirmed the company sign in Decorations and absent from Expansion (CatalogueBrandReview.log / CatalogueColorReview.json). Inspected the Expansion and Decorations screenshots for palette, labels and layout.


### Catalogue hierarchy and interaction polish

- Charcoal navigation rail groups all seven categories. The selected category has a filled foundry-orange background, readable charcoal text and a per-category item count. Unselected categories use animated graphite hover and an orange leading edge.
- A section title and available-item count identify the current category or search results above the card grid. The grid now starts below that heading. Card borders are quieter when idle; hovered cards gain an orange outline, dark label panel and a small image lift without moving their click bounds.
- The detail preview remembers the last hovered item while the pointer moves into its panel. A large orange Select Item button invokes the same catalogue item action as its card; it does not introduce a separate purchasing path.
- Search has an orange focus underline and a Clear button when nonempty. Clearing leaves focus in the input and restores the current category. The dark rail replaces the earlier workspace paragraph with a shorter company/property label.
- Palette remains charcoal, graphite, off-white and foundry orange; Decorations contains the company sign. Transparent isometric previews are retained.

The native visual review also exercises typing a single-result search, clicking Clear with real pointer input, and selecting the sign through the detail-panel button. All changes are local presentation; equipment and construction still use existing server validation.

Polish validation: Win64 Development compilation passed (CataloguePolishBuild.log). Native unsaved PIE reviewed all seven categories and passed sign categorization, single-result search, real-click search clearing and real-click detail selection with zero errors (CataloguePolishReview.log / CatalogueColorReview.json). Inspected Expansion and Storage images for spacing and rack controls. Multiplayer purchase acceptance remains the earlier recorded test; this revision was a local HUD/input review.


### Shared surface skin

Catalogue buttons, category rows and item cards now use the same private Canvas surface helper as the Workshop computer: soft edge smoothing, small corner radii, thin borders, slight face gradients and restrained shadows. Hover and click depression remain local UI animation. All reported input rectangles, actions, card prices and server validation paths are unchanged. No additional logo art or category colors are introduced.
