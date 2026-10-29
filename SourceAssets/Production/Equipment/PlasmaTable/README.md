# Foundryline PT-1410 plasma table

True-scale cutting envelope is 14 ft long × 10 ft wide (426.72 × 304.8 cm). The bed opening is 438.92 × 317.4 cm, leaving loading clearance. Standard stock remains one 60 × 120 inch (152.4 × 304.8 cm), 3.175 mm sheet. Blender uses metres; Unreal imports centimetres without manual scaling.

Components: Body, SlatBed, Gantry, TorchCarriage, Torch, CableChain, Umbilical, ServiceLead, OperatorConsole, PowerUnit, Extraction, and StockSheet. A Complete mesh combines the equipment, excluding stock. Use either Complete or the independent assembly, not both. Runtime replaces the two static hose meshes with flexible spline hoses so the torch can move without disconnecting them.

The blend includes review motion for longitudinal gantry travel, transverse carriage travel and torch retraction. Gantry owns carriage and chain; carriage owns torch. The energy chain remains a rigid bridge-mounted guide; the connected flexible hoses update with motion in Unreal.

The screen has a dedicated 1600 × 900 texture with nested brackets, job count, material, thickness, current, feed and torch-height readouts. Values and buttons are static illustrative artwork.

Source tools: `Tools/Blender/build_plasma_display.py`, then `build_plasma_table_assets.py`. Unreal importer: `Tools/Unreal/import_plasma_table_assets.py`. Equipment belongs to `/Game/Foundryline/Production/Equipment/PlasmaTable`; stock belongs to `/Game/Foundryline/Products/Components/Steel`.

`AFoundrylinePlasmaTable` adds a running demonstration to the starter site. A 29-second cycle repositions at raised height, lowers the torch, follows a rectangular cutting path, and retracts. Server-owned operating state and epoch replicate; clients derive the same motion and render bounded local plasma sparks. The effect turns off during travel and stopping. Four-peer and late-join checks are in `Tools/Unreal/validate_plasma_network.py`; actual validation results belong in Saved/Logs.

This does not implement stock consumption, material removal, recipes, employee operation or machine placement UI. Screen values are illustrative artwork, not live telemetry. No cutting performance or real-machine specification is certified by the model.
