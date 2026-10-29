# Original hinged welding hood — model ready, runtime integration candidate

`WeldingHood_Original.blend` contains only original hood geometry and procedural materials. `WeldingHood_Harness.fbx` and `WeldingHood_Visor.fbx` are independent components sharing the ear-hinge origin. `WeldingHood_Complete.fbx` is a review assembly, not a third component to spawn over the others. The dark filter is non-emissive; no HUD screen is embedded in the lens.

Measured closed size is approximately 13.90 in wide × 14.21 in deep × 12.23 in high, fitted to the actual stylized Handyman heads without changing their scale. The editable action `WeldingHood_Lift_Closed_Open` shows closed at frame1, raised at frame30 and closed at frame60.

The model is authored in a neutral ear-hinge frame. Native component front is +Y, up +Z. Attach its root to the **head bone** using the precise local translation and quaternion in `manifest.json`; no employee identity, world position or animation authority changes are required. Rotate the visor about its local +X axis from0° closed to+105° raised. Blender's corresponding source angle is-105° due to coordinate handedness. These native attachment constants are prepared from the measured existing head reference; they still require in-engine validation.

All three current heads and their actual selected beards were measured and tested at22 poses each, from0° through105° in5° steps:66 pose checks, zero head/hood triangle intersections, minimum sampled vertex clearance5.443mm. Existing forward/backward cap brims cannot fit inside the hood; the runtime candidate filters only `SK_Hat_*` from three shared welder outfit variants, preserving each worker's head, beard, clothing, identity and stable appearance index.

`head_fit_qa.json` records the actual reference fit. `source_qa.json` records seven passing original-source and full-scale FBX roundtrip checks. The original `.blend` was saved before any Handyman reference mesh or texture was loaded; it contains no licensed head/body geometry, external images, linked libraries or armatures. The worn fitting PNG uses licensed local heads as visual reference and is separate from the original model source.

`IntegrationCandidate` contains the proposed attachment code and hook patch. It has not been adopted into live Source, compiled, imported or tested in Unreal while Claude is working. Live hood animation and cap filtering remain pending integration acceptance. No other project, active content, map, save or vendor asset was changed.

## V2 curved-shell refinement

The canonical source/FBXs now use the approved V2 compound-curved shell, tapered cheeks/chin, rounded protective rim, recessed curved filter/gasket/outer cover, narrow solar strip and two arc sensors, gloved shade control, detented knurled hinges and crown suspension strap. The exact original head-relative offset/quaternion and0..105-degree hinge contract remain unchanged. Source7/7 and all66 head-pose checks pass; minimum clearance5.443mm. High-detail original geometry totals33,288 evaluated triangles across6 materials; no engine performance measurement is claimed. V1 source is preserved separately. The V2-labeled PNGs are current actual Blender renders; no live Source/Content changes or native import were made.
