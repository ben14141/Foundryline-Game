# Original forklift upgrade kit — public-safe additions only

This package contains **only our original procedural addon geometry**, not a complete forklift. The eight purchased Factory-derived base meshes were removed before export; only their empty attachment datums remain. The manifest lists removed base-mesh vertex counts and the original addon objects. All retained materials are original `FQ_` or `Beacon_` procedural materials; no image textures, linked libraries, Factory geometry or original `MH_` material datablocks remain.

The complete kit `.blend`/`.fbx` and eight component-addon FBXs contain service panels and vents, access steps/handles, guard harnesses and lamps, wheel rim/hub details, mast guides/hydraulic/chain details, the original hollow amber beacon and surface-applied printed marks. Datums preserve existing component positions; the kit is not an additional complete vehicle to spawn over an unchanged mesh. Existing body geometry already has a crude beacon and floating glyphs, so use the local patch pipeline to remove those before attaching/adopting additions.

## Generate the complete vehicle locally

Run from a checkout with your locally licensed/prepared sources available:

1. `SourceAssets/MaterialHandling/manifest.json` and its eight `Vehicles/Forklift/FL01/Forklift_*.fbx` source files must already exist locally. These licensed-derived files are not supplied by this public package.
2. Run `Tools/Blender/build_forklift_amber_beacon.py` in background Blender. The original independent beacon is saved under `SourceAssets/Vehicles/Forklift/AmberBeacon`. Its roof review also reads the locally licensed body; do not publish that roof-review image.
3. Run `Tools/Blender/refine_forklift_quality_20261029.py` to generate the complete local quality copy with separate moving components.
4. Run `Tools/Blender/fix_forklift_review_labels.py` to remove inherited floating brand glyphs and replace them with flush service-panel print.
5. Run `Tools/Blender/verify_forklift_quality_20261029.py` for source pivot/ground-offset/lift checks and the beacon socket/reference-light update.

Generated complete vehicle files are under `SourceAssets/Vehicles/Forklift/QualityReview_2026-10-29` and **must remain excluded from public Git**. All scripts resolve the checkout from their own path. No Unreal import, editor action, build, gameplay change or automatic adoption is performed by these scripts.

The independent original beacon's `SOCKET_Bulb` is 10.1 cm above its mounting datum. Roof mount is forklift-local `(-115,0,253)` cm. These are preparation anchors; an actual Unreal point light and runtime vehicle replacement are not wired by this source-only publication.
