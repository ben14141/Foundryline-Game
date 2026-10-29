# MIG equipment quality rebuild — model source review

Original procedural rebuild with eight independent groups: cart frame, wheels, power unit, gas cylinder, regulator, torch, leads and work clamp. All geometry/materials in this folder are authored from scratch, with no vendor meshes or bitmap textures. The existing production assets and original Blender files remain unchanged.

Substantial changes include:

- Folded-steel trolley, axles, shelf, retaining bands and linked bottle chain.
- Panelled orange/graphite MIG source with an actual open side inspection window, copper wire spool and feed rollers; readable amp/volt display, encoders and labelled lead sockets.
- Drawn blue gas cylinder with domed shoulder, open base ring, neck valve/handwheel, brass regulator, two graduated gauges with needles and glass, and routed shielding-gas hose.
- Ergonomic torch with rubber grip, trigger/guard, bent steel swan neck, real hollow copper nozzle, recessed contact tip and 0.047 in welding wire.
- Copper ground-clamp jaws, insulated handles and pivot; separate rubber work/MIG leads and connector strain relief.

Scale remains 1 Blender metre = 100 Unreal cm. The cart tray is approximately 3 ft 5.3 in × 2 ft 0.4 in. The assembly is approximately 5 ft 3.5 in high. Exact evaluated bounds and group pivots are recorded in `manifest.json`.

The runtime torch contract is preserved: grip `(0,0,0)`, support `(3,-3,8)`, welding endpoint `(23.9,0,13.5)` in centimetres. The bore is genuinely open, approximately 0.646 in across, with a 0.866 in outside nozzle diameter. `SOCKET_Grip`, `SOCKET_Support`, `SOCKET_WeldTip` and `SOCKET_TriggerPivot` remain editable anchors. Compatible local exports are `Welder_MIGGun.fbx`, `Welder_WorkClamp.fbx`, `Welder_Machine.fbx` and `Welder_Leads.fbx`; generic group FBXs retain the common assembled datum.

`Foundryline_MIGWelder_QualityReview.blend` is the editable assembly. `Welder_Complete_Review.fbx` is a complete preview, not an additional mesh to spawn alongside the components. Gauges/spool/wheels/trigger/valve elements remain separate for future animation; no new production, gas or animation gameplay is implemented.

`geometry_qa.json` validates the source anchors and real FBX roundtrip, nozzle bore, bottle/tray contact, wheel clearance, independent groups, no external dependencies and unchanged original production hashes. The three images are actual CPU Blender renders. This review does not import, build, modify active Content, or interrupt Claude's Unreal session. Native materials, runtime collision and in-engine fit acceptance remain pending.

## Surface-label correction

All14 labels are zero-extrusion mesh print. Bottle and torch marks wrap onto their curved skins; panel legends sit on painted faces and gauge units/ticks sit on the dial. label_surface_qa.json measures vertices and face centres against actual underlying meshes: gaps0.143..0.297mm; minimum normal alignment0.9967. No FONT objects or billboard text remain. Compatible FBX exports and editable source updated; all13 earlier geometry checks and original source hashes still pass. The LabelsFixed images show the corrected source.
