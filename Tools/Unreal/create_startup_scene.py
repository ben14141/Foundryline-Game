"""Create Foundryline's first site inside its editor, with no vendor modifications.

Existing target maps/materials are never overwritten. This script does not start
an editor or access any other game project. Run with the Foundryline project only.
"""
from pathlib import Path
import json
import math
import random
import unreal

project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
if not (project / 'Foundryline.uproject').is_file():
    raise RuntimeError('This script requires the Foundryline project.')
map_path = '/Game/Foundryline/Maps/L_StartupSite'
existing_map = unreal.EditorAssetLibrary.does_asset_exist(map_path)
if existing_map and (project / 'Saved/Setup/startup_scene.json').exists():
    raise RuntimeError('Completed startup map already exists; refusing to overwrite it.')

assets = unreal.AssetToolsHelpers.get_asset_tools()
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
report = {'map': map_path, 'ground_width_cm': 60000, 'vegetation': [], 'skipped': []}


def material(name, color):
    folder = '/Game/Foundryline/World/Materials'
    path = folder + '/' + name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.load_asset(path)
    result = assets.create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())
    node = unreal.MaterialEditingLibrary.create_material_expression(result, unreal.MaterialExpressionConstant3Vector, -250, 0)
    node.set_editor_property('constant', unreal.LinearColor(*color, 1.0))
    rough = unreal.MaterialEditingLibrary.create_material_expression(result, unreal.MaterialExpressionConstant, -250, 180)
    rough.set_editor_property('r', 0.95)
    unreal.MaterialEditingLibrary.connect_material_property(node, '', unreal.MaterialProperty.MP_BASE_COLOR)
    unreal.MaterialEditingLibrary.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.recompile_material(result)
    unreal.EditorAssetLibrary.save_loaded_asset(result)
    return result


def spawn(cls, name, location=(0, 0, 0), rotation=(0, 0, 0)):
    actor = actors.spawn_actor_from_class(cls, unreal.Vector(*location),
        unreal.Rotator(pitch=rotation[0], yaw=rotation[1], roll=rotation[2]))
    if not actor:
        raise RuntimeError('Could not spawn ' + name)
    actor.set_actor_label(name)
    return actor


def mesh_actor(name, mesh, location, scale=(1, 1, 1), mat=None):
    actor = spawn(unreal.StaticMeshActor, name, location)
    component = actor.static_mesh_component
    component.set_static_mesh(mesh)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    if mat:
        component.set_material(0, mat)
    return actor


if existing_map:
    if not levels.load_level(map_path):
        raise RuntimeError('Could not inspect unfinished startup map.')
    if any(a.get_actor_label() == 'Ground_600m_FlatGrass' for a in actors.get_all_level_actors()):
        raise RuntimeError('Existing map contains scene content; refusing to overwrite it.')
elif not levels.new_level(map_path):
    raise RuntimeError('Could not create startup map.')
grass = material('M_StartupGrass', (0.16, 0.24, 0.095))
boundary = material('M_PropertyBoundary', (0.035, 0.18, 0.29))
reference = material('M_ScaleReference', (0.65, 0.28, 0.07))
plane = unreal.load_asset('/Engine/BasicShapes/Plane.Plane')
cube = unreal.load_asset('/Engine/BasicShapes/Cube.Cube')
if not plane or not cube:
    raise RuntimeError('Engine reference meshes are unavailable.')
mesh_actor('Ground_600m_FlatGrass', plane, (0, 0, 0), (600, 600, 1), grass)

# A 100 x 80 m starter-property marker, independent of town size and land ownership.
for name, location, scale in (
    ('Property_North', (0, 4000, 2), (100, 0.08, 0.02)),
    ('Property_South', (0, -4000, 2), (100, 0.08, 0.02)),
    ('Property_East', (5000, 0, 2), (0.08, 80, 0.02)),
    ('Property_West', (-5000, 0, 2), (0.08, 80, 0.02)),
):
    actor = mesh_actor(name, cube, location, scale, boundary)
    actor.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
mesh_actor('ScaleReference_1_8m', cube, (-3500, -2500, 90), (0.45, 0.3, 1.8), reference)
mesh_actor('ScaleReference_1mCube', cube, (-3300, -2500, 50), (1, 1, 1), reference)

sun = spawn(unreal.DirectionalLight, 'Sun_Daylight', rotation=(-48, -35, 0))
sun.light_component.set_editor_property('intensity', 100000.0)
sun.light_component.set_editor_property('light_source_angle', 1.5)
sun.light_component.set_editor_property('atmosphere_sun_light', True)
sun.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
spawn(unreal.SkyAtmosphere, 'Sky_Atmosphere')
sky = spawn(unreal.SkyLight, 'Sky_Ambient')
sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
sky.light_component.set_editor_property('real_time_capture', True)
fog = spawn(unreal.ExponentialHeightFog, 'Distance_Haze')
fog.component.set_editor_property('fog_density', 0.002)
post = spawn(unreal.PostProcessVolume, 'Exposure_Daylight')
post.set_editor_property('unbound', True)
settings = post.get_editor_property('settings')
for prop, value in (
    ('override_auto_exposure_method', True), ('auto_exposure_method', unreal.AutoExposureMethod.AEM_MANUAL),
    ('override_auto_exposure_apply_physical_camera_exposure', True), ('auto_exposure_apply_physical_camera_exposure', True),
    ('override_camera_iso', True), ('camera_iso', 100.0),
    ('override_camera_shutter_speed', True), ('camera_shutter_speed', 200.0),
    ('override_depth_of_field_fstop', True), ('depth_of_field_fstop', 16.0),
):
    settings.set_editor_property(prop, value)
post.set_editor_property('settings', settings)
spawn(unreal.PlayerStart, 'PlayerStart_Foundryline', (0, 0, 100))

# Inspect original mesh bounds; use only plausible 1:1 assets and never rescale them.
rng = random.Random(14141)
names = ('SM_Tree_03', 'SM_Tree_04', 'SM_Tree_07', 'SM_Bush_01', 'SM_Bush_02', 'SM_Bush_03')
for name in names:
    mesh = unreal.load_asset('/Game/Factory/Meshes/Tree/' + name + '.' + name)
    if not mesh:
        report['skipped'].append({'mesh': name, 'reason': 'unavailable'})
        continue
    bounds = mesh.get_bounds()
    height = bounds.box_extent.z * 2.0
    minimum, maximum = (250, 2500) if 'Tree' in name else (20, 400)
    if not minimum <= height <= maximum:
        report['skipped'].append({'mesh': name, 'height_cm': height, 'reason': 'scale review required'})
        continue
    bottom = bounds.origin.z - bounds.box_extent.z
    for index in range(8 if 'Tree' in name else 12):
        angle = rng.uniform(0, 2 * math.pi)
        radius = rng.uniform(11000, 26000)
        actor = mesh_actor(name + '_' + str(index), mesh,
                           (math.cos(angle) * radius, math.sin(angle) * radius, -bottom))
        actor.set_actor_rotation(unreal.Rotator(pitch=0, yaw=rng.uniform(0, 360), roll=0), False)
        # Scenery only; authoritative obstacles need registered placeable definitions.
        actor.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    report['vegetation'].append({'mesh': name, 'height_cm': height, 'scale': 1.0})

unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).set_level_viewport_camera_info(
    unreal.Vector(-15000, -15000, 18000), unreal.Rotator(pitch=-40.3, yaw=45, roll=0))
if not levels.save_current_level():
    raise RuntimeError('Could not save startup map.')
(project / 'Saved/Setup').mkdir(parents=True, exist_ok=True)
(project / 'Saved/Setup/startup_scene.json').write_text(json.dumps(report, indent=2))
unreal.log('FOUNDRYLINE_STARTUP_SCENE_CREATED ' + map_path)
