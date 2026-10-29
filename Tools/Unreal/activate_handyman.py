"""Create game-owned appearance profile; never save runtime merged render meshes."""
import unreal,json
from pathlib import Path
root='/Game/Foundryline/Employees/Handyman'
unreal.EditorAssetLibrary.make_directory(root)
path=root+'/DA_HandymanEmployees'
if unreal.EditorAssetLibrary.does_asset_exist(path):asset=unreal.load_asset(path)
else:
 factory=unreal.DataAssetFactory();factory.set_editor_property('data_asset_class',unreal.FoundrylineEmployeeAppearance)
 asset=unreal.AssetToolsHelpers.get_asset_tools().create_asset('DA_HandymanEmployees',root,unreal.FoundrylineEmployeeAppearance,factory)
source='/Game/Handyman/Mesh/Handyman/Parts/'
sets=[['SK_Body_1','SK_Head_1','SK_Boots_1','SK_Hat_1'],['SK_Body_1','SK_Head_2','SK_Boots_1','SK_Hat_2','SK_Beard_1_2'],['SK_Body_4','SK_Head_3','SK_Boots_1','SK_Hat_1','SK_Beard_2_3']]
main=unreal.load_asset('/Game/Handyman/Mesh/Handyman/SK_Handyman_modular')
outfits=[]
for names in sets:
 outfit=unreal.FoundrylineEmployeeOutfit();outfit.set_editor_property('parts',[unreal.load_asset(source+p) for p in names]);outfits.append(outfit)
for name,value in {'mesh':main,'variants':[],'outfits':outfits,'handyman_pose':True,'idle':unreal.load_asset('/Game/Handyman/Animations/Handyman/anim_Idle'),'walk':unreal.load_asset('/Game/Handyman/Animations/Handyman/anim_Walk_1'),'walk_stride_cm':110,'facing_offset':unreal.Rotator(pitch=0,yaw=-90,roll=0)}.items():asset.set_editor_property(name,value)

if not unreal.EditorAssetLibrary.save_loaded_asset(asset):raise RuntimeError('Unable to save appearance')
proof={'profile':asset.get_path_name(),'source':json.loads(unreal.FoundrylineHandymanAssetTools.inspect_mesh(main)),'outfits':sets,'runtime_meshes':[]}
sub=unreal.get_editor_subsystem(unreal.SkeletalMeshEditorSubsystem)
for i,names in enumerate(sets):
 mesh=unreal.FoundrylineHandymanAssetTools.build_runtime_variant(i,[unreal.load_asset(source+p) for p in names]);count=sub.get_num_verts(mesh,0)
 if count<=0:raise RuntimeError('Runtime outfit has no rendered vertices')
 proof['runtime_meshes'].append({'mesh':mesh.get_path_name(),'vertices':count,'bones':len(json.loads(unreal.FoundrylineHandymanAssetTools.inspect_mesh(mesh))['bones'])})
Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()),'Logs/HandymanActivationProof.json').write_text(json.dumps(proof,indent=2))
