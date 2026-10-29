from pathlib import Path
import json
import unreal

root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
if not (root / 'Foundryline.uproject').exists():
    raise RuntimeError('Foundryline only')
base = '/Game/StyleHex_Studio/Free_Packs/FREE_Stylized_Forest_Sample'
report = {'meshes': [], 'materials': []}
for path in unreal.EditorAssetLibrary.list_assets(base + '/Meshes', recursive=True):
    mesh = unreal.load_asset(path)
    if isinstance(mesh, unreal.StaticMesh):
        bounds = mesh.get_bounds()
        report['meshes'].append({'path': path, 'height_cm': bounds.box_extent.z * 2,
            'radius_cm': max(bounds.box_extent.x, bounds.box_extent.y),
            'bottom_cm': bounds.origin.z - bounds.box_extent.z,
            'materials': [str(slot.material_interface.get_path_name()) if slot.material_interface else None
                          for slot in mesh.get_editor_property('static_materials')]})
for name in ('Mat_Foliage/MI_Fol_Bushes', 'Mat_Trees/MI_Tree_Leaves_Birch_A', 'Mat_Trees/MI_Tree_Leaves_Broadleaf'):
    mat = unreal.load_asset(base + '/Materials/' + name)
    report['materials'].append({'path': mat.get_path_name(),
        'scalar_parameters': [str(n) for n in unreal.MaterialEditingLibrary.get_scalar_parameter_names(mat)],
        'vector_parameters': [str(n) for n in unreal.MaterialEditingLibrary.get_vector_parameter_names(mat)]})
(root / 'Saved/Setup').mkdir(parents=True, exist_ok=True)
(root / 'Saved/Setup/forest_assets.json').write_text(json.dumps(report, indent=2))
unreal.log('FOUNDRYLINE_FOREST_INSPECTION_COMPLETE')
