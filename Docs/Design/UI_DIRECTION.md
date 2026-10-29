# First-scene UI specification

The current Canvas construction HUD uses the original logo lockup and the palette
below, with chamfered blue/orange tool buttons. It is not yet a full production HUD.

Reference: Master Plan v1.1 page 2 brand board and UI chapter, approved in chat.

- Charcoal #1F2327; graphite #4B5258; steel #9AA0A6; fog #F7F5F1.
- Industrial blue #2F6B8F for selection; foundry orange #F07A2F sparingly for emphasis.
- Clean readable typography, thin borders, modest corners and restrained panel sizes.
- Small FOUNDRYLINE wordmark/monogram; do not substitute an unrelated generated logo.
- Top HUD: cash, date/time, employees, meaningful alerts and simulation controls.
- Bottom toolbar: Structure, Surfaces, Production, Office, Logistics, Paths, Decor.
- One right-side context inspector, with selection details and explicit blocked reasons.
- Placement controls show snap resolution, rotation, footprint/clearance and cost.
- Holograms show validity with an icon/text reason as well as color. Visual material
  is separate from real collision and never modifies authoritative state on hover.
- Ghost planning is free to revise; commitment and spending form a later transaction.
- Grid/flow overlays are optional and labels simplify with camera distance.
- Department identity uses colors plus icons; status warnings remain a separate layer.
- Roofs remain invisible at every gameplay zoom.


## 2026-10-26 logo import and animated sidebar

Use the dedicated original logo texture T_FoundrylineLogo, imported from SourceAssets/UI/Brand/Foundryline_Logo_Original.png. It is extracted from the supplied Master Plan's embedded brand board (large original monogram/wordmark/tagline), with original pixels preserved; no generated replacement. It uses UI compression, UI texture group and no mipmaps. Tools/Unreal/import_original_logo.py reproduces import. The old full-board texture is retained as reference and is no longer cropped at runtime.

The left sidebar remains authoritative for construction controls. Larger 76-unit cards separate tool names and details, with Build/Edit/Office navigation. Build has Shells, Roads and Doors filters so the complete kit fits without tiny crowded rows. Hover uses a smooth colour fade, a 4-unit horizontal lift, orange edge emphasis and a chevron; click feedback settles back without moving hit targets. Status, dimensions, current selection, Cancel and Undo remain visible. Animations change presentation only. Earlier top/bottom toolbar descriptions are historical; the approved sidebar supersedes them.
