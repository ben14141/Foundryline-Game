"""Fresh-process serialization and dependency check for original model publication."""
import json
from pathlib import Path
import unreal
ROOT=Path(r'C:/Users/curly/Documents/Foundryline Game')
report=json.loads((ROOT/'Saved/Logs/OriginalModelNativeImport.json').read_text())
registry=unreal.AssetRegistryHelpers.get_asset_registry()
result={'assets':[],'materials':[],'passed':False}
for row in report['assets']:
    mesh=unreal.load_asset(row['path']);assert isinstance(mesh,unreal.StaticMesh),row['path']
    bounds=mesh.get_bounds();size=[bounds.box_extent.x*2,bounds.box_extent.y*2,bounds.box_extent.z*2]
    assert all(abs(a-b)<.02 for a,b in zip(size,row['dimensions_cm'])),(row['path'],size)
    slots=mesh.get_editor_property('static_materials');assert len(slots)==len(row['materials'])
    for slot in slots:
        assert slot.material_interface and '/Game/Foundryline/Shared/Materials/OriginalModels/' in slot.material_interface.get_path_name()
    dependencies=[str(p) for p in registry.get_dependencies(row['path'],unreal.AssetRegistryDependencyOptions(include_hard_package_references=True,include_soft_package_references=True))]
    assert all(not p.startswith('/Game/') or p.startswith('/Game/Foundryline/Shared/Materials/OriginalModels/') for p in dependencies),(row['path'],dependencies)
    result['assets'].append({'path':row['path'],'size_cm':size,'material_slots':len(slots),'dependencies':dependencies})
for path in report['materials']:
    mat=unreal.load_asset(path);assert isinstance(mat,unreal.Material),path
    assert unreal.MaterialEditingLibrary.get_material_property_input_node(mat,unreal.MaterialProperty.MP_BASE_COLOR) or mat.get_editor_property('use_material_attributes'),path
    dependencies=[str(p) for p in registry.get_dependencies(path,unreal.AssetRegistryDependencyOptions(include_hard_package_references=True,include_soft_package_references=True))]
    assert not any(p.startswith('/Game/') for p in dependencies),(path,dependencies)
    result['materials'].append({'path':path,'dependencies':dependencies})
result['passed']=True
(ROOT/'Saved/Logs/OriginalModelSerializedVerification.json').write_text(json.dumps(result,indent=2))
unreal.log('ORIGINAL_MODELS_SERIALIZED_VERIFIED '+str(len(result['assets'])))
