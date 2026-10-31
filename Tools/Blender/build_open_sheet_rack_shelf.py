"""Original open-sided replacement for the game's sheet-rack shelf, cm bearings preserved."""
import bpy, json
from pathlib import Path
R=Path(__file__).resolve().parents[2]
out=R/'SourceAssets/Inventory/Storage/OpenSheetShelf';out.mkdir(parents=True,exist_ok=True)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
bpy.context.scene.unit_settings.system='METRIC'
m=bpy.data.materials.new('FL_OpenShelf_Graphite');m.diffuse_color=(.05,.065,.075,1);m.use_nodes=True
n=m.node_tree.nodes.get('Principled BSDF');n.inputs['Base Color'].default_value=m.diffuse_color;n.inputs['Metallic'].default_value=.55;n.inputs['Roughness'].default_value=.4
def box(name,at,size):
 bpy.ops.mesh.primitive_cube_add(size=1,location=at);o=bpy.context.object;o.name=name;o.dimensions=size;o.data.materials.append(m)
 bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 bevel=o.modifiers.new('Folded steel edges','BEVEL');bevel.width=.002;bevel.segments=2
 bpy.ops.object.modifier_apply(modifier=bevel.name)
for x in [-.55,.55]:box('Long bearing beam',(x,0,.06385),(.10,4.16,.1277))
for x in [-.595,.595]:box('Low shelf edge lip',(x,0,.125),(.01,4.16,.03))
for y in [-2.04,-1.36,-.68,0,.68,1.36,2.04]:box('Cross bearer',(0,y,.075),(1.10,.06,.065))
box('Steel shelf deck',(0,0,.1062),(1.10,4.12,.012))
bpy.ops.object.select_all(action='SELECT');bpy.context.view_layer.objects.active=bpy.context.selected_objects[0];bpy.ops.object.join();o=bpy.context.object;o.name='Rack_Shelf400'
bpy.context.scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
bpy.ops.wm.save_as_mainfile(filepath=str(out/'Rack_Shelf400.blend'))
bpy.ops.export_scene.fbx(filepath=str(out/'Rack_Shelf400.fbx'),use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',apply_unit_scale=True,bake_anim=False,mesh_smooth_type='FACE')
(out/'manifest.json').write_text(json.dumps({'original_authorship':True,'bearing_cm':12.77,'deck_cm':11.22,'both_faces_open':True,'destination':'/Game/Foundryline/Inventory/Storage/SheetRacking/SM_Rack_Shelf400'},indent=2))
