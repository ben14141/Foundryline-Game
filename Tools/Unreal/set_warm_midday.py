"""Warm midday daylight, only the saved Foundryline construction map."""
import unreal,json
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
if root.name!='Foundryline Game' or not (root/'Foundryline.uproject').is_file(): raise RuntimeError('Foundryline project required')
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not levels.load_level('/Game/Foundryline/Maps/L_ConstructionSite'): raise RuntimeError('Map not loaded')
updated=[]
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
 if actor.get_actor_label()=='Sun_Daylight':
  actor.set_actor_rotation(unreal.Rotator(pitch=-75,yaw=-30,roll=0),False)
  light=actor.light_component
  for key,value in [('intensity',100000.0),('use_temperature',True),('temperature',5400.0),('light_source_angle',1.5)]: light.set_editor_property(key,value)
  updated.append('Sun_Daylight')
 if actor.get_actor_label()=='Sky_Ambient':
  actor.light_component.set_editor_property('intensity',2.0)
  actor.light_component.set_editor_property('real_time_capture',True)
  updated.append('Sky_Ambient')
if len(updated)!=2: raise RuntimeError('Expected sun and sky')
if not levels.save_current_level(): raise RuntimeError('Midday map save failed')
(root/'Saved/Logs/MiddayLighting.json').write_text(json.dumps({'sun_elevation_degrees':75,'sun_lux':100000,'temperature_k':5400,'sky_intensity':2,'lumen':'existing project settings preserved','updated':updated},indent=2))
unreal.log('FOUNDRYLINE_WARM_MIDDAY_SAVED')
