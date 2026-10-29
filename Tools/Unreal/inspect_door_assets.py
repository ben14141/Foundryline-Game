import unreal, json
registry=unreal.AssetRegistryHelpers.get_asset_registry()
results=[]
for asset in registry.get_assets_by_path('/Game/Factory/Meshes', recursive=True):
    if 'Door' not in str(asset.asset_name): continue
    mesh=asset.get_asset()
    if not isinstance(mesh,unreal.StaticMesh): continue
    b=mesh.get_bounds(); lo=b.origin-b.box_extent; hi=b.origin+b.box_extent
    results.append(dict(path=mesh.get_path_name(),min=[lo.x,lo.y,lo.z],max=[hi.x,hi.y,hi.z],materials=[str(m.material_interface.get_name()) if m.material_interface else '' for m in mesh.static_materials]))
open(r'C:/Users/curly/Documents/Foundryline Game/Saved/Logs/FactoryDoorBounds.json','w').write(json.dumps(results,indent=2))
