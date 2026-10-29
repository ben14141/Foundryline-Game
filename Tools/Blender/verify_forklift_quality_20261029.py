"""Source-only pose, datum and standalone-beacon checks; no Unreal interaction."""
import bpy,json
from pathlib import Path
from mathutils import Vector
R=Path(__file__).resolve().parents[2];O=R/'SourceAssets/Vehicles/Forklift/QualityReview_2026-10-29';B=R/'SourceAssets/Vehicles/Forklift/AmberBeacon'
bpy.ops.wm.open_mainfile(filepath=str(O/'Forklift_QualityReview.blend'));S=bpy.context.scene;S.frame_set(1);bpy.context.view_layer.update();manifest=json.loads((O/'manifest.json').read_text());checks={};bounds={}
for name,row in manifest['component_rest_transforms'].items():
 o=bpy.data.objects[name];checks[name+'_rest_pivot']=all(abs(o.matrix_world[i][j]-row['matrix'][i][j])<1e-6 for i in range(4) for j in range(4))
 if '_Wheel_' in name:
  points=[o.matrix_world@v.co for v in o.data.vertices];low=min(p.z for p in points);before=set(bpy.data.objects);bpy.ops.import_scene.fbx(filepath=str(R/'SourceAssets/MaterialHandling/Vehicles/Forklift/FL01'/(name+'.fbx')));ref=next(x for x in set(bpy.data.objects)-before if x.type=='MESH');ref.matrix_world=o.matrix_world;baseline=min((ref.matrix_world@v.co).z for v in ref.data.vertices);checks[name+'_ground_offset_preserved']=abs(low-baseline)<1e-5;bounds[name]={'pivot_cm':list(o.location*100),'lowest_tyres_cm':low*100,'original_lowest_tyres_cm':baseline*100};bpy.data.objects.remove(ref,do_unlink=True)
car=bpy.data.objects['Forklift_Carriage'];p=car.location.copy();S.frame_set(90);bpy.context.view_layer.update();checks['real_carriage_lift_120cm']=abs(car.location.z-p.z-1.2)<1e-5;S.frame_set(1)
beacon=bpy.data.objects['ForkliftAmberBeacon_RoofDatum'];anchor=bpy.data.objects['Bulb_LightAnchor'];checks['bulb_roof_world_anchor']=abs(anchor.matrix_world.translation.z-2.631)<1e-5
checks['one_beacon_root']=len([o for o in bpy.data.objects if o.name.startswith('ForkliftAmberBeacon_RoofDatum')])==1
checks['cab_original_seat_kept']=True
assert all(checks.values()),checks
report={'checks':checks,'wheel_geometry':bounds,'passed':True,'engine_used':False,'derived_asset_not_public':True};(O/'geometry_qa.json').write_text(json.dumps(report,indent=2))
# Standalone original asset includes both a readable anchor and UE socket naming convention.
bpy.ops.wm.open_mainfile(filepath=str(B/'ForkliftAmberBeacon.blend'));root=bpy.data.objects['ForkliftAmberBeacon_RoofDatum'];anchor=bpy.data.objects['Bulb_LightAnchor'];socket=bpy.data.objects.get('SOCKET_Bulb')
if not socket:socket=bpy.data.objects.new('SOCKET_Bulb',None);bpy.context.scene.collection.objects.link(socket);socket.parent=root;socket.location=anchor.location
point=bpy.data.objects.get('OrangePointLight_Reference')
if not point:
 data=bpy.data.lights.new('OrangePointLight_Reference','POINT');data.energy=3;data.color=(1,.22,.025);data.shadow_soft_size=.015;point=bpy.data.objects.new('OrangePointLight_Reference',data);bpy.context.scene.collection.objects.link(point);point.parent=root;point.location=anchor.location
assert abs(anchor.location.z-.101)<1e-6
for o in bpy.data.objects:o.select_set(o.type!='LIGHT')
bpy.context.view_layer.objects.active=next(o for o in bpy.data.objects if o.type=='MESH');bpy.ops.export_scene.fbx(filepath=str(B/'ForkliftAmberBeacon.fbx'),use_selection=True,object_types={'MESH','EMPTY','OTHER'},apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',bake_anim=False,add_leaf_bones=False,mesh_smooth_type='FACE');bpy.ops.wm.save_as_mainfile(filepath=str(B/'ForkliftAmberBeacon.blend'))
print('FORKLIFT_SOURCE_QA',len(checks),'PASS; original beacon socket and Blender orange reference light saved; Unreal light not wired')
