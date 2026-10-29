# Detailed Foundryline sheet roller

Retains the existing roller's runtime asset names and pivots, with 352 cm shafts. The upper roll is 34.8 cm in diameter; lower rolls are 24 cm. At the authored centres this leaves approximately 3.32 mm between roll surfaces, fitting 3.175 mm stock. Shaft journals bridge the rolls to their bearing blocks. Runtime upper-roll speed is adjusted for its larger diameter.

Added hydraulic service block and hoses, pressure gauge, guards, emergency stops, frame hardware and a CNC display matching the press brake and plasma table. The screen artwork is illustrative, not live telemetry.

Build `Tools/Blender/build_roller_display.py`, then `build_sheet_roller_assets.py`. The latter extracts only the roller section of the original material-handling builder, preserving the forklift, pallet and racking assets. Import with `Tools/Unreal/import_sheet_roller_assets.py`. Sources and previews live here; runtime meshes remain under `/Game/Foundryline/Production/Equipment/SheetRoller`.
