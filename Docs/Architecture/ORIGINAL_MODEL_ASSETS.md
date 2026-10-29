# Original models available for integration

## Ready-to-load package paths

| Model | Unreal asset path |
| --- | --- |
| Steel pickup headache rack | `/Game/Foundryline/Products/HeadacheRack/SM_HeadacheRack` |
| Independent painted welding bench | `/Game/Foundryline/Production/Equipment/WeldingBench/SM_StandardWeldingBench` |
| Removable welding fixture kit | `/Game/Foundryline/Production/Equipment/WeldingBench/SM_WeldingFixtureKit` |
| Full-sheet roller-output cradle | `/Game/Foundryline/Inventory/Storage/CurvedSheetCradle/SM_CurvedSheetCradle` |
| One-level pallet rack | `/Game/Foundryline/Inventory/Storage/ModularPalletRacking/SM_PalletRack_1Level` |
| Two-level pallet rack | `/Game/Foundryline/Inventory/Storage/ModularPalletRacking/SM_PalletRack_2Levels` |
| Three-level pallet rack | `/Game/Foundryline/Inventory/Storage/ModularPalletRacking/SM_PalletRack_3Levels` |

These are model assets and original materials, not new factory simulation logic. The workstation implementation must load the bench/rack mesh instead of its temporary primitive, use the correct origin/transform, and provide its own collision/occupancy and inventory bindings. Existing actor roots, tool targets and saved records are not changed by this asset-only publication. The full-sheet cradle is NOT the small headache-rack finished-output cradle; do not substitute it by name.

## Dimensions and variants

- Headache rack: generic pickup steel cab guard, 6 ft 6 in wide, 28 in above the bed rail with 4 in stake-pocket inserts. No truck is embedded.
- Bench: 8 ft x 4 ft top, 36 in work height; glossy orange frame and polished bare-steel top. The grounding cable is stowed inside the footprint. Rack and removable clamps/risers are separate assets. Small top hardware extends slightly above work height.
- Curved-sheet cradle: 8 ft x 6 ft, shaped for the provisional full raw 5 ft x 10 ft rolled-sheet model; not the earlier plasma-cut 60-degree shell sector. The sheet is not embedded. Curvature is an art draft, not a changed production recipe.
- Pallet racking: two positions across each usable level. Floor counts as level 1; 1/2/3-level models represent 2/4/6 potential positions. Heights are 5/10/15 ft, with deck elevations floor/5 ft/10 ft as appropriate. The standard pallet footprint is not a 5 ft x 10 ft sheet rack. Allow about 16 ft clear interior height for the three-level model. Model metadata does not silently enlarge current ledger storage capacity.

All models retain true scale: Blender metres, Unreal centimetres, scale 1. Bounds/centred-floor origins should be consumed from their manifests rather than inferred from old proxy geometry.

## Original source and provenance

- `SourceAssets/PublishedOriginalModels/`: game-ready standalone Blender and FBX files for the four product/workstation assets; component grouping and original scalar material values are retained.
- `SourceAssets/Inventory/Storage/ModularPalletRacking/`: three complete rack models, reusable shelf/frame components, Blender sources, dimensions and clearance QA.
- `Tools/Blender/prepare_original_model_publication.py`: prepares the original game-ready source set.
- `Tools/Unreal/import_original_model_publication.py`: imports only this original set into a minimal isolated project, preserving game package paths.
- Corresponding procedural materials live under `/Game/Foundryline/Shared/Materials/OriginalModels`.

These models, materials and review props are original procedural work; they do not include Factory, Handyman or Megapoly geometry or image textures. This differs from the derived white tanker, which stays local and is not part of this public upload. The paused raw-steel tanker edit is not resumed here.

The previously merged plasma, press-brake and roller equipment quality assets already live in `main` under `Content/Foundryline/Production/Equipment/{PlasmaTable,PressBrake,SheetRoller}` and their manifest sample-part paths. Their FBX/CNC/source-tool subset is under `SourceAssets/Production/Equipment/`. See [equipment asset publication](EQUIPMENT_ASSET_PUBLICATION.md). Their packed Blender sources with unverified external texture provenance remain excluded.

## Isolation and validation

Imports use a separate minimal Unreal project under ignored `Saved/OriginalModelsImportSandbox`, not Claude's editor/project. Only this reviewed set of new asset packages is copied into the primary project. No map, game module, employee assignment, inventory schema, live editor or normal player save is changed. Source publication does not require closing another editor.

Geometry QA includes independent model contents, exact dimensions and floor origin, pallet/fork approach, highest-deck reach, and non-overlapping storage positions. The native importer checks actual serialized mesh bounds and material slots; final import evidence is recorded below after completion. Model-only asset checks are not a claim of a new runtime rack-level selector or completed factory put-away behavior.

## Blender previews

These are Blender views, not proof that a live gameplay actor has switched from its placeholder.

![Pickup headache rack](../Images/OriginalModels/HeadacheRack.png)

![Independent welding bench](../Images/OriginalModels/WeldingBench.png)

![Curved-sheet cradle](../Images/OriginalModels/CurvedSheetCradle.png)

![Pallet-rack levels](../Images/OriginalModels/PalletRack_AllLevels.png)

## Final native verification

Seven native meshes and twenty original procedural materials were imported into the isolated project, then loaded again in a fresh Unreal process. Saved bounds, 44 assigned material slots and dependencies passed. No purchased/foreign game-content dependency was found. Exactly 27 new native asset files were copied; zero existing files were overwritten. Existing plasma/brake/roller assets were verified as tracked in main and were not reimported.

The source Blender files were audited: the four standalone sources have no external images or linked libraries, and the three production racking sources are likewise original procedural geometry/materials. The current snapshots are suitable for this public source/model publication.

Datum notes: bench/cradle/rack variants use floor origins. The headache rack uses the bed-rail datum, with stake inserts extending 4 in below it; raise it by that amount if displaying it directly on a flat floor. The removable fixture kit is separate geometry in its authored stored-shelf pose; do not assume it is already arranged as a complete active jig on the table. These transforms need to be respected by Claude's workstation integration.

Native meshes intentionally have no giant auto-generated solid hull that would close rack passages. Placement/gameplay actors still need their appropriate explicit collision, interaction points and inventory bindings. This publication adds models and metadata, not new rack-capacity selection logic or a new playable station simulation.
