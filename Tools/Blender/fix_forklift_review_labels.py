"""Replace inherited floating glyphs only in the inactive forklift quality copy."""
import bpy,bmesh,json,math
from pathlib import Path
from mathutils import Vector
R=Path(__file__).resolve().parents[2];O=R/'SourceAssets/Vehicles/Forklift/QualityReview_2026-10-29';bpy.ops.wm.open_mainfile(filepath=str(O/'Forklift_QualityReview.blend'));S=bpy.context.scene;S.frame_set(1);body=bpy.data.objects['Forklift_Body'];bm=bmesh.new();bm.from_mesh(body.data);white={i for i,m in enumerate(body.data.materials) if m and 'White' in m.name};remove={v for f in bm.faces if f.material_index in white and all(.8<v.co.z<1.25 for v in f.verts) for v in f.verts};assert len(remove)>100;count=len(remove);bmesh.ops.delete(bm,geom=list(remove),context='VERTS');bm.to_mesh(body.data);bm.free()
material=bpy.data.materials.new('FQ_SurfacePrintedWhite');material.diffuse_color=(.8,.83,.85,1);material.use_nodes=True;p=material.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(.8,.83,.85,1);p.inputs['Roughness'].default_value=.45
qa=[]
for side in [-1,1]:
 bpy.ops.object.text_add(location=(-2.10 if side<0 else -1.75,side*.70915,1.117),rotation=(math.pi/2,0,0 if side<0 else math.pi));o=bpy.context.object;o.name='SurfacePrint_FOUNDRYLINE_'+str(side);o.data.body='FOUNDRYLINE';o.data.size=.025;o.data.extrude=0;o.data.bevel_depth=0;bpy.ops.object.convert(target='MESH');o=bpy.context.object;o.data.materials.append(material);M=o.matrix_world.copy();o.parent=body;o.matrix_world=M;bpy.context.view_layer.update();gaps=[(abs((o.matrix_world@v.co).y)-.709)*1000 for v in o.data.vertices];assert min(gaps)>.14 and max(gaps)<.16;qa.append({'object':o.name,'surface':'flat service-panel paint','min_gap_mm':min(gaps),'max_gap_mm':max(gaps),'extrusion_mm':0})
def export(name,obs):
 bpy.ops.object.select_all(action='DESELECT')
 for o in obs:o.select_set(True)
 bpy.context.view_layer.objects.active=next(o for o in obs if o.type=='MESH');bpy.ops.export_scene.fbx(filepath=str(O/(name+'.fbx')),use_selection=True,object_types={'MESH','EMPTY','OTHER'},apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',bake_anim=False,add_leaf_bones=False,mesh_smooth_type='FACE')
export('Forklift_Body',[body,*body.children_recursive]);export('Forklift_Complete_Review',[o for o in bpy.data.objects if o.type in {'MESH','CURVE','EMPTY'}]);bpy.ops.wm.save_as_mainfile(filepath=str(O/'Forklift_QualityReview.blend'));(O/'label_surface_qa.json').write_text(json.dumps({'removed_inherited_glyph_vertices':count,'labels':qa,'only_inactive_review_source_changed':True},indent=2));print('FORKLIFT_FLOATING_LABELS_REPLACED',count)
