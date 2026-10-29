from pathlib import Path
import unreal
root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
if not (root / 'Foundryline.uproject').is_file():
    raise RuntimeError('Foundryline only')
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
levels.load_level('/Game/Foundryline/Maps/L_FlatForest_Grass')
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
for i in range(1, 6):
    old = unreal.load_asset('/Game/Foundryline/World/Foliage/FT_SM_Fol_Grass_Clump_' + str(i))
    unreal.InstancedFoliageActor.remove_all_instances(world, old)
    ft = unreal.load_asset('/Game/Foundryline/World/Foliage/FT_DenseGrass_' + str(i))
    assert ft.get_editor_property('cull_distance').max == 3500
if not levels.save_current_level():
    raise RuntimeError('Grass map save failed')
total = 0
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    for component in actor.get_components_by_class(unreal.FoliageInstancedStaticMeshComponent):
        mesh = component.get_editor_property('static_mesh')
        if mesh and 'SM_Fol_Grass_Clump_' in mesh.get_name():
            total += component.get_instance_count()
assert total > 700000, total
unreal.log('FOUNDRYLINE_DENSE_GRASS_VERIFIED ' + str(total))
