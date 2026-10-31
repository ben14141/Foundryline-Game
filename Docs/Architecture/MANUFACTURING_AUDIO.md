# Manufacturing equipment audio

## Pack location and licensing

The user added **Manufacturing Ambiences – Factory Sounds Collection**, by rawAmbience, through Fab. The Launcher initially installed it into `C:/Users/curly/Documents/Foundryline-Collectors/Content/Manufacturing_Ambiences`. Its 102 files were copied to the correct primary project at `C:/Users/curly/Documents/Foundryline Game/Content/Manufacturing_Ambiences`, preserving `/Game/Manufacturing_Ambiences` package paths. The pack contains 51 SoundWave recordings and 51 SoundCue wrappers. Vendor assets are preserved unchanged.

The pack is licensed content. It and the processed derivatives remain excluded from the public repository. Code, import tools, role mappings and this document may be published independently. A source-only clone still requires the authorized local assets.

## Equipment mapping

The pack names its recordings by number, rather than identifying specific machines. Selected recordings are filtered and calibrated into short mono loops; these are sound-design textures, not a claim that the supplier recorded each particular Foundryline machine.

| Sound layer | Activation |
| --- | --- |
| Plasma gantry | Operator present and machine processing |
| Plasma cutting hiss | Actual moving torch effect reports cutting |
| Press brake hydraulics | Active forming/return portion of the ram cycle |
| Press contact | One short accent as the stroke reaches forming depth |
| Sheet roller | Assigned operator actually forming a sheet |
| Welding arc | The actual gun-tip welding effect is active |
| Forklift drive | Assigned driver moving the vehicle |
| Forklift hydraulics | Assigned driver handling a pickup or unload |
| Collector cart | Assigned collector pushing the cart |
| Room ambience | Very low level while multiple machines are active |

There is no dedicated sound explicitly labeled welding in this pack. The arc layer is a processed broadband machine texture with restrained pulse/crackle shaping and remains provisional. The drive/hydraulic/cart layers are similarly processed industrial textures. Dedicated recordings can replace the corresponding game-owned SoundWaves without changing the simulation wiring. No unsupported diesel, horn, footstep or door recording is claimed or assigned.

## Mix and spatial behavior

Each listener's world subsystem reads the existing factory snapshot and local machine effects. It never changes production state or sends per-sound network events. Keys include the owning actor and equipment identity, so separate companies retain separate spatial sources. Dedicated servers create no audio components.

The listener uses the player's current camera in first person or Planning View. Sources are mono and spatialized with a 1.5 m inner region, natural distance falloff and a 16.5 m falloff range. Distant sound is low-pass filtered; walls apply occlusion, reduced volume and a softer frequency response. Sources beyond 20 m are not created. Machinery is separate from existing Softline UI sounds.

A maximum of eight voices is shared across machinery in each listener's world. Nearest sources take priority. The mix scales by the inverse square root of active-source count, with smooth starts/stops and a separate machinery SoundClass at 0.8 volume. The room bed is deliberately faint and stops when production is inactive. Idle equipment does not run continuous fake factory noise. Parked forklifts are silent. A brief movement hold prevents snapshot updates from making forklift audio stutter.

Processed loops target approximately -20 dBFS RMS after gentle peak compression, with individual peaks capped at -9 dBFS before playback gains. EQ reduces low rumble and harsh upper frequencies. Loop seams are crossfaded. The complete lineage, gains, frequency ranges and measured file levels are recorded in the private derived-audio manifest.

## Files

- Runtime: `Public/Audio/FoundrylineManufacturingAudio.h` and `Private/Audio/FoundrylineManufacturingAudio.cpp`.
- Analysis export: `Tools/Unreal/export_manufacturing_audio.py` (read-only vendor export).
- Processing: `Tools/Audio/prepare_manufacturing_loops.py`.
- Game-owned assets: `/Game/Foundryline/Audio/Manufacturing`.
- Private WAV sources: `SourceAssets/Audio/ManufacturingDerived`.
- Import: `Tools/Unreal/import_manufacturing_audio.py`.
- Runtime and mix review: `Tools/Unreal/review_manufacturing_audio.py`.

The game-owned audio directory is explicitly included in cooking because runtime code loads the role assets by path. No shader, renderer or global engine configuration change is required for this system.

## Acceptance on 29 October 2026

Both the isolated and primary Foundryline Editor builds passed. The final four-player runtime review passed all seven checks, covering profile loading, valid machine exercise, bounded voices, inactive-state silence, distance culling and equipment/welding playback. Nine roles were observed, including both forklift layers and the press contact; collector-cart playback is wired but was not exercised by that full-workshop fixture.

Single-listener and four-listener captures contained no clipped samples. Paused and distant recordings were digitally silent. The single-listener nearby review peaked at -38.38 dBFS in its native eight-channel capture. Source-file level measurements, runtime reports and captures are retained under Saved/Logs and Saved/AudioReview. These measurements establish signal presence, headroom and gating; they do not replace subjective listening or establish a frame-rate target. Hidden-editor capture used temporary test-only audio-device overrides, which do not alter the primary game settings.

All 102 vendor files in the primary project were hash-checked against the downloaded pack and matched. No purchased audio was pushed to GitHub.
