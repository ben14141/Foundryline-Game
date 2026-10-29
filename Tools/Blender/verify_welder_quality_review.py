"""Bounded source geometry/export checks; never loads Unreal or changes live assets."""
import bpy,json,hashlib
from pathlib import Path
from mathutils import Vector
R=Path(__file__).resolve().parents[2];O=R/'SourceAssets/Production/Equipment/Welder/QualityReview_2026-10-29';bpy.ops.wm.open_mainfile(filepath=str(O/'Foundryline_MIGWelder_QualityReview.blend'));checks={}
for n,target in [('Grip',(0,0,0)),('Support',(.03,-.03,.08)),('WeldTip',(.239,0,.135))]:checks['gun_'+n+'_unchanged']=(bpy.data.objects['SOCKET_'+n].location-Vector(target)).length<1e-6
nozzle=bpy.data.objects['CopperNozzle_OpenBore'];hit=nozzle.ray_cast(Vector((.24,.006,.135)),Vector((-1,0,0)),distance=.10)[0];checks['nozzle_has_real_open_bore']=not hit
checks['no_external_images']=not any(i.source=='FILE' for i in bpy.data.images);checks['no_linked_vendor_libraries']=not bpy.data.libraries
foot=bpy.data.objects['OpenFootRing'];zmin=min((foot.matrix_world@v.co).z for v in foot.data.vertices);checks['bottle_foot_contacts_cart_top']=abs(zmin-.38)<1e-6
checks['eight_independent_groups']=all(bpy.data.objects.get(n) and bpy.data.objects[n].type=='EMPTY' for n in ['CartFrame','CartWheels','PowerUnit','GasCylinder','GasRegulator','MIGGun','Leads','WorkClamp'])
checks['no_review_objects_saved_in_asset']=not any(o.name.startswith('REVIEW_') for o in bpy.data.objects)
mins=[]
for o in bpy.data.objects:
 if o.name.startswith('Tyre'):mins.append(min((o.matrix_world@v.co).z for v in o.data.vertices))
checks['all_wheels_ground_clear']=len(mins)==4 and all(0<=z<=.006 for z in mins)
# Reimport the actual local gun export to establish cm scale and socket placement.
bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(O/'Welder_MIGGun.fbx'));tip=next(o for o in bpy.data.objects if o.name.startswith('SOCKET_WeldTip'));grip=next(o for o in bpy.data.objects if o.name.startswith('SOCKET_Grip'));checks['fbx_tip_local_scale_preserved']=(tip.matrix_world.translation-Vector((.239,0,.135))).length<1e-5;checks['fbx_grip_origin_preserved']=grip.matrix_world.translation.length<1e-5
meshes=[o for o in bpy.data.objects if o.type=='MESH'];points=[o.matrix_world@v.co for o in meshes for v in o.data.vertices];gunbounds={'min_cm':[min(v[i] for v in points)*100 for i in range(3)],'max_cm':[max(v[i] for v in points)*100 for i in range(3)]}
before=json.loads((R/'Saved/Logs/WelderOriginalHashesBefore.json').read_text(encoding='utf-8-sig'));checks['original_source_assets_unchanged']=all(hashlib.sha256(Path(row['Path']).read_bytes()).hexdigest().upper()==row['Hash'] for row in before)
assert all(checks.values()),checks
(O/'geometry_qa.json').write_text(json.dumps({'checks':checks,'passed':True,'gun_local_bounds_cm':gunbounds,'original_hashes_unchanged':True,'no_engine_use':True},indent=2));print('WELDER_QUALITY_QA',len(checks),'PASS')
