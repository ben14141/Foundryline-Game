# Foundryline PB-10 hydraulic press brake

Game-owned model authored at 1:1 scale. Tooling length is 304.8 cm (10 ft), with twelve segmented punches and dies. Blender uses metres; Unreal imports centimetres without manual resizing.

`Foundryline_PressBrake.blend` contains the assembled machine and a 240-frame review animation. The ram travels 31.5 cm; the upper punch and piston rods follow it. The back gauge also moves. Flat and bent 3.175 mm steel coupons swap visibility to illustrate the result; this is not continuous sheet deformation or a production simulation.

Independent FBX meshes include the body, ram, upper punch, piston rods, lower die, back gauge, operator console, foot pedal, and control cables. `PressBrake_Complete.fbx` is an alternative single display mesh: use either that mesh or the separate assembly, never both together. Component offsets are recorded in `manifest.json`.

Equipment imports to `/Game/Foundryline/Production/Equipment/PressBrake`. The two sample steel parts belong to `/Game/Foundryline/Products/Components/Steel`. Shared Foundryline materials are reused. The source builder is `Tools/Blender/build_press_brake_assets.py`; the importer is `Tools/Unreal/import_press_brake_assets.py`.

Details include C-frame throats, tooling clamps, hydraulic hoses and reservoir, pump motor, electrical cabinet, adjustable sheet supports, light-curtain posts, CNC console and keyboard, guarded pedal, anchors, fasteners, and service panels.

This delivery is an asset and Blender animation. It does not add placement UI, employee operation, production recipes, or replicated machine state. Those require a subsequent gameplay implementation.

The CNC monitor uses a dedicated 1600×900 display texture, with a bend section diagram, angle target, axis positions, tooling information, job counter and machine status. `Tools/Blender/build_press_brake_display.py` generates the artwork before the model builder runs. Blender packs it into the blend; Unreal uses the dedicated `Materials/M_PB_CNCDisplay` emissive material. Display values and buttons are illustrative artwork, not live controls.
