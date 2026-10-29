import bpy,re,json
from pathlib import Path
from mathutils import Matrix,Vector,Quaternion
root=Path(r'C:/Users/curly/Documents/Foundryline Game');src=root/'SourceAssets/MegapolySmallTrucks';out=root/'SourceAssets/StarterHauler';out.mkdir(exist_ok=True)
text=(src/'Prefabs/Truck_sidewall.prefab').read_text();docs=re.split(r'--- !u!',text)[1:];names={};trans={}
def vec(body,key,n):
 m=re.search(key+r': \{([^}]+)\}',body);v=dict(re.findall(r'(\w+): ([^,}]+)',m[1]));return [float(v[k]) for k in n]
for d in docs:
 kind,ident=re.match(r'(\d+) &(\d+)',d).groups()
 if kind=='1':names[int(ident)]=re.search(r'm_Name: (.+)',d)[1].strip()
 if kind=='4':
  gid=int(re.search(r'm_GameObject: \{fileID: (\d+)',d)[1]);parent=int(re.search(r'm_Father: \{fileID: (\d+)',d)[1]);p=vec(d,'m_LocalPosition','xyz');q=vec(d,'m_LocalRotation','wxyz');s=vec(d,'m_LocalScale','xyz');trans[int(ident)]=(gid,parent,Matrix.LocRotScale(Vector(p),Quaternion(q),Vector(s)))
def world(t):
 gid,parent,m=trans[t]
 return world(parent)@m if parent else Matrix.Identity(4)
byname={names[gid]:world(t) for t,(gid,parent,m) in trans.items() if gid in names}
bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(src/'Meshes/Truck_sidewall.fbx'))
convert=Matrix(((1,0,0,0),(0,0,-1,0),(0,1,0,0),(0,0,0,1)));objects=[]
for o in list(bpy.data.objects):
 if o.type!='MESH':continue
 if o.name not in byname:raise RuntimeError('Unmapped mesh '+o.name)
 mat=convert@byname[o.name]
 o.parent=None;o.matrix_world=Matrix.Identity(4)
 o.data.transform(mat)
 if o.data.has_custom_normals:
  bpy.context.view_layer.objects.active=o;o.select_set(True)
  bpy.ops.mesh.customdata_custom_splitnormals_clear();o.select_set(False)
 # Unity renderer overrides are not carried by the raw FBX material named Texture.
 # Preserve glass separately and use the game's light cab paint on the cab shell.
 material='TruckGlass' if o.name=='Cube.084' else 'TruckCabPaint' if o.name in ('Cube.076','Cube.082','Cube.096') else 'TruckPalette'
 m=bpy.data.materials.get(material) or bpy.data.materials.new(material)
 o.data.materials.clear();o.data.materials.append(m)
 for poly in o.data.polygons:poly.material_index=0
 objects.append(o)
lo=Vector(tuple(min(v.co[i] for o in objects for v in o.data.vertices) for i in range(3)));hi=Vector(tuple(max(v.co[i] for o in objects for v in o.data.vertices) for i in range(3)))
source_size=list((hi-lo)*100)
# Centre the complete vehicle; retain prefab dimensions and ground contact.
shift=Vector(((lo.x+hi.x)/2,(lo.y+hi.y)/2,lo.z))
for o in objects:
 for v in o.data.vertices:v.co-=shift
# Separate the loading-side drop gate so the forklift has a real opening.
import bmesh
side=bpy.data.objects['Cube.069'];gate_faces=[p for p in side.data.polygons if all(side.data.vertices[i].co.y>1.26 and side.data.vertices[i].co.z>1.125 for i in p.vertices)]
used=sorted({i for p in gate_faces for i in p.vertices});remap={v:i for i,v in enumerate(used)};pivot=Vector((-1.344,1.28,1.129513))
gmesh=bpy.data.meshes.new('LoadingSideGate');gmesh.from_pydata([side.data.vertices[i].co-pivot for i in used],[],[[remap[i] for i in p.vertices] for p in gate_faces]);gmesh.materials.append(bpy.data.materials['TruckCabPaint']);gate=bpy.data.objects.new('LoadingSideGate',gmesh);bpy.context.collection.objects.link(gate)
bpy.ops.object.select_all(action='DESELECT');gate.select_set(True);bpy.context.view_layer.objects.active=gate;bpy.ops.export_scene.fbx(filepath=str(out/'LoadingSideGate.fbx'),use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',add_leaf_bones=False)
bm=bmesh.new();bm.from_mesh(side.data);bm.faces.ensure_lookup_table();bmesh.ops.delete(bm,geom=[bm.faces[p.index] for p in gate_faces],context='FACES');bm.to_mesh(side.data);bm.free();bpy.data.objects.remove(gate,do_unlink=True)
# Complete the thin cab shell behind the source's window and trim pieces. This
# game-owned wrapper keeps the original bounds and does not resize the vendor cab.
def block(name,lo,hi):
 bpy.ops.mesh.primitive_cube_add(size=1,location=(Vector(lo)+Vector(hi))/2);o=bpy.context.object;o.name=name;o.dimensions=Vector(hi)-Vector(lo);bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);return o
shell=block('CabShell', (2.98,-1.195,1.31),(4.98,1.195,3.245))
for i,(lo,hi) in enumerate([
 ((3.075,-1.10,1.41),(4.89,1.10,3.15)),
 ((4.75,-1.105,2.045),(5.10,1.105,3.085)),
 ((3.46,-1.30,2.02),(4.75,1.30,3.105))]):
 cut=block('CabOpening',lo,hi);bpy.context.view_layer.objects.active=shell;mod=shell.modifiers.new('Opening'+str(i),'BOOLEAN');mod.operation='DIFFERENCE';mod.object=cut;bpy.ops.object.modifier_apply(modifier=mod.name);bpy.data.objects.remove(cut,do_unlink=True)
shell.data.materials.append(bpy.data.materials['TruckCabPaint']);bev=shell.modifiers.new('Pressed metal corners','BEVEL');bev.width=.025;bev.segments=3;bpy.context.view_layer.objects.active=shell;bpy.ops.object.modifier_apply(modifier=bev.name)
# Bake wrapper location so all exported pieces share the same vehicle origin.
shell.data.transform(shell.matrix_world);shell.matrix_world=Matrix.Identity(4);objects.append(shell)
rows=[]
parts=[]
for o in objects:
 vs=[v.co for v in o.data.vertices];a=Vector(tuple(min(v[i] for v in vs) for i in range(3)));b=Vector(tuple(max(v[i] for v in vs) for i in range(3)));parts.append({'name':o.name,'min_cm':list(a*100),'max_cm':list(b*100)})
for o in objects:
 if not o.name.startswith('Circle.'):continue
 vs=[v.co for v in o.data.vertices];a=Vector(tuple(min(v[i] for v in vs) for i in range(3)));b=Vector(tuple(max(v[i] for v in vs) for i in range(3)));rows.append({'name':o.name,'centre_cm':list((a+b)*50),'size_cm':list((b-a)*100)})
# Export each wheel around its real axle pivot, independently of the chassis.
for i,row in enumerate(rows):
 o=bpy.data.objects[row['name']];centre=Vector(row['centre_cm'])/100
 for v in o.data.vertices:v.co-=centre
 o.location=centre
 bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o
 o.location=(0,0,0)
 bpy.ops.export_scene.fbx(filepath=str(out/('Wheel'+str(i)+'.fbx')),use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',add_leaf_bones=False)
 o.location=centre
for o in list(objects):
 if o.name.startswith('Circle.'):
  objects.remove(o);bpy.data.objects.remove(o,do_unlink=True)
bpy.ops.object.select_all(action='DESELECT')
for o in objects:o.select_set(True)
bpy.context.view_layer.objects.active=objects[0];bpy.ops.object.join();body=bpy.context.object;body.name='StarterFlatbed';bpy.context.scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
bpy.ops.export_scene.fbx(filepath=str(out/'StarterFlatbed.fbx'),use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',add_leaf_bones=False)
bpy.ops.wm.save_as_mainfile(filepath=str(out/'StarterFlatbed.blend'))
(out/'manifest.json').write_text(json.dumps({'size_cm':source_size,'wheels':rows,'parts':parts,'source':'Truck_sidewall Unity prefab local transforms; original dimensions preserved'},indent=2))
