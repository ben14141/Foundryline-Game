# Equipment visual foundation

The press brake, plasma table and sheet roller use a common CNC display palette: dark blue panels, Foundryline orange active states, white values and green readiness indicators. Each machine has its own diagram and readouts. Textures are 1600 × 900 with dedicated local emissive materials; values remain static illustrative artwork.

The plasma cutting envelope is 14 ft long × 10 ft wide. Stock remains the existing 60 × 120 × 1/8 inch sheet. The source model separates the body, bed, gantry, carriage, torch and bridge chain. Runtime substitutes spline hoses for the two static review hoses. Its cutting nozzle has approximately 1.825 mm clearance above the sample sheet; the arc origin is midway in that gap.

The roller retains its existing 352 cm length and runtime component pivots. A larger upper roll and connecting journals correct the thin-sheet nip and bearing continuity. Runtime compensates upper-roll angular speed for the larger diameter. Existing inventory quantities and operation ownership remain in the material-handling cell.

`AFoundrylinePlasmaTable` is a starter-site demonstration actor. The server replicates an operating flag and cycle epoch. Every peer derives the same 29-second path with gantry Y travel, carriage X travel and torch Z retraction. The shared welding-effect component has a separate plasma presentation mode with downward sparks. Particles are local, bounded and culled by camera distance. The authority-only start/stop API is not a client command or finished production interaction.

This prototype does not remove material, consume sheets, produce inventory parts, or assign workers. No production throughput or FPS claim follows from the animation. Mechanical checks establish authored dimensions and visible alignment, not industrial design certification.

Validation entry points:

- `Tools/Blender/validate_equipment_models.py`: saved blend scale, clearances, finite mesh coordinates, display UVs and press stroke.
- `Tools/Unreal/import_*_assets.py`: per-mesh dimensions and material assignment during import.
- `Foundryline.Production.PlasmaCycle`: deterministic path, bounded emission and stopping; existing welding and material-handling tests cover shared behavior.
- `Tools/Unreal/validate_plasma_network.py`: four-peer operating state, motion, particle behavior, client authority rejection and fifth-peer late join. The launcher must configure listen-server PIE before running it.

Current evidence is recorded under Saved/Logs, including EquipmentModelQA.json, EquipmentAutomation.log and PlasmaNetworkQA.json. Blender previews are stored alongside their equipment sources. The review script leaves a local demonstration running without saving placement/map changes.

Equipment placement definitions live under `/Game/Foundryline/Production/Equipment/Placement`. Solid footprints are rounded outward to 25 cm cells and use a full-machine preview mesh. Working areas are 6 × 6.5 m for the press, 7.5 × 8 m for the plasma, and 5 × 6.5 m for the roller. They include operator/material clearance. The existing G planning-grid toggle shows local blue one-metre lines, a machine outline and an amber work-area outline. First-person hides them; a preview may request the area separately. This adds authored placement bounds and visible local guides, not a finished equipment purchase/drag-placement UI or reservations for the starter demonstration actors.

Local acceptance on 2026-10-27: FoundrylineEditor built; all 47 Foundryline automation cases passed, followed by three production cases after the local grid visibility update. Saved Blender geometry/UV/clearance checks passed. Four-peer plasma acceptance plus a fifth late join passed all 16 checks. Maximum sampled inter-peer tip difference was about 0.32 cm. An imported-nozzle bounds check explicitly covers Blender-to-Unreal Y handedness; the welding glow and flexible hoses use the converted coordinates. These are local/editor results, not packaged multiplayer or performance certification.
