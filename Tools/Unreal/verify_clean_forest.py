import unreal
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/Foundryline/Maps/L_FlatForest_Clean')
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
for actor in actors:
    component = actor.get_component_by_class(unreal.StaticMeshComponent)
    mesh = component.get_editor_property('static_mesh') if component else None
    if mesh and mesh.get_path_name().startswith('/Game/Factory/Meshes/Tree/'):
        raise RuntimeError('Old foliage remains: ' + actor.get_actor_label())
unreal.log('FOUNDRYLINE_CLEAN_FOREST_RELOAD_VERIFIED')
