"""Build a flat, instanced forest scene in Foundryline. Original map/assets are preserved."""
from pathlib import Path
import json
import math
import random
import unreal

root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
if not (root / 'Foundryline.uproject').is_file():
    raise RuntimeError('Foundryline only')
target = '/Game/Foundryline/Maps/L_FlatForest'
lib = unreal.EditorAssetLibrary
if lib.does_asset_exist(target):
    if (root / 'Saved/Setup/flat_forest.json').exists():
        raise RuntimeError('Completed forest map already exists; refusing to overwrite.')
else:
    if not lib.duplicate_asset('/Game/Foundryline/Maps/L_StartupSite', target):
        raise RuntimeError('Could not preserve/duplicate startup map.')
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not levels.load_level(target):
    raise RuntimeError('Could not open forest map.')
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
tools = unreal.AssetToolsHelpers.get_asset_tools()
mesh_base = '/Game/StyleHex_Studio/Free_Packs/FREE_Stylized_Forest_Sample/Meshes'
report = {'map': target, 'ground_z_cm': 0, 'groups': [], 'clear_site_cm': [13000, 11000]}

# Preserve old reference objects and simple foliage, but hide them in this new map.
for actor in actors.get_all_level_actors():
    label = actor.get_actor_label()
    if label.startswith(('SM_Tree_', 'SM_Bush_', 'ScaleReference_', 'Property_')):
        actor.set_actor_hidden_in_game(True)
        actor.set_is_temporarily_hidden_in_editor(True)

# Flat textured ground: world-space UVs, no landscape displacement or sculpting.
folder = '/Game/Foundryline/World/Materials'
material_path = folder + '/M_FlatForestGround'
if lib.does_asset_exist(material_path):
    mat = unreal.load_asset(material_path)
else:
    mat = tools.create_asset('M_FlatForestGround', folder, unreal.Material, unreal.MaterialFactoryNew())
    if not mat:
        raise RuntimeError('Could not create ground material.')
    edit = unreal.MaterialEditingLibrary
    def node(cls, x, y):
        return edit.create_material_expression(mat, cls, x, y)
    position = node(unreal.MaterialExpressionWorldPosition, -900, 0)
    mask = node(unreal.MaterialExpressionComponentMask, -720, 0)
    mask.set_editor_property('r', True)
    mask.set_editor_property('g', True)
    edit.connect_material_expressions(position, '', mask, 'Input')
    uv = node(unreal.MaterialExpressionMultiply, -540, 0)
    uv.set_editor_property('const_b', 1.0 / 450.0)
    edit.connect_material_expressions(mask, '', uv, 'A')
    texture_root = '/Game/StyleHex_Studio/Shared_Resources/Free_Packs/Shared_Terrain/Shared_Terrain_Textures/Terrain_Textures_FSBF/'
    grass = node(unreal.MaterialExpressionTextureSample, -330, 0)
    grass.set_editor_property('texture', unreal.load_asset(texture_root + 'T_Fsbf_Terrain_Grass_1_BC'))
    edit.connect_material_expressions(uv, '', grass, 'Coordinates')
    macro_uv = node(unreal.MaterialExpressionMultiply, -540, 250)
    macro_uv.set_editor_property('const_b', 1.0 / 8000.0)
    edit.connect_material_expressions(mask, '', macro_uv, 'A')
    macro = node(unreal.MaterialExpressionTextureSample, -330, 250)
    macro.set_editor_property('texture', unreal.load_asset('/Game/StyleHex_Studio/Shared_Resources/Free_Packs/Shared_Textures/Nosies/T_Macro_Variation'))
    edit.connect_material_expressions(macro_uv, '', macro, 'Coordinates')
    variation = node(unreal.MaterialExpressionLinearInterpolate, -100, 200)
    variation.set_editor_property('const_a', 0.80)
    variation.set_editor_property('const_b', 1.12)
    edit.connect_material_expressions(macro, 'R', variation, 'Alpha')
    color = node(unreal.MaterialExpressionMultiply, 80, 0)
    edit.connect_material_expressions(grass, 'RGB', color, 'A')
    edit.connect_material_expressions(variation, '', color, 'B')
    edit.connect_material_property(color, '', unreal.MaterialProperty.MP_BASE_COLOR)
    rough = node(unreal.MaterialExpressionConstant, 80, 200)
    rough.set_editor_property('r', 0.93)
    edit.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
    edit.recompile_material(mat)
    lib.save_loaded_asset(mat)
for actor in actors.get_all_level_actors():
    if actor.get_actor_label() == 'Ground_600m_FlatGrass':
        actor.static_mesh_component.set_material(0, mat)

rng = random.Random(14141024)
groups = {}
def group(relative):
    if relative in groups:
        return groups[relative]
    mesh = unreal.load_asset(mesh_base + '/' + relative)
    if not mesh:
        raise RuntimeError('Missing forest mesh: ' + relative)
    bounds = mesh.get_bounds()
    asset_name = 'FT_' + relative.split('/')[-1]
    asset_folder = '/Game/Foundryline/World/Foliage'
    existing = asset_folder + '/' + asset_name
    component = unreal.load_asset(existing) if lib.does_asset_exist(existing) else tools.create_asset(
        asset_name, asset_folder, unreal.FoliageType_InstancedStaticMesh,
        unreal.FoliageType_InstancedStaticMeshFactory())
    component.set_editor_property('mesh', mesh)
    body = component.get_editor_property('body_instance')
    body.set_editor_property('collision_enabled', unreal.CollisionEnabled.NO_COLLISION)
    component.set_editor_property('body_instance', body)
    is_small = 'Grass_' in relative or 'Flower_' in relative
    component.set_editor_property('cast_shadow', not is_small)
    component.set_editor_property('cull_distance', unreal.Int32Interval(
        min=4500 if is_small else 22000, max=10000 if is_small else 90000))
    lib.save_loaded_asset(component)
    groups[relative] = [component, bounds, []]
    return groups[relative]

def put(relative, x, y, scale=1.0, embed=0.0):
    component, bounds, transforms = group(relative)
    bottom = bounds.origin.z - bounds.box_extent.z
    z = -bottom * scale - embed
    rotation = unreal.Rotator(pitch=0, yaw=rng.uniform(0, 360), roll=0)
    transforms.append(unreal.Transform(location=unreal.Vector(x, y, z), rotation=rotation,
                                      scale=unreal.Vector(scale, scale, scale)))

def clear_site(x, y, padding=0):
    return abs(x) < 6500 + padding and abs(y) < 5500 + padding

def road_reserve(x, y):
    return y < -4800 and abs(x - 500 * math.sin(y / 6500)) < 1400

trees = ('Trees/SM_Tree_Birch_1', 'Trees/SM_Tree_Birch_2',
         'Trees/SM_Tree_Broadleaf_1', 'Trees/SM_Tree_Broadleaf_2')
tree_centers = []
for ix in range(-32, 33):
    for iy in range(-32, 33):
        x, y = ix * 880 + rng.uniform(-260, 260), iy * 880 + rng.uniform(-260, 260)
        if clear_site(x, y, 500) or road_reserve(x, y):
            continue
        density = 0.64 + 0.20 * math.sin(x / 4100) * math.cos(y / 5300)
        if rng.random() > density:
            continue
        kind = rng.choices(trees, weights=(3, 2, 3, 2))[0]
        put(kind, x, y, rng.uniform(0.90, 1.12))
        tree_centers.append((x, y))

# Varied undergrowth around tree groups rather than a uniform lawn of props.
for x, y in tree_centers:
    for _ in range(rng.randint(1, 3)):
        bx, by = x + rng.uniform(-650, 650), y + rng.uniform(-650, 650)
        if not clear_site(bx, by) and not road_reserve(bx, by):
            put('Foliage/SM_Fol_Bush_' + str(rng.choice((1, 1, 2))), bx, by, rng.uniform(0.65, 1.05))
    if rng.random() < 0.16:
        put('Stones/SM_Stone_' + str(rng.choice((1, 2, 3))), x + rng.uniform(-300, 300),
            y + rng.uniform(-300, 300), rng.uniform(0.8, 1.1), 18)

# Grassy meadow fringes; buildable centre and the future access lane remain clear.
for _ in range(32000):
    x, y = rng.uniform(-28800, 28800), rng.uniform(-28800, 28800)
    if clear_site(x, y) or road_reserve(x, y):
        continue
    put('Foliage/SM_Fol_Grass_Clump_' + str(rng.randint(1, 5)), x, y, rng.uniform(0.75, 1.1))
for _ in range(3000):
    x, y = rng.uniform(-15000, 15000), rng.uniform(-15000, 15000)
    if clear_site(x, y) or road_reserve(x, y):
        continue
    put('Foliage/SM_Fol_Flower_Clump_' + str(rng.randint(1, 3)), x, y,
        rng.uniform(0.9, 1.2), -18)

for name, (component, bounds, transforms) in groups.items():
    unreal.InstancedFoliageActor.add_instances(editor.get_editor_world(), component, transforms)
    report['groups'].append({'mesh': name, 'instances': len(transforms),
                             'authored_height_cm': bounds.box_extent.z * 2})
world = editor.get_editor_world()
unreal.SystemLibrary.execute_console_command(world, 'r.EyeAdaptation.CachedLightingPreExposure 10')
editor.set_level_viewport_camera_info(unreal.Vector(-13500, -15500, 13500),
                                      unreal.Rotator(pitch=-37, yaw=49, roll=0))
if not levels.save_current_level():
    raise RuntimeError('Could not save forest scene.')
(root / 'Saved/Setup/flat_forest.json').write_text(json.dumps(report, indent=2))
unreal.log('FOUNDRYLINE_FLAT_FOREST_COMPLETE ' + str(sum(g['instances'] for g in report['groups'])))
