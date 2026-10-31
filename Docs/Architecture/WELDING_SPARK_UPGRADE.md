# Welding spark and arc upgrade

The shared gun-tip effect now uses a denser shower of small molten-metal sparks. Particle positions remain in world space as the gun moves. Gravity, air drag and longer lifetimes let sparks fall from bench height to the shop floor. The first impact sheds heat and produces a short bounce; further impacts lose speed and settle. Emission stops immediately with the existing production/paused state.

Each spark supplies its own cooling color and emission strength to the original `M_WeldingSpark` material. Hot yellow-orange streaks shorten and fade toward red. Streak length follows particle motion rather than remaining fixed throughout its flight.

The MIG arc point light is increased from 40,000 to 125,000 candela, with a 9 m attenuation radius and restrained flicker. Nearby welding casts point-light shadows; distant instances omit these shadows. The visible arc core is smaller and brighter. Plasma retains its existing 3,500-candela light and elongated core.

## Scope and budgets

This pass extends the existing instanced particle component rather than adding Niagara. It supports at most 640 particles nearby, 256 at medium distance and 96 farther away, and culls the effect beyond 55 m. A cached downward collision trace finds the actual floor surface beneath the emitter. Particle contact uses that surface plane within the hit component's bounds, which suits the current flat shop slab. This is not general per-particle collision against every bench, prop, stair or curved surface. Sparks are cosmetic and do not damage objects or alter inventory.

Clients generate particles from replicated welding activity. Individual sparks and impact events are not replicated, and dedicated servers do not create their visual components. This change does not alter production timing, recipes or employee assignments. No frame-rate improvement is claimed.

Source: `FoundrylineWeldingEffectComponent.h/.cpp`. Material generation: `Tools/Unreal/prepare_welding_sparks.py`. Rendered multiplayer floor-impact review: `Tools/Unreal/review_welding_ground_sparks.py`. The review creates an unsaved demonstration actor only; it does not save the map or game.

## Validation

The Foundryline Editor build succeeded. The existing welding-effect lifecycle automation passed, covering active emission, the bounded particle budget, immediate stop and the gun socket attachment. A rendered four-player PIE review passed all six checks: four peers, bounded particle counts, particles on every peer, real-floor impacts, replicated welding activity and dense nearby emission. It recorded a peak of 634 visible sparks and 2,785 floor contacts on the sampled emitter. Both review images were inspected. Review settings were restored afterward. No performance benchmark was run, and no changes were pushed or merged.
