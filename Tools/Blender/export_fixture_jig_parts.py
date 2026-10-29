"""Split one riser and one hold-down clamp out of Codex's original fixture kit so the station can arrange them as a jig.

Nothing is modelled or resized: the parts are copied from SourceAssets/PublishedOriginalModels/WeldingFixtureKit/
WeldingFixtureKit.blend (its -X/-Y corner set), moved so each part's origin is the riser's underside centre, and written
as FBX beside the kit. Run: blender --background <kit.blend> --python Tools/Blender/export_fixture_jig_parts.py
"""
import bpy,json
from mathutils import Vector
from pathlib import Path
OUT=Path(bpy.data.filepath).parent
riser=bpy.data.objects['StoredFixtureRiser']
vs=[riser.matrix_world@v.co for v in riser.data.vertices]
origin=Vector(((min(v.x for v in vs)+max(v.x for v in vs))/2,(min(v.y for v in vs)+max(v.y for v in vs))/2,min(v.z for v in vs)))
def export(name,parts):
 bpy.ops.object.select_all(action='DESELECT');copies=[]
 for n in parts:
  o=bpy.data.objects[n];c=o.copy();c.data=o.data.copy();c.parent=None;c.matrix_world=o.matrix_world.copy();bpy.context.scene.collection.objects.link(c);copies.append(c)
 for c in copies:c.location-=origin;c.select_set(True)
 bpy.context.view_layer.objects.active=copies[0];bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
 if len(copies)>1:bpy.ops.object.join()
 j=bpy.context.view_layer.objects.active;j.name=name
 bpy.ops.export_scene.fbx(filepath=str(OUT/(name+'.fbx')),use_selection=True,apply_unit_scale=True,bake_space_transform=False,object_types={'MESH'},mesh_smooth_type='FACE',add_leaf_bones=False)
 w=[j.matrix_world@v.co for v in j.data.vertices];return [[round(min(p[i] for p in w)*100,3) for i in range(3)],[round(max(p[i] for p in w)*100,3) for i in range(3)]]
report={'origin_in_kit_cm':[round(x*100,3) for x in origin],
 'WeldingFixtureRiser':export('WeldingFixtureRiser',['StoredFixtureRiser']),
 'WeldingFixtureHoldDown':export('WeldingFixtureHoldDown',['AnchorCollar','HoldDownAnchor','HoldDownBridge','SwivelPressurePad','HoldDownScrew','ScrewTHandle'])}
(OUT/'WeldingFixtureJigParts.json').write_text(json.dumps(report,indent=1));print('RESULT',json.dumps(report))
