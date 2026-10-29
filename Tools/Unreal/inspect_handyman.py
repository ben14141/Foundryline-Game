"""Read-only purchased Handyman inventory; preserves vendor package paths."""
import unreal,json
from collections import Counter
from pathlib import Path
registry=unreal.AssetRegistryHelpers.get_asset_registry()
registry.scan_paths_synchronous(['/Game/Handyman'],force_rescan=True)
assets=registry.get_assets_by_path('/Game/Handyman',recursive=True)
report={'root':'/Game/Handyman','count':len(assets),'classes':{},'meshes':[],'animations':[],'failed_loads':[],'warnings':[]}
classes=Counter()
for data in assets:
 cls=str(data.asset_class_path.asset_name);classes[cls]+=1
 if cls not in ('SkeletalMesh','AnimSequence','Material','MaterialInstanceConstant','Skeleton','PhysicsAsset'):continue
 try:
  obj=data.get_asset()
  if not obj:report['failed_loads'].append(str(data.package_name));continue
  if cls=='SkeletalMesh':
   row={'path':obj.get_path_name(),'skeleton':str(obj.get_editor_property('skeleton').get_path_name()),'materials':[str(m.material_interface.get_path_name()) if m.material_interface else None for m in obj.get_editor_property('materials')]}
   try:
    b=obj.get_editor_property('imported_bounds');row['bounds_origin_cm']=[b.origin.x,b.origin.y,b.origin.z];row['bounds_extent_cm']=[b.box_extent.x,b.box_extent.y,b.box_extent.z]
   except Exception as e:row['bounds_note']=str(e)
   report['meshes'].append(row)
  if cls=='AnimSequence':
   report['animations'].append({'path':obj.get_path_name(),'seconds':obj.get_play_length(),'skeleton':obj.get_editor_property('skeleton').get_path_name()})
 except Exception as e:report['failed_loads'].append({'path':str(data.package_name),'error':str(e)})
report['classes']=dict(classes)
out=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()))/'Logs/HandymanImportReport.json'
out.write_text(json.dumps(report,indent=2));unreal.log('HANDYMAN_IMPORT_REPORT '+str(out)+' count='+str(len(assets)))
