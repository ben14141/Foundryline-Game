"""Original 12 ft x 4 ft, 9 ft tall manufacturing sign; separate headless Blender process."""
import bpy,math,json
from pathlib import Path
from mathutils import Vector
ROOT=Path(r'C:/Users/curly/Documents/Foundryline Game/SourceAssets/Buildings/ManufacturingSign');ROOT.mkdir(parents=True,exist_ok=True)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
scene=bpy.context.scene;scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1
def material(name,color,metal=.0,rough=.4):
    m=bpy.data.materials.new(name);m.diffuse_color=(*color,1);m.use_nodes=True;p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*color,1);p.inputs['Metallic'].default_value=metal;p.inputs['Roughness'].default_value=rough;return m
mats={'Frame':material('Powdercoat_Graphite',(.035,.045,.055),.5,.32),'Panel':material('Editable_Sign_Paint',(.055,.07,.09),.05,.4),'Accent':material('Editable_Accent',(.9,.25,.035),.3,.35),'Hardware':material('Galvanized_Hardware',(.45,.49,.52),.85,.26)}
groups={k:[] for k in mats}
def box(name,at,size,group,bevel=.005):
    bpy.ops.mesh.primitive_cube_add(size=1,location=at);o=bpy.context.object;o.name=name;o.dimensions=size;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    if bevel:mod=o.modifiers.new('Real edge radius','BEVEL');mod.width=bevel;mod.segments=3;mod.affect='EDGES'
    o.data.materials.append(mats[group]);o.modifiers.new('Weighted surface normals','WEIGHTED_NORMAL');groups[group].append(o);return o
def cylinder(name,at,radius,depth,group,vertices=16):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices,radius=radius,depth=depth,location=at);o=bpy.context.object;o.name=name;o.data.materials.append(mats[group]);mod=o.modifiers.new('Machined edges','BEVEL');mod.width=.002;mod.segments=2;o.modifiers.new('Weighted normals','WEIGHTED_NORMAL');groups[group].append(o);return o
for side in [-1,1]:
    x=side*1.3716
    box('Square steel support', (x,0,1.32),(.1524,.1524,2.60),'Frame',.009)
    box('Welded base plate',(x,0,.014),(.42,.40,.028),'Hardware',.007)
    box('Post end cap',(x,0,2.625),(.166,.166,.018),'Frame',.006)
    for dx in [-.145,.145]:
        for dy in [-.125,.125]:
            cylinder('Anchor washer',(x+dx,dy,.035),.031,.008,'Hardware')
            cylinder('Hex anchor nut',(x+dx,dy,.052),.024,.026,'Hardware',6)
            cylinder('Anchor stud',(x+dx,dy,.074),.011,.028,'Hardware')
    # Cabinet mounting brackets and visible fasteners.
    for z in [1.64,2.58]:
        box('Back mounting saddle',(x,.105,z),(.24,.10,.10),'Hardware',.006)
        for dx in [-.085,.085]:cylinder('Bracket bolt',(x+dx,.13,z+.06),.012,.018,'Hardware',6)
W=3.6576;H=1.2192;Z=2.1336
box('Deep sealed sign cabinet',(0,0,Z),(W,.22,H),'Frame',.025)
for side in [-1,1]:
    y=side*.118
    box('Editable front or rear sign face',(0,y,Z),(W-.10,.018,H-.10),'Panel',.010)
    box('Raised branding stripe',(0,side*.134,Z-H/2+.125),(W-.20,.012,.065),'Accent',.004)
    for x in [-W/2+.038,W/2-.038]:
        for z in [Z-H/2+.06,Z+H/2-.06]:
            bolt=cylinder('Face retention screw',(x,side*.14,z),.009,.008,'Hardware',12);bolt.rotation_euler[0]=math.pi/2
for x in [-W/2+.025,W/2-.025]:box('Rolled corner cap',(x,0,Z),(.05,.25,H+.025),'Accent',.010)
box('Weatherproof top drip cap',(0,0,Z+H/2+.018),(W+.06,.29,.036),'Frame',.012)
# Ground weld gussets at both posts.
for side in [-1,1]:
    x=side*1.3716
    for dy in [-.11,.11]:
        verts=[(x-.11,dy,.028),(x+.11,dy,.028),(x,dy,.25)];mesh=bpy.data.meshes.new('Gusset profile');mesh.from_pydata(verts,[],[(0,1,2)]);o=bpy.data.objects.new('Triangular base gusset',mesh);scene.collection.objects.link(o);o.data.materials.append(mats['Frame']);mod=o.modifiers.new('Plate thickness','SOLIDIFY');mod.thickness=.012;mod=o.modifiers.new('Weld edge radius','BEVEL');mod.width=.003;mod.segments=2;groups['Frame'].append(o)
exported=[]
for group,objects in groups.items():
    bpy.ops.object.select_all(action='DESELECT')
    for o in objects:
        o.select_set(True);bpy.context.view_layer.objects.active=o
        for mod in list(o.modifiers):
            try:bpy.ops.object.modifier_apply(modifier=mod.name)
            except RuntimeError:pass
    bpy.context.view_layer.objects.active=objects[0];bpy.ops.object.join();o=bpy.context.object;o.name='SM_ManufacturingSign_'+group
    scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR');exported.append(o)
    bpy.ops.export_scene.fbx(filepath=str(ROOT/(o.name+'.fbx')),use_selection=True,apply_unit_scale=True,apply_scale_options='FBX_SCALE_ALL',axis_forward='-Y',axis_up='Z',bake_anim=False,mesh_smooth_type='FACE',add_leaf_bones=False)
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'Foundryline_ManufacturingSign.blend'))
(ROOT/'manifest.json').write_text(json.dumps({'units':'source metres / export centimetres','panel_width_cm':365.76,'panel_height_cm':121.92,'height_cm':276.92,'pivot':'ground at sign centre','face_y_cm':[-13.4,13.4],'text_center_z_cm':220,'parts':[o.name for o in exported]},indent=2))
# Neutral standalone modelling preview; no Unreal or live Blender session touched.
bpy.ops.object.camera_add(location=(5.2,-7,3.5));camera=bpy.context.object;camera.rotation_euler=(Vector((0,0,1.55))-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.type='ORTHO';camera.data.ortho_scale=5.2;scene.camera=camera
bpy.ops.object.light_add(type='AREA',location=(1,-4,6));bpy.context.object.data.energy=1200;bpy.context.object.data.shape='DISK';bpy.context.object.data.size=5
scene.world.color=(.25,.25,.25);scene.render.engine='CYCLES';scene.cycles.samples=32;scene.render.resolution_x=1400;scene.render.resolution_y=1050;scene.render.resolution_percentage=100;scene.render.image_settings.file_format='PNG';scene.render.filepath=str(ROOT/'ManufacturingSign_Model.png');bpy.ops.render.render(write_still=True)
