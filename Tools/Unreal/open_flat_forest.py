from pathlib import Path
import unreal
root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
if not (root / 'Foundryline.uproject').is_file():
    raise RuntimeError('Foundryline only')
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not levels.load_level('/Game/Foundryline/Maps/L_FlatForest'):
    raise RuntimeError('Forest map unavailable')
editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
editor.set_level_viewport_camera_info(unreal.Vector(-13500, -15500, 13500),
                                      unreal.Rotator(pitch=-37, yaw=49, roll=0))
unreal.SystemLibrary.execute_console_command(editor.get_editor_world(), 'r.EyeAdaptation.CachedLightingPreExposure 10')
unreal.log('FOUNDRYLINE_FOREST_OPENED')
