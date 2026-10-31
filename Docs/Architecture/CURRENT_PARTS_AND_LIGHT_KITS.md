# Current product range and identifiable light kits

The active game range is now limited to the headache-rack chain. Stable part IDs and historical meshes remain on disk for save compatibility and later development; they are not renumbered or deleted.

| Item | Source / use |
| --- | --- |
| Steel sheets | Purchased raw material; plasma consumes pounds from a sheet |
| Bracket blanks | Plasma: 5 lb and 8 plant minutes per blank |
| Steel mounting plates | Plasma: 2 lb and 4 plant minutes per plate |
| Formed brackets | Press brake: one bracket blank and 8 plant minutes; no additional steel |
| Frame sections | Purchased ready-cut members |
| Light kits | Purchased lamp, harness and hardware |
| Headache racks | Welding assembly: 2 mounting plates, 4 formed brackets, 6 frame sections, 2 light kits |

The user explicitly chose formed brackets for the headache rack. Raw bracket blanks do not satisfy its assembly recipe. The starter still contains only the plasma as production equipment; the press brake and welding station are later purchases. Manufacturing a complete rack therefore requires the forming upgrade before assembly, or previously produced formed stock. Existing monetary values, item weights and process times are preserved.

Fresh starter plasma menus expose only bracket blanks and mounting plates. The brake upgrade adds only formed brackets. Retired items are excluded from new contracts and saleable stock; server programming rejects them in commerce games. New contract generation, machine programs, assembly requirements, supply bins and collector demand derive from the same catalogue. Other forming outputs retain explicit Formed names in their historical definitions. Old development scenarios are retained separately from the active starter range.

The light kit uses its own original `SM_LightKitBox` under `/Game/Foundryline/Products/Components/LightKit`, replacing the generic fittings crate. Its approximately 16.5 by 11.2 inch carton has orange identification bands, graphite panels, large LIGHT KIT labels on four sides, a lamp pictogram, LK-01 end labels and a sealed flat lid. Labels are mesh geometry and materials are standalone constants, so there are no font or external texture dependencies in Unreal. The flat-lid carton is approximately 27.5 cm tall and stays below the existing 35 cm visual stacking interval. Pallets, racking, trucks, workstation supplies and the UI thumbnail use this identity.

Rebuild with `Tools/Blender/build_light_kit_box.py` in an isolated Blender process, then `Tools/Unreal/import_light_kit_box.py`. The importer checks physical size, material assignments and the replacement UI thumbnail. The general contract-thumbnail generator also uses the new model. Blender sources and exports are in `SourceAssets/Products/Components/LightKit`.

## Part and pallet weights

Every active item now has a default unit weight in the central part specification, including sheets (approximately 255 lb from the real 60 by 120 by 1/8 inch stock dimensions). Blanks and formed brackets weigh 5 lb each, mounting plates 2 lb, frame sections 16 lb, light kits 4.5 lb and a finished headache rack 129 lb. Pallet creation applies the default when no explicit material weight is supplied; processing preserves the incoming material mass.

A standard wooden pallet carries at most 1,500 lb of cargo. Existing oversized loads supported by two pallets—sheet stock and complete headache racks—have a combined 3,000 lb cargo limit. Physical item-count/stack limits remain in force as well. The limit is checked when creating stock, reserving forklift moves, combining destination loads, booking freight and validating saved pallets/manifests. Cargo limits exclude the wooden pallet weight; freight totals continue to include 60 lb per supporting pallet. The physical stock inspector displays unit weight, current/maximum cargo weight and gross freight weight.

Bracket blanks, formed brackets, plates and light-kit cartons now render the actual quantity in a bounds-based grid of instanced meshes. Sheet bundles retain the earlier three-sheet visual representation while their data and weight track every sheet. No per-part physics actors are introduced. The unsaved four-player `review_current_parts.py` review checks replicated stock and matching rendered instance counts for full pallets of 50 blanks, 50 formed brackets, 100 plates and 8 marked light-kit cartons.

The carton is authored to fit the existing supply-bin shelf cells at full scale. Supply bins show up to two representative labelled cartons per light-kit compartment; collector carts also carry the identifiable carton mesh. Their exact quantities and pounds remain in the simulation and inspection UI.

## Validation on 2026-10-30

The Win64 Development editor build succeeded, and all 162 automation checks passed (`FocusedBuild.log`, `Saved/Automation/Focused-Full/index.json`). Coverage includes the reduced catalogue, disabled-recipe rejection, four-formed-bracket assembly bill, default sheet/part weights, overload rejection, manufacturing/material conservation, shipment handling, and the complete two-order plasma → press brake → headache-rack assembly → shipping flow. The latter uses an expanded upgrade shop with separately connected forklift and employee approaches; it does not add a brake or bench to the first shop.

The normal editor import completed cleanly with six material slots and measured box extents of approximately 20.893 × 14.238 × 16.675 cm (`LightKitImport2.log`, `LightKitImportQA.json`). All 16 native checks passed across four players (`CurrentPartsQA.json`, `CurrentPartsReview.log`), including restricted starter offers, positive replicated unit weights, identical pallet quantities and the actual rendered counts of 50 blanks, 50 formed brackets, 100 plates and 8 light kits. Review stock was unsaved. The gameplay screenshots are `CurrentParts_FullPallets00000.png` and `LightKits_InGame00000.png` under Saved/Screenshots/Reviews. Normal standalone preferences were restored and the clean first-person starter reopened afterward.

User revision: removed the sample lamp, bezel, optics and mounting tabs from the carton lid. The Blender source, FBX, Unreal mesh and UI thumbnail now use the sealed flat-lid version; all side labels, pictograms, names, inventory weights and pallet rules are retained. The previous six-slot/33.35 cm-high import evidence above refers to the earlier sample-lamp version. Current import bounds and materials are recorded in LightKitImportQA.json and LightKitFlatLidImport.log.
