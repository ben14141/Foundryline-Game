# Megapoly.Art Trucks & Trailers I

Imported from the user-owned Fab cache Unity package into:
`/Game/Foundryline/Vehicles/MegapolyTrucks`.

Original FBX, texture, material and prefab sources are retained in
`SourceAssets/MegapolyTrucks`. The Fab cache is unchanged.

12 static meshes: three tractor cabs, five semi trailers, one cement mixer,
and three cargo meshes (rocks, sand, soil). Source nodes are combined per FBX.
The gradient texture supplies the palette through an Unreal material. Tangents
are recomputed without MikkTSpace to avoid degenerate UV tangent warnings.

Unreal scene-unit conversion preserves physical dimensions, with no arbitrary
resizing. Cabs measure roughly 6.06-7.48 m long and 4.05 m tall; mirror-inclusive
width is approximately 3.17 m. Tank trailers are about 13.34 m long.

These are imported visual assets, not connected to live vehicle movement or
network simulation. Vendor pack has no rigging or animation. Wheel articulation,
vehicle collision, hitch alignment, turning envelopes and fleet performance need
separate integration. Imported assets currently have one LOD. No map placements
or player saves were changed. Report: Saved/Logs/MegapolyTrucksImport.json.
