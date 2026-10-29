import unreal
from pathlib import Path
import json
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
result=[]
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if actor.get_actor_label()=='Sun_Daylight':
        c=actor.light_component
        result.append({'rotation':str(actor.get_actor_rotation()),'forward':str(c.get_forward_vector()),'intensity':c.get_editor_property('intensity'),'color':str(c.get_editor_property('light_color')),'temperature':c.get_editor_property('temperature'),'visibility':c.is_visible()})
    if actor.get_actor_label()=='Exposure_Daylight':
        s=actor.get_editor_property('settings')
        result.append({'iso':s.camera_iso,'shutter':s.camera_shutter_speed,'fstop':s.depth_of_field_fstop,'bias':s.auto_exposure_bias})
for name in ('Asphalt','Concrete','Wall','White','Yellow','Preview'):
    mat=unreal.load_asset('/Game/Foundryline/Construction/Materials/M_'+name)
    result.append({'material':name,'tint':str(unreal.MaterialEditingLibrary.get_material_default_vector_parameter_value(mat,'Tint')),'shading':str(mat.get_editor_property('shading_model'))})
(root/'Saved/Setup/live_daylight.json').write_text(json.dumps(result,indent=2))
