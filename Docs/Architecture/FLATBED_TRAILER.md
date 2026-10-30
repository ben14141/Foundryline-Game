# 53 ft flatbed trailer

An aluminium flatbed semi-trailer (Utility 4000AE style: 53 ft x 102 in, spread tandem, oak edge boards, black
mudflaps, red marker lamps) that hitches to the existing Megapoly tractor. Ben approved the model on 2026-10-30.
It is imported as static meshes only; no carrier, booking or freight-rig code uses it yet.

## Files

| What | Where | In Git |
| --- | --- | --- |
| Build script (geometry, drivers, demo animation, FBX export) | `Tools/Blender/build_flatbed_trailer.py` | yes |
| Unreal import (meshes, lamp material, LODs) | `Tools/Unreal/import_flatbed_trailer.py` | yes |
| Unreal render check with the tractor | `Tools/Unreal/review_flatbed_trailer.py` | yes |
| Blender source, FBX parts, `manifest.json` | `SourceAssets/Vehicles/FlatbedTrailer/` | no |
| Static meshes and `M_TrailerMarkerLight` | `/Game/Foundryline/Vehicles/FlatbedTrailer/` | no |

The geometry is original, but every face is coloured by UVs into the purchased Megapoly 256-gradient palette
(`T_TruckPalette`, the starter hauler's `M_TruckPalette`), and the `.blend` contains the Megapoly tractor for the
hitch. So, like the tractor, the meshes and sources stay local (see `Docs/REPOSITORY_CONTENT.md`). Publishing would
need vendor-free constant materials and a `.blend` without the tractor.

## Rebuild

1. In Blender 5.1, run `build_flatbed_trailer.py` with `EXPORT_DIR` set to
   `SourceAssets/Vehicles/FlatbedTrailer` (the script reads the palette PNG and tractor FBXs from `SourceAssets`).
2. `UnrealEditor Foundryline.uproject -RenderOffscreen -ExecutePythonScript=<abs>/Tools/Unreal/import_flatbed_trailer.py`
   The full editor is needed for LOD reduction; the commandlet imports without LODs.
3. Optional: the same with `review_flatbed_trailer.py` writes `Saved/Screenshots/Reviews/FlatbedTrailer_*.png`.

## Assembly (Unreal cm, from `manifest.json`)

- Trailer origin: ground under the kingpin, +X toward the tractor. Kingpin plate at Z 128, matching the tractor's
  fifth wheel at (42, 0, 128) from the tractor origin (`AFoundrylineFreightRig::Hitch`).
- Deck top Z 147, length 1615.4, width 259. The starter carrier code assumes a 113 cm deck, so pallet anchors
  will need the trailer's height.
- Wheels: `SM_Trailer_Wheel_0..3` on axle centres at X -1095 and -1400, Y +/-98, Z 47.35 (same rolling radius as
  the tractor). Spin about local Y from distance travelled.
- Kingpin to tandem centre: 1247.5 cm. `AFoundrylineFreightRig` hard-codes 821.8 cm for the tanker; articulation
  for this trailer needs 1247.5.
- Legs: `SM_Trailer_Leg_0/1` at X -360, Y -/+58, Z 30 raised; lower by 30 cm to stand.
- Material slots: 0 `M_TruckPalette`, 1 `M_TrailerMarkerLight` (emissive palette, lamp lenses).
- Triangles (LOD0/1/2): body 75k/26k/7.5k, each dual-wheel set 10k/3.6k/1k, legs 368.

The Blender file also has `FlatbedTrailer_ROOT` controls (`travel_metres`, `landing_gear`), tractor controls
(`travel_metres`, `steering_degrees`) and a 901-frame demo action: drive, turn with articulation, stop, lower
the legs, uncouple.

FBX import reports degenerate tangents. That is expected for palette UVs (every face maps to a point) and
harmless without normal maps; the Megapoly trucks do the same.
