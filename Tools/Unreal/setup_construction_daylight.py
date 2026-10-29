"""A neutral, soft daylight setting with Lumen; edits only the Foundryline map."""
import json
from pathlib import Path
import unreal
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
if not (root/'Foundryline.uproject').is_file():
    raise RuntimeError('Foundryline only')
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
target='/Game/Foundryline/Maps/L_ConstructionSite'
if unreal.EditorAssetLibrary.does_asset_exist(target):
    raise RuntimeError('Construction scene already exists; refusing to replace it')
if not levels.load_level('/Game/Foundryline/Maps/L_FlatForest_Grass'):
    raise RuntimeError('Grass scene load failed')
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for actor in actors.get_all_level_actors():
    label=actor.get_actor_label()
    if label=='Sun_Daylight':
        actor.set_actor_rotation(unreal.Rotator(pitch=-52,yaw=-30,roll=0),False)
        light=actor.light_component
        light.set_editor_property('intensity',85000.0)
        light.set_editor_property('light_source_angle',2.5)
        light.set_editor_property('use_temperature',True)
        light.set_editor_property('temperature',5800.0)
    elif label=='Sky_Ambient':
        actor.light_component.set_editor_property('intensity',2.0)
        actor.light_component.set_editor_property('real_time_capture',True)
    elif label=='Distance_Haze':
        actor.component.set_editor_property('fog_density',0.001)
    elif label=='Exposure_Daylight':
        settings=actor.get_editor_property('settings')
        for name,value in (
            ('override_auto_exposure_bias',True),('auto_exposure_bias',0.0),
            ('override_camera_shutter_speed',True),('camera_shutter_speed',125.0),
            ('override_lumen_scene_lighting_quality',True),('lumen_scene_lighting_quality',2.0),
            ('override_lumen_final_gather_quality',True),('lumen_final_gather_quality',2.0),
        ):
            settings.set_editor_property(name,value)
        actor.set_editor_property('settings',settings)
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if not unreal.EditorLoadingAndSavingUtils.save_map(world,target):
    raise RuntimeError('Daylight scene save failed')
(root/'Saved/Setup/construction_daylight.json').write_text(json.dumps({'map':target,'sun_lux':85000,'sun_angle':2.5,'temperature_k':5800,'sky_intensity':2.0,'shutter_speed':125,'lumen_gi':True,'lumen_reflections':True,'performance_measured':False},indent=2))
unreal.log('FOUNDRYLINE_CONSTRUCTION_DAYLIGHT_SAVED')
