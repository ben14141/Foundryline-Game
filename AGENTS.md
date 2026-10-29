# Foundryline working rules

- Read Docs/ARCHITECTURE.md and Docs/Planning/STARTUP_PLAN.md before changes.
- Follow the Master Plan v1.1 system boundaries. Public headers go in
  Source/Foundryline/Public/<System>; implementations in Private/<System>.
- Multiplayer is mandatory for all gameplay work, including more than two players.
  Keep server authority, company/plant ownership and validated actions separate
  from local presentation and per-player camera/roof preferences.
- Expect many moving objects: keep construction records separate from continuous
  vehicle, employee and machinery simulation; use relevance and bounded updates.
  Four-player local acceptance does not establish a production player cap or FPS.
- Units are real centimetres (1 UU = 1 cm). Placement snapping never changes scale.
- Roofs default hidden in top-down view, with a show/hide button; always show in first person. Target at least 60 FPS with Lumen, but report
  measurements separately from estimates and configuration.
- Preserve Content/Factory vendor assets and package paths. Author game-owned
  content under Content/Foundryline. Do not publish purchased assets to a public
  remote without checking redistribution rights.
- No editor launch, compilation, shader cook or asset import while Claude's other
  Unreal work is in progress, unless the user explicitly lifts that constraint.
- Do not change other projects, global engine settings or shared build processes.
- Report source review, compilation, automated runtime tests and visual/editor
  acceptance separately. Uncompiled source is not a validated working feature.
- Never merge without explicit user permission. Do not push unless requested.
