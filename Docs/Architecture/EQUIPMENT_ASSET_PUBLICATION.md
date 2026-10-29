# Reviewed original equipment asset publication

This equipment-quality update publishes actual original model files in addition to C++ animation, generators, importers and acceptance tools.

## Included

- Original plasma-table, press-brake and sheet-roller component/complete FBX exports under `SourceAssets/Production/Equipment/`.
- Their original CNC display PNGs, manifests and existing model notes.
- The corresponding 33 native Unreal meshes at their manifest package paths, including the four sample steel meshes outside the equipment folders.
- The three native CNC materials and three CNC textures.
- The equipment quality implementation and acceptance document `EQUIPMENT_QUALITY.md`.

Geometry is authored by the project's procedural builders (boxes, cylinders, profile meshes and authored detail). These builders execute the shared helper prefix before `def import_factory`; they do not import Factory, Megapoly or Handyman geometry. CNC images are drawn by the project's display scripts. FBX texture embedding is disabled. Binary assets use Git LFS.

## Excluded and dependencies

Do not publish the source `.blend` files yet: their helper material initialization packs PalletWood/SteelSheet images whose provenance was not verified for this publication. MaterialHandling image files and shared material dependencies remain local. Original purchased content stays excluded.

Native equipment materials reference the project's shared `M_MH_*` library. This reviewed subset does not turn the public clone into a complete playable project or a fully self-contained material package. Restore authorized shared materials/content locally, or rebuild suitable original materials before importing. The FBX files provide the actual model geometry independently of the missing game world.

`Content/` and `SourceAssets/` remain ignored by default. Only this explicitly reviewed file set is tracked; future components need their own provenance review. Blender backups, caches, unrelated rack/bench work, user saves and purchased packs are not part of this update.

## Validation

The final code and imported assets were accepted locally: 13 Production cases, 15 Employee cases, 13 native cycle/UI checks, and 12 four-initial-peer/fifth-late-join mechanical checks. See `EQUIPMENT_QUALITY.md` for exact logs, initial test-fixture corrections, and limits. No process times, recipe prices, material accounting or save schema were changed. No FPS claim is made.
