import unreal
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
if not (root/'Foundryline.uproject').is_file():
    raise RuntimeError('Foundryline only')
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
levels.load_level('/Game/Foundryline/Maps/L_ConstructionSite')
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if actor.get_actor_label()=='Sky_Ambient':
        actor.light_component.set_editor_property('intensity',2.0)
    elif actor.get_actor_label()=='Exposure_Daylight':
        s=actor.get_editor_property('settings')
        s.set_editor_property('auto_exposure_bias',0.0)
        s.set_editor_property('override_camera_shutter_speed',True)
        s.set_editor_property('camera_shutter_speed',125.0)
        actor.set_editor_property('settings',s)
if not levels.save_current_level():
    raise RuntimeError('Daylight polish save failed')
unreal.log('FOUNDRYLINE_DAYLIGHT_POLISHED')
