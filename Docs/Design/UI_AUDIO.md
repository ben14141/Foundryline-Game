# Softline UI Sounds

The user's downloaded Fab pack is Softline UI Sounds FREE - 50 Game UI Sound Effects
by TheCookedWorks. The local download contains a Unity package, not Unreal assets.

Original WAV files have been extracted unchanged to SourceAssets/Audio/SoftlineUI,
with the vendor's documentation PDFs and per-theme CSV catalogs. manifest.json
records the package hash, per-file hashes and audio format/duration information.
Unity demo code, scenes, fonts and Unity-specific sound-set assets were not copied.

There are five themes: Clean Paper, Cozy Chunky, Flat Night, Pixel and Soft Glass,
with ten sounds per theme. All 50 WAV headers were parsed successfully. This verifies
file readability, not subjective sound quality or engine playback.

## Unreal import (pending)

Update: the script has now run successfully inside Foundryline. All 50 SoundWave
assets exist under /Game/Foundryline/UI/Audio/Softline; the import logged zero errors
or warnings. Audition and UI bindings remain pending. The instructions below are
retained for reproducibility; reruns skip existing assets.

When other Unreal work is finished, open Foundryline, enable the Python Editor
Script Plugin if needed, and run Tools/Unreal/import_softline_ui.py inside that
project. It imports under /Game/Foundryline/UI/Audio/Softline, preserving theme and
category folders. It skips existing assets and never launches an editor itself.
The script has been syntax-checked but not executed in Unreal. No SoundWave assets,
Sound Cues or UI bindings have been created yet.

Audition the themes before selecting one. Then bind restrained hover/click,
tab-switch, popup, confirm/cancel, error and notification cues to the UI. Throttle
hover feedback and avoid routine production notification spam. UI audio should be
local to each player; it must not be replicated as town/world sound.
