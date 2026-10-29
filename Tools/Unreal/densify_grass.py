"""Dense cosmetic meadow grass with short distance culling; Foundryline only."""
from pathlib import Path
import json
import random
import unreal

root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
if not (root / 'Foundryline.uproject').is_file():
    raise RuntimeError('Foundryline only')
target = '/Game/Foundryline/Maps/L_FlatForest_Grass'
lib = unreal.EditorAssetLibrary
if lib.does_asset_exist(target):
    raise RuntimeError('Grass map already exists; refusing duplicate generation')
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not levels.load_level('/Game/Foundryline/Maps/L_FlatForest_Clean'):
    raise RuntimeError('Could not load clean forest')
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
rng = random.Random(241026)
tools = unreal.AssetToolsHelpers.get_asset_tools()
groups = []
for index in range(1, 6):
    mesh = unreal.load_asset('/Game/StyleHex_Studio/Free_Packs/FREE_Stylized_Forest_Sample/Meshes/Foliage/SM_Fol_Grass_Clump_' + str(index))
    ft = tools.create_asset('FT_DenseGrass_' + str(index), '/Game/Foundryline/World/Foliage',
                            unreal.FoliageType_InstancedStaticMesh, unreal.FoliageType_InstancedStaticMeshFactory())
    if not ft:
        raise RuntimeError('Could not create foliage definition')
    ft.set_editor_property('mesh', mesh)
    ft.set_editor_property('cull_distance', unreal.Int32Interval(min=2200, max=3500))
    ft.set_editor_property('cast_shadow', False)
    body = ft.get_editor_property('body_instance')
    body.set_editor_property('collision_enabled', unreal.CollisionEnabled.NO_COLLISION)
    ft.set_editor_property('body_instance', body)
    lib.save_loaded_asset(ft)
    # Existing sparse grass must also disappear at the same distance.
    old = unreal.load_asset('/Game/Foundryline/World/Foliage/FT_SM_Fol_Grass_Clump_' + str(index))
    unreal.InstancedFoliageActor.remove_all_instances(world, old)
    bounds = mesh.get_bounds()
    groups.append([ft, bounds.origin.z - bounds.box_extent.z, []])

# About two overlapping, variably sized clumps per square metre. Include the
# buildable meadow: decorative grass neither blocks placement nor navigation.
for ix in range(-445, 446):
    for iy in range(-445, 446):
        if rng.random() < 0.07:
            continue
        x, y = ix * 65 + rng.uniform(-23, 23), iy * 65 + rng.uniform(-23, 23)
        ft, bottom, transforms = rng.choices(groups, weights=[3, 3, 3, 2, 1])[0]
        scale = rng.uniform(0.8, 1.15)
        transforms.append(unreal.Transform(location=unreal.Vector(x, y, -bottom * scale - 1),
            rotation=unreal.Rotator(pitch=0, yaw=rng.uniform(0, 360), roll=0),
            scale=unreal.Vector(scale, scale, scale)))

counts = []
for ft, bottom, transforms in groups:
    unreal.InstancedFoliageActor.add_instances(world, ft, transforms)
    counts.append({'asset': ft.get_path_name(), 'added_instances': len(transforms), 'end_cull_cm': 3500})
if not unreal.EditorLoadingAndSavingUtils.save_map(world, target):
    raise RuntimeError('Could not save dense grass map')
(root / 'Saved/Setup/dense_grass.json').write_text(json.dumps({'map': target, 'groups': counts}, indent=2))
unreal.log('FOUNDRYLINE_DENSE_GRASS_SAVED ' + str(sum(c['added_instances'] for c in counts)))
