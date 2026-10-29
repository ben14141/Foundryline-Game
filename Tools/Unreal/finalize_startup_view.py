"""Run in Foundryline's open editor to finalize its viewport and lighting range."""
from pathlib import Path
import unreal

project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
if not (project / 'Foundryline.uproject').is_file():
    raise RuntimeError('Foundryline project required.')
editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = editor.get_editor_world()
if world.get_path_name().split('.')[0] != '/Game/Foundryline/Maps/L_StartupSite':
    raise RuntimeError('Open the Foundryline startup map first.')
unreal.SystemLibrary.execute_console_command(world, 'r.EyeAdaptation.CachedLightingPreExposure 10')
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for actor in actors.get_all_level_actors():
    if actor.get_actor_label() == 'Sun_Daylight':
        actor.set_actor_rotation(unreal.Rotator(pitch=-48, yaw=-35, roll=0), False)
editor.set_level_viewport_camera_info(unreal.Vector(-15000, -15000, 18000),
                                      unreal.Rotator(pitch=-40.3, yaw=45, roll=0))
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
unreal.log('FOUNDRYLINE_VIEW_FINALIZED')
