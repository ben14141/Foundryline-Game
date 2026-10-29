# Flat forest scene

The grass scene is `/Game/Foundryline/Maps/L_FlatForest_Grass`; the current startup
map is its daylight construction variant `/Game/Foundryline/Maps/L_ConstructionSite`.

Dense grass update: 738,187 instanced 3D grass clumps cover the meadow, including
the central buildable area. Five authored grass meshes use varied rotations and
scales at approximately 65 cm spacing. Grass has no collision or shadow casting,
and ends rendering at 35 m from the camera. The underlying grass ground texture
remains at distant zooms. The earlier sparse grass instances are removed from this
map. Saved map reload and instance/cull checks passed; close/far visual acceptance
and FPS measurement are still pending. No smooth material fade is claimed.

StyleHex forest content is preserved at its original vendor paths. Game-owned
foliage definitions and the textured ground material live in `Content/Foundryline/World`.
The 600 m ground remains flat at Z=0, with no height displacement. A clear central
building site and southern access corridor are reserved. Instanced trees, bushes,
grass, flowers and stones surround the site; small foliage has distance culling.

The 24 old Factory trees and 36 bushes have been removed from the clean map.
A separate Unreal process reloaded the saved clean map and verified zero old
Factory foliage actors. The earlier maps are retained. The clean map was saved
separately because the open editor locked L_FlatForest.umap.

Lumen daylight remains configured. The forest was inspected visually before this
cleanup; the final saved cleanup was verified by reload, not another visual pass.
60 FPS has not been measured. Vegetation collision, harvest/removal gameplay and
pathfinding integration remain future work.
