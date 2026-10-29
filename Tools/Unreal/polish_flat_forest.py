from pathlib import Path
import json
import unreal

root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
if not (root / 'Foundryline.uproject').is_file():
    raise RuntimeError('Foundryline only')
editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = editor.get_editor_world()
if world.get_path_name().split('.')[0] != '/Game/Foundryline/Maps/L_FlatForest':
    raise RuntimeError('Open L_FlatForest before polishing.')
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
removed = []
for actor in actors.get_all_level_actors():
    label = actor.get_actor_label()
    mesh_component = actor.get_component_by_class(unreal.StaticMeshComponent)
    mesh = mesh_component.get_editor_property('static_mesh') if mesh_component else None
    if mesh and mesh.get_path_name().startswith('/Game/Factory/Meshes/Tree/'):
        if not actors.destroy_actor(actor):
            raise RuntimeError('Could not remove old foliage: ' + label)
        removed.append(label)
        continue
    if label.startswith(('SM_Tree_', 'SM_Bush_', 'ScaleReference_', 'Property_')):
        component = actor.get_component_by_class(unreal.StaticMeshComponent)
        if component:
            component.set_visibility(False, True)
        actor.set_actor_hidden_in_game(True)
    elif label == 'Sun_Daylight':
        actor.light_component.set_editor_property('light_source_angle', 3.0)
    elif label == 'Sky_Ambient':
        actor.light_component.set_editor_property('intensity', 1.6)
    elif label == 'Ground_600m_FlatGrass':
        pos = actor.get_actor_location()
        rot = actor.get_actor_rotation()
        if abs(pos.z) > 0.001 or abs(rot.pitch) > 0.001 or abs(rot.roll) > 0.001:
            raise RuntimeError('Ground is not flat at Z=0.')
        ground_report = {'z_cm': pos.z, 'pitch': rot.pitch, 'roll': rot.roll,
                         'material': actor.static_mesh_component.get_material(0).get_path_name()}
editor.set_level_viewport_camera_info(unreal.Vector(-13500, -15500, 13500),
                                      unreal.Rotator(pitch=-37, yaw=49, roll=0))
if not unreal.EditorLoadingAndSavingUtils.save_map(world, '/Game/Foundryline/Maps/L_FlatForest_Clean'):
    raise RuntimeError('Could not save clean forest map.')
(root / 'Saved/Setup/flat_ground_validation.json').write_text(json.dumps(ground_report, indent=2))
remaining = []
for actor in actors.get_all_level_actors():
    component = actor.get_component_by_class(unreal.StaticMeshComponent)
    mesh = component.get_editor_property('static_mesh') if component else None
    if mesh and mesh.get_path_name().startswith('/Game/Factory/Meshes/Tree/'):
        remaining.append(actor.get_actor_label())
(root / 'Saved/Setup/old_foliage_removal.json').write_text(json.dumps({'removed': removed, 'remaining': remaining}, indent=2))
if remaining:
    raise RuntimeError('Some old foliage remains.')
unreal.log('FOUNDRYLINE_FLAT_FOREST_POLISHED')
