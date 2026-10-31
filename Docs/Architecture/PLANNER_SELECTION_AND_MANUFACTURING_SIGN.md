# Planner selection and manufacturing signs

The office computer opens property planning. Hover outlines identify the item under the pointer; cyan is hover, orange is selected, and red marks a blocked move. Select the actual door or window at its wall rather than selecting the entire building. Delete removes that opening only. Construction undo restores it.

## Factory items

Machines, purchased and installed storage racks, fabrication stations, product work zones, forklifts, pallets and painted forklift lanes share planner selection. The right inspector offers Move, Inspect and Remove as appropriate. Production must be paused. Existing delivery jobs must finish before changing the floor layout. The host validates every edit against the requesting player's business and plant.

Moving machines carries their ports and attached rack slots with them. Moving an empty rack carries its shelf and dock coordinates. The visible equipment and collision follow the saved layout. Occupied racks, active cycles, reserved stock, shipping bookings and in-progress products block destructive edits. Station collectors must be released before relocation. Forklifts must be parked; release the employee before removing the vehicle. Loading aisles may overlap each other but physical rack bodies cannot obstruct them.

Removing a fixed production machine puts it in equipment storage instead of erasing its identity, program or remaining sheet. It can be placed again from the stored equipment list. This is deliberately not a sale or a refund. Pallet Move requests a real forklift transfer to a matching empty rack slot. Pallet Remove is explicitly labeled Scrap and adds the material to the scrap ledger.

## Manufacturing sign

Build > Buildings > Manufacturing Sign places a freestanding, double-sided sign. R rotates the placement by a quarter turn; the selected sign inspector provides 15-degree fine rotation. Generic Move and Remove work after placement.

- Panel: 12 ft wide by 4 ft high; approximately 9 ft 1 in overall.
- Steel posts, footplates, gussets, mounting fasteners, framed cabinet, inset faces and drip cap.
- Separate frame, panel, accent and hardware meshes preserve distinct materials.
- Editable company name, up to 40 printable characters; no markup or multiline text.
- Independent panel, accent and lettering palettes; Foundryline charcoal/orange defaults.
- Ground pivot and full-size centimetre geometry. A conservative 400 cm square reserves room for any yaw without changing model scale.
- Placement on grass, parking, walkway or loading pavement. It cannot overlap a road, building or another sign.
- Name, colors and rotation are part of the placed-object state, use the construction command path and persist with the property. Appearance edits support construction undo.

The source model lives under `SourceAssets/Buildings/ManufacturingSign`; game-owned native meshes live under `/Game/Foundryline/Buildings/Signs/Manufacturing`. Vendor packages remain untouched. `Tools/Blender/build_manufacturing_sign.py` rebuilds the source using background Blender. `Tools/Unreal/import_manufacturing_sign.py` imports the four meshes.

## Validation

Focused construction automation covers opening selection/move/delete/undo, sign placement/name/rotation/color validation and save round trips, and equipment edit rollback/storage safety. The unsaved architectural multiplayer review checks odd-sized connected sections, different-height transitions, material scale, four clients and late joining. Consult the generated reports in `Saved/Logs` for the result of an actual run; this document describes intended behavior and does not itself certify acceptance.
