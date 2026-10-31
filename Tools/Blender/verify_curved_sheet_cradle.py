import bpy,json
from pathlib import Path
from mathutils import Vector
P=Path(r'C:/Users/curly/Documents/Foundryline Game/SourceAssets/Production/Equipment/CurvedSheetCradle')
bpy.ops.wm.open_mainfile(filepath=str(P/'Foundryline_CurvedSheetCradle.blend'));root=bpy.data.objects['CurvedSheetCradle_FloorCentre'];objects=list(root.children_recursive);assert not any('REVIEW_FullSheet' in o.name for o in objects);assert tuple(root.location)==(0,0,0);assert tuple(root.scale)==(1,1,1)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False);bpy.ops.import_scene.fbx(filepath=str(P/'CurvedSheetCradle_Complete.fbx'));meshes=[o for o in bpy.data.objects if o.type=='MESH'];assert not any('REVIEW_FullSheet' in o.name for o in meshes);v=[o.matrix_world@Vector(p) for o in meshes for p in o.bound_box];size=[(max(p[i] for p in v)-min(p[i] for p in v))*100 for i in range(3)];assert abs(size[0]-243.84)<.03 and abs(size[1]-182.88)<.03,size
qa={'result':'PASS','independent_cradle_meshes':len(meshes),'complete_export_dimensions_cm':size,'product_meshes_in_export':0,'source_origin':'Floor centre, unit scale','fork_opening_clear_inches':[7.5,3.5],'fork_entry':'Either six-foot end, along Y; not the eight-foot side','cradles_needed_in_later_layout':2,'saddles_per_cradle':2};(P/'export_qa.json').write_text(json.dumps(qa,indent=2));print('CURVED_CRADLE_EXPORT_QA',json.dumps(qa))
