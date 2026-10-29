import bpy,json
from pathlib import Path
from mathutils import Matrix
P=Path(r'C:/Users/curly/Documents/Foundryline Game/SourceAssets/Production/Equipment/SheetRoller')
bpy.ops.wm.open_mainfile(filepath=str(P/'Foundryline_SheetRoller.blend'));bpy.context.scene.frame_set(1);o=bpy.data.objects['SheetRoller_UpperCarriers'];bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o;bpy.ops.object.transform_apply(location=False,rotation=True,scale=True);bpy.context.view_layer.update();dims=list(o.dimensions*100);assert 390<dims[1]<405 and 40<dims[2]<55,dims
bpy.ops.wm.save_as_mainfile(filepath=str(P/'Foundryline_SheetRoller.blend'));c=o.copy();c.data=o.data.copy();bpy.context.scene.collection.objects.link(c);c.animation_data_clear();c.parent=None;c.matrix_world=Matrix.Identity(4);bpy.ops.object.select_all(action='DESELECT');c.select_set(True);bpy.context.view_layer.objects.active=c;bpy.ops.export_scene.fbx(filepath=str(P/'SheetRoller_UpperCarriers.fbx'),use_selection=True,object_types={'MESH'},bake_anim=False,axis_forward='-Y',axis_up='Z');m=json.loads((P/'manifest.json').read_text())
for a in m['assets']:
 if a['name']=='SheetRoller_UpperCarriers':a['dimensions_cm']=dims
(P/'manifest.json').write_text(json.dumps(m,indent=2));print('UPPER_CARRIERS_AXIS_FIXED',dims)
