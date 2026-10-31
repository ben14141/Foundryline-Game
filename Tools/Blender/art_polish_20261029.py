"""Separate review copies only. No Unreal interaction or production asset writes."""
import bpy,bmesh,math,json,hashlib,importlib.util
from pathlib import Path
from mathutils import Vector
ROOT=Path(r'C:/Users/curly/Documents/Foundryline Game');STAGE=ROOT/'SourceAssets/ArtPolish/2026-10-29';STAGE.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location('review_helpers',ROOT/'Tools/Blender/build_rack_welding_bench.py');h=importlib.util.module_from_spec(spec);spec.loader.exec_module(h)
ASSETS=[('WeldingBench','SourceAssets/Production/Equipment/WeldingBench/Foundryline_StandardWeldingBench.blend','StandardWeldingBench_FloorDatum'),('HeadacheRack','SourceAssets/Products/HeadacheRack/Foundryline_HeadacheRack.blend','HeadacheRack_AssemblyDatum'),('CurvedSheetCradle','SourceAssets/Production/Equipment/CurvedSheetCradle/Foundryline_CurvedSheetCradle.blend','CurvedSheetCradle_FloorCentre')]
WATCH=[ROOT/p for _,p,_ in ASSETS]+[ROOT/'SourceAssets/Production/Equipment/WeldingBench/StandardWeldingBench_SeparateComponents.fbx',ROOT/'SourceAssets/Products/HeadacheRack/HeadacheRack_AllSeparateParts.fbx',ROOT/'SourceAssets/Production/Equipment/CurvedSheetCradle/CurvedSheetCradle_Complete.fbx']
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
BEFORE={str(p):digest(p) for p in WATCH};(STAGE/'original_hashes_before.json').write_text(json.dumps(BEFORE,indent=2))
def material(name,color,metal,rough,coat=0):
 m=bpy.data.materials.get(name) or bpy.data.materials.new(name);m.use_nodes=True;bs=m.node_tree.nodes.get('Principled BSDF');bs.inputs['Base Color'].default_value=(*color,1);bs.inputs['Metallic'].default_value=metal;bs.inputs['Roughness'].default_value=rough;bs.inputs['Coat Weight'].default_value=coat;bs.inputs['Coat Roughness'].default_value=.16;return m

def ring(name,r,ri,depth,at,parent,mat):
 n=24;v=[]
 for z in [-depth/2,depth/2]:
  for radius in [r,ri]:
   for i in range(n):a=i*math.tau/n;v.append((radius*math.cos(a),radius*math.sin(a),z))
 f=[]
 for i in range(n):j=(i+1)%n;f.extend([(i,j,2*n+j,2*n+i),(n+j,n+i,3*n+i,3*n+j),(i,n+i,n+j,j),(2*n+j,3*n+j,3*n+i,2*n+i)])
 mesh=bpy.data.meshes.new(name);mesh.from_pydata(v,[],f);mesh.materials.append(mat);o=bpy.data.objects.new(name,mesh);bpy.context.scene.collection.objects.link(o);o.parent=parent;o.location=at;mod=o.modifiers.new('Washer edge chamfer','BEVEL');mod.width=.0003;mod.segments=2;return o

def bead(name,a,b,parent,mat,r=.0016):
 a,b=Vector(a),Vector(b);direction=(b-a).normalized();normal=direction.cross(Vector((0,1,0))).normalized()
 if normal.length<.5:normal=direction.cross(Vector((0,0,1))).normalized()
 tangent=direction.cross(normal).normalized();v=[];segments=20;sides=6
 for i in range(segments+1):
  p=a.lerp(b,i/segments);radius=r*(.9+.1*math.cos(i*math.tau/2.5))
  for j in range(sides):v.append(p+radius*(normal*math.cos(j*math.tau/sides)+tangent*math.sin(j*math.tau/sides)))
 faces=[tuple(range(sides-1,-1,-1)),tuple(segments*sides+j for j in range(sides))]
 for i in range(segments):
  for j in range(sides):k=i*sides+j;q=i*sides+(j+1)%sides;faces.append((k,q,q+sides,k+sides))
 me=bpy.data.meshes.new(name);me.from_pydata(v,[],faces);me.materials.append(mat);o=bpy.data.objects.new(name,me);bpy.context.scene.collection.objects.link(o);o.parent=parent;return o

def triangles(objects):
 total=0;dg=bpy.context.evaluated_depsgraph_get()
 for o in objects:
  if o.type in {'MESH','CURVE','FONT'}:
   e=o.evaluated_get(dg);m=e.to_mesh();m.calc_loop_triangles();total+=len(m.loop_triangles);e.to_mesh_clear()
 return total

def update_materials():
 for m in bpy.data.materials:
  if not m.use_nodes:continue
  bs=m.node_tree.nodes.get('Principled BSDF')
  if not bs:continue
  if m.name.startswith('Bench gloss orange'):bs.inputs['Base Color'].default_value=(.52,.085,.008,1);bs.inputs['Roughness'].default_value=.23;bs.inputs['Coat Weight'].default_value=.35
  if m.name.startswith('Bench polished steel'):bs.inputs['Roughness'].default_value=.24
  if m.name.startswith(('Raw carbon steel','Structural carbon steel')):
   for n in m.node_tree.nodes:
    if n.type=='VALTORGB':n.color_ramp.elements[0].color=(.028,.037,.048,1);n.color_ramp.elements[1].color=(.050,.061,.074,1)
    if n.type=='MAP_RANGE':n.inputs['To Min'].default_value=.47;n.inputs['To Max'].default_value=.61
  if m.name.startswith(('Bench gloss','Raw carbon steel','Structural carbon steel')):
   nt=m.node_tree;noise=nt.nodes.new('ShaderNodeTexNoise');noise.inputs['Scale'].default_value=850;noise.inputs['Detail'].default_value=1;coord=nt.nodes.new('ShaderNodeTexCoord');nt.links.new(coord.outputs['Object'],noise.inputs['Vector']);bump=nt.nodes.new('ShaderNodeBump');bump.inputs['Strength'].default_value=.06;bump.inputs['Distance'].default_value=.000018;nt.links.new(noise.outputs['Fac'],bump.inputs['Height']);nt.links.new(bump.outputs['Normal'],bs.inputs['Normal'])

def run():
 reports=[]
 for name,source,rootname in ASSETS:
  out=STAGE/name;out.mkdir(exist_ok=True);bpy.ops.wm.open_mainfile(filepath=str(ROOT/source));root=bpy.data.objects[rootname];bpy.context.view_layer.update();beforebounds=h.bounds(h.descendants(root));beforetri=triangles(h.descendants(root));original_names={o.name for o in root.children_recursive};update_materials();hardware=material('AP / Turned steel hardware',(.25,.29,.32),.9,.29);weld=material('AP / Weld metal',(.09,.11,.13),.8,.46);h.M={m.name:m for m in bpy.data.materials}
  for o in list(root.children_recursive):
   if o.type!='MESH':continue
   for mod in o.modifiers:
    if mod.type=='BEVEL':mod.harden_normals=True
   if o.name.startswith(('TableTopRetainingBolt','BedRailBoltHead','EntryWearFastener')):
    # A real annular washer stack, with the bolt head lifted only by washer thickness.
    p=o.location.copy();zmin=min(v.co.z for v in o.data.vertices)+p.z;th=.0012;o.location.z+=th;ring('AP_Washer_'+o.name,.0115,.0058,th,(p.x,p.y,zmin+th/2),o.parent,hardware)
   if o.name.startswith(('FixtureTop_','CurvedSteelSupportWeb','FootPlate','BedRailTopPlate')):
    m=o.modifiers.new('AP / Real edge break','BEVEL');m.width=.00045;m.segments=2;m.limit_method='ANGLE'
   if len(o.data.polygons)>5:
    n=o.modifiers.new('AP / Weighted manufactured normals','WEIGHTED_NORMAL');n.keep_sharp=True;n.weight=40
  if name=='WeldingBench':
   for x in [-1.03632,1.03632]:
    for y in [-.48768,.48768]:bead('AP_LegApronFillet', (x,y,.845),(x,y,.895),root,weld)
  elif name=='HeadacheRack':
   for side in [-1,1]:
    for front in [-1,1]:bead('AP_UprightSaddleWeld',(side*.884,front*.026,.010),(side*.944,front*.026,.010),root,weld,.0018)
  else:
   # Previously the 2in-high gap between pocket and saddle was only bridged visually.
   # Separate staging copy now has four real load-path shoes, above the fork channels.
   c=bpy.context.scene.collection
   for x in [-.508,.508]:
    for y in [-.508,.508]:
     h.box('AP_SaddleSupportShoe',(.22,.25,.0508),(x,y,.127),'Bench gloss graphite',c,root,.0015)
     bead('AP_ShoeFillet',(x-.09,y-.12,.153),(x+.09,y-.12,.153),root,weld)
  bpy.context.view_layer.update();afterbounds=h.bounds(h.descendants(root));aftertri=triangles(h.descendants(root));assert tuple(root.scale)==(1,1,1)
  for axis in [0,1]:assert abs(beforebounds['size_cm'][axis]-afterbounds['size_cm'][axis])<.05,(name,beforebounds,afterbounds)
  assert aftertri<beforetri*2.5+12000,(name,beforetri,aftertri)
  assert not any('REVIEW_FullSheet' in o.name for o in root.children_recursive)
  h.export(out/(name+'_ArtPolish_Complete.fbx'),h.descendants(root))
  # Replace review-only studio in the staging copy; never touch original source paths.
  for collection in list(bpy.data.collections):
   if collection.name.startswith('REVIEW ONLY / Studio'):
    for o in list(collection.objects):bpy.data.objects.remove(o,do_unlink=True)
  h.M={m.name:m for m in bpy.data.materials};target=(0,0,.5);scale=3.4;camera=(3.2,-4,2.5)
  if name=='HeadacheRack':target=(0,0,.30);scale=2.75;camera=(2.7,-4,1.9)
  if name=='CurvedSheetCradle':target=(0,0,.60);scale=3.9;camera=(4,-5,3.3)
  c,cam=h.studio(target,scale,camera)
  if name=='HeadacheRack':bpy.data.objects['StudioGround'].location.z=-.125
  bpy.context.scene.world.node_tree.nodes['Background'].inputs[1].default_value=.35
  for o in c.objects:
   if o.type=='LIGHT':o.data.energy*=.72
  h.overlay(cam,c,'FOUNDRYLINE / '+name.upper()+' ART REVIEW',['Separate polish copy - not adopted in the game','True scale retained | dressed edges, welds and washer stacks'])
  scene=bpy.context.scene;scene.cycles.device='CPU';scene.cycles.samples=24;scene.render.threads_mode='FIXED';scene.render.threads=2;scene.render.resolution_x=1280;scene.render.resolution_y=1000;scene.view_settings.exposure=-.15
  h.save_render(out/(name+'_ArtPolish.png'),out/(name+'_ArtPolish.blend'))
  reports.append({'asset':name,'before_triangles':beforetri,'after_triangles':aftertri,'before_bounds_cm':beforebounds,'after_bounds_cm':afterbounds,'unit_scale':[1,1,1],'added_meshes':[o.name for o in root.children_recursive if o.name not in original_names],'review_only':True})
 after={str(p):digest(p) for p in WATCH};assert after==BEFORE,'Production source/export changed during review';(STAGE/'art_polish_qa.json').write_text(json.dumps({'result':'PASS','originals_unchanged_sha256':after,'assets':reports,'engine_actions':'None; background Blender CPU only'},indent=2));print('ART_POLISH_REVIEW_COPIES_COMPLETE',json.dumps(reports))
if __name__=='__main__':run()
