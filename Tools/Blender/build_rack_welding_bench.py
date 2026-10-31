"""Model-only draft: full-size truck headache rack and reusable welding bench.
Internal Blender metres; manifests in centimetres. No game recipes or Unreal changes.
"""
import bpy,bmesh,math,json,sys
from pathlib import Path
from mathutils import Vector,Matrix
ROOT=Path(r'C:/Users/curly/Documents/Foundryline Game')
RACK=ROOT/'SourceAssets/Products/HeadacheRack';BENCH=ROOT/'SourceAssets/Production/Equipment/WeldingBench'
for p in (RACK,BENCH):p.mkdir(parents=True,exist_ok=True)
M={};GROUPS={};RACK_OBJECTS=[];BENCH_OBJECTS=[]
def mat(name,color,metal=.0,rough=.45,emission=0):
 m=bpy.data.materials.new(name);m.diffuse_color=(*color,1);m.use_nodes=True;s=m.node_tree.nodes.get('Principled BSDF');s.inputs['Base Color'].default_value=(*color,1);s.inputs['Metallic'].default_value=metal;s.inputs['Roughness'].default_value=rough
 if emission:s.inputs['Emission Color'].default_value=(*color,1);s.inputs['Emission Strength'].default_value=emission
 M[name]=m;return m
def materials():
 mat('Raw carbon steel',(.035,.046,.057),.82,.62);mat('Structural carbon steel',(.035,.046,.057),.82,.62);mat('Machined steel',(.22,.245,.26),.85,.48);mat('Fixture steel',(.06,.075,.085),.85,.58);mat('Rubber',(.025,.032,.035),.05,.7);mat('Lens',(.80,.91,1),.25,.19);mat('LED',(.88,.94,1),.1,.25,1.7);mat('Amber',(.95,.37,.025),.15,.3,.5);mat('Copper',(.40,.15,.055),.83,.28);mat('Label',(.90,.91,.88),.05,.5);mat('Annotation',(.07,.10,.12),0,.65);mat('Studio',(.72,.74,.73),0,.7)
 ann=M['Annotation'];nt=ann.node_tree;nt.nodes.clear();out=nt.nodes.new('ShaderNodeOutputMaterial');e=nt.nodes.new('ShaderNodeEmission');e.inputs['Color'].default_value=(.025,.04,.05,1);nt.links.new(e.outputs[0],out.inputs['Surface'])
 # Unpainted hot-rolled steel: subtle mill scale, satin finish, fine surface grain.
 for key in ['Raw carbon steel','Structural carbon steel','Fixture steel']:
  nt=M[key].node_tree;shader=nt.nodes.get('Principled BSDF');coord=nt.nodes.new('ShaderNodeTexCoord');tex=nt.nodes.new('ShaderNodeTexNoise');tex.inputs['Scale'].default_value=23;tex.inputs['Detail'].default_value=3;nt.links.new(coord.outputs['Object'],tex.inputs['Vector'])
  ramp=nt.nodes.new('ShaderNodeValToRGB');ramp.color_ramp.elements[0].position=.18;ramp.color_ramp.elements[0].color=(.016,.022,.029,1);ramp.color_ramp.elements[1].position=.82;ramp.color_ramp.elements[1].color=(.065,.078,.089,1);nt.links.new(tex.outputs['Fac'],ramp.inputs[0]);nt.links.new(ramp.outputs['Color'],shader.inputs['Base Color'])
  rough=nt.nodes.new('ShaderNodeMapRange');rough.inputs['From Min'].default_value=0;rough.inputs['From Max'].default_value=1;rough.inputs['To Min'].default_value=.49;rough.inputs['To Max'].default_value=.72;nt.links.new(tex.outputs['Fac'],rough.inputs['Value']);nt.links.new(rough.outputs['Result'],shader.inputs['Roughness'])
  grain=nt.nodes.new('ShaderNodeTexNoise');grain.inputs['Scale'].default_value=950;grain.inputs['Detail'].default_value=2;nt.links.new(coord.outputs['Object'],grain.inputs['Vector']);bump=nt.nodes.new('ShaderNodeBump');bump.inputs['Strength'].default_value=.13;bump.inputs['Distance'].default_value=.00014;nt.links.new(grain.outputs['Fac'],bump.inputs['Height']);nt.links.new(bump.outputs['Normal'],shader.inputs['Normal'])
 mat('Weld steel',(.10,.12,.14),.86,.52)

 # Painted table frame and conductive polished worktop; rack materials stay unchanged.
 for name,color in [('Bench gloss orange',(.55,.085,.006)),('Bench gloss graphite',(.018,.024,.03))]:
  paint=mat(name,color,0,.20);bs=paint.node_tree.nodes.get('Principled BSDF');bs.inputs['Coat Weight'].default_value=.45;bs.inputs['Coat Roughness'].default_value=.12
 top=mat('Bench polished steel',(.34,.37,.40),1,.22);nt=top.node_tree;bs=nt.nodes.get('Principled BSDF');tex=nt.nodes.new('ShaderNodeTexNoise');tex.inputs['Scale'].default_value=1600;tex.inputs['Detail'].default_value=1;coord=nt.nodes.new('ShaderNodeTexCoord');nt.links.new(coord.outputs['Object'],tex.inputs['Vector']);bump=nt.nodes.new('ShaderNodeBump');bump.inputs['Strength'].default_value=.07;bump.inputs['Distance'].default_value=.000025;nt.links.new(tex.outputs['Fac'],bump.inputs['Height']);nt.links.new(bump.outputs['Normal'],bs.inputs['Normal'])

def collection(name):
 c=bpy.data.collections.new(name);bpy.context.scene.collection.children.link(c);return c
def move(obj,c):
 for old in list(obj.users_collection):old.objects.unlink(obj)
 c.objects.link(obj)
def mesh(name,verts,faces,material,c,parent=None,location=(0,0,0),bevel=0):
 data=bpy.data.meshes.new(name);data.from_pydata(verts,[],faces);data.update();bm=bmesh.new();bm.from_mesh(data);bmesh.ops.recalc_face_normals(bm,faces=bm.faces);bm.to_mesh(data);bm.free();o=bpy.data.objects.new(name,data);c.objects.link(o);o.data.materials.append(M[material]);o.location=location
 if parent:o.parent=parent
 if bevel:
  m=o.modifiers.new('Small manufactured edge radius','BEVEL');m.width=bevel;m.segments=2;m.affect='EDGES'
 return o
def box(name,size,center,material,c,parent=None,bevel=.001):
 sx,sy,sz=(v/2 for v in size);v=[(x*sx,y*sy,z*sz) for x,y,z in [(-1,-1,-1),(1,-1,-1),(1,1,-1),(-1,1,-1),(-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1)]];f=[(0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)]
 return mesh(name,v,f,material,c,parent,center,bevel)
def cylinder(name,radius,depth,center,material,c,parent=None,axis='Z',verts=20):
 bpy.ops.mesh.primitive_cylinder_add(vertices=verts,radius=radius,depth=depth,location=center);o=bpy.context.object;o.name=name;move(o,c);o.data.materials.append(M[material]);
 if axis=='Y':o.rotation_euler.x=math.pi/2
 if axis=='X':o.rotation_euler.y=math.pi/2
 if parent:o.parent=parent
 return o
def tube(name,length,a,b,wall,center,axis,material,c,parent=None):
 # Four actual steel walls, inner/outer surfaces, and annular end faces.
 outer=[(-a/2,-b/2),(a/2,-b/2),(a/2,b/2),(-a/2,b/2)];inner=[(-a/2+wall,-b/2+wall),(a/2-wall,-b/2+wall),(a/2-wall,b/2-wall),(-a/2+wall,b/2-wall)];v=[]
 for z in [-length/2,length/2]:
  for ring in [outer,inner]:
   for x,y in ring:v.append((z,x,y) if axis=='X' else (x,z,y) if axis=='Y' else (x,y,z))
 f=[]
 for i in range(4):j=(i+1)%4;f.extend([(i,j,8+j,8+i),(4+j,4+i,12+i,12+j),(i,4+i,4+j,j),(8+j,12+j,12+i,8+i)])
 return mesh(name,v,f,material,c,parent,center,.0008)
def group(name,c,parent,kind):
 o=bpy.data.objects.new(name,None);c.objects.link(o);o.parent=parent;o['inventory_class']=kind;o['visual_group']=True;GROUPS[name]=o;return o
def hole(plate,at,r=.007,depth=.06,axis='Z'):
 bpy.ops.mesh.primitive_cylinder_add(vertices=20,radius=r,depth=depth,location=at);cut=bpy.context.object
 if axis=='Y':cut.rotation_euler.x=math.pi/2
 if axis=='X':cut.rotation_euler.y=math.pi/2
 mod=plate.modifiers.new('True through hole','BOOLEAN');mod.operation='DIFFERENCE';mod.solver='EXACT';mod.object=cut;bpy.context.view_layer.objects.active=plate;bpy.ops.object.modifier_apply(modifier=mod.name);bpy.data.objects.remove(cut,do_unlink=True)
def triangle(name,points,thickness,center,c,parent,material='Raw carbon steel'):
 v=[(x,y,z) for x in [-thickness/2,thickness/2] for y,z in points];f=[(0,2,1),(3,4,5),(0,1,4,3),(1,2,5,4),(2,0,3,5)];return mesh(name,v,f,material,c,parent,center,.0007)
def cable(name,points,r,c,parent=None):
 curve=bpy.data.curves.new(name,'CURVE');curve.dimensions='3D';curve.resolution_u=12;curve.bevel_depth=r;curve.bevel_resolution=2;s=curve.splines.new('BEZIER');s.bezier_points.add(len(points)-1)
 for p,co in zip(s.bezier_points,points):p.co=co;p.handle_left_type='AUTO';p.handle_right_type='AUTO'
 o=bpy.data.objects.new(name,curve);c.objects.link(o);o.data.materials.append(M['Rubber']);
 if parent:o.parent=parent
 return o
def rack():
 global GROUPS,RACK_OBJECTS
 GROUPS={};root=bpy.data.objects.new('HeadacheRack_AssemblyDatum',None);bpy.context.scene.collection.objects.link(root);root['units']='metres; datum at pickup bed-rail top';root['draft']=True
 C={n:collection('RACK / '+n) for n in ['FrameSections','SteelPlates','SteelBrackets','LightKits']}
 # Five genuinely mitered hollow frame sections; adjacent end rings meet exactly.
 path=[Vector(v) for v in [(-.9144,0,.0079375),(-.9144,0,.5588),(-.7874,0,.67945),(.7874,0,.67945),(.9144,0,.5588),(.9144,0,.0079375)]]
 normals=[]
 for a,b in zip(path,path[1:]):
  d=(b-a).normalized();normals.append(Vector((d.z,0,-d.x)))
 offsets=[normals[0]]
 for i in range(1,len(path)-1):
  n=(normals[i-1]+normals[i]).normalized();offsets.append(n/n.dot(normals[i]))
 offsets.append(normals[-1])
 for i,(a,b) in enumerate(zip(path,path[1:])):
  name=['Upright_L','AngledCabCorner_L','UpperCrossmember','AngledCabCorner_R','Upright_R'][i];g=group(name,C['FrameSections'],root,'FrameSections');v=[]
  for center,off in [(a,offsets[i]),(b,offsets[i+1])]:
   for half,depth in [(.03175,.0254),(.028575,.022225)]:
    for sign,y in [(-1,-depth),(1,-depth),(1,depth),(-1,depth)]:v.append(center+off*(half*sign)+Vector((0,y,0)))
  f=[]
  for k in range(4):j=(k+1)%4;f.extend([(k,j,8+j,8+k),(4+j,4+k,12+k,12+j),(k,4+k,4+j,j),(8+j,12+j,12+k,8+k)])
  mesh(name+'_Tube',v,f,'Raw carbon steel',C['FrameSections'],g,bevel=.0005)
  if i<4:
   o=cable('CornerWeld',[(b+offsets[i+1]*t+Vector((0,-.0256,0))) for t in [-.03175,0,.03175]],.0018,C['FrameSections'],g);o.data.materials.clear();o.data.materials.append(M['Weld steel'])
 g=group('LowerCrossmember',C['FrameSections'],root,'FrameSections');tube('LowerCrossmember_Tube',1.7653,.0508,.0381,.003175,(0,0,.1524),'X','Raw carbon steel',C['FrameSections'],g)
 for i in range(5):
  g=group('ProtectiveLouver_%02d'%(i+1),C['SteelPlates'],root,'SteelPlates');z=.2286+i*.0762
  p=[(-.027,-.019),(.022,.010),(.022,.019),(.019,.019),(.019,.012),(-.028,-.016)];v=[(x,y,q) for x in [-.88265,.88265] for y,q in p];n=len(p);f=[tuple(range(n-1,-1,-1)),tuple(range(n,2*n))]+[(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)];mesh('Louver_%02d_Steel'%(i+1),v,f,'Raw carbon steel',C['SteelPlates'],g,(0,0,z),.0005)
 for side in [-1,1]:
  g=group('BedRailSaddle_'+('L' if side<0 else 'R'),C['SteelBrackets'],root,'SteelBrackets');o=box('BedRailTopPlate_'+str(side),(.1524,.2794,.0079375),(side*.9144,0,.00396875),'Raw carbon steel',C['SteelBrackets'],g,.001)
  for dx in [-.0508,.0508]:
   for dy in [-.09525,.09525]:
    hole(o,(side*.9144+dx,dy,.00396875),.00714375);cylinder('BedRailBolt',.006,.040,(side*.9144+dx,dy,-.008),'Machined steel',C['SteelBrackets'],g);cylinder('BedRailBoltHead',.011,.008,(side*.9144+dx,dy,.0119375),'Machined steel',C['SteelBrackets'],g,verts=6)
  box('SaddleOuterDownstand',(.0079375,.2794,.0762),(side*.98663125,0,-.0381),'Raw carbon steel',C['SteelBrackets'],g,.001)
  g=group('StakePocketInsert_'+str(side),C['SteelBrackets'],root,'SteelBrackets');tube('StakePocketTube',.1016,.0508,.0508,.003175,(side*.9144,0,-.0508),'Z','Raw carbon steel',C['SteelBrackets'],g)
  for front in [-1,1]:
   g=group('MountGusset_%s_%s'%(side,front),C['SteelBrackets'],root,'SteelBrackets');triangle('GussetPlate',[(front*.0254,.0079375),(front*.127,.0079375),(front*.0254,.1349375)],.0047625,(side*.9144,0,0),C['SteelBrackets'],g)
   bead=cable('SaddleFilletWeld',[(side*.884,front*.026,.010),(side*.9144,front*.026,.010),(side*.944,front*.026,.010)],.0025,C['SteelBrackets'],g);bead.data.materials.clear();bead.data.materials.append(M['Weld steel'])
 for side in [-1,1]:
  g=group('WorkLightKit_'+('L' if side<0 else 'R'),C['LightKits'],root,'LightKits');g.location=(side*.61,.0564,.678);g.rotation_euler.z=math.pi
  box('LightMountBracket_'+str(side),(.075,.005,.035),(0,.0285,0),'Raw carbon steel',C['LightKits'],g,.001)
  box('RibbedLightHousing_'+str(side),(.118,.052,.065),(0,0,0),'Raw carbon steel',C['LightKits'],g,.007)
  for j in range(5):box('CoolingRib_%s_%s'%(side,j),(.108,.004,.004),(0,.027,-.024+j*.012),'Machined steel',C['LightKits'],g,.0006)
  box('LightBezel_'+str(side),(.110,.005,.057),(0,-.028,0),'Machined steel',C['LightKits'],g,.004)
  box('FrostedLens_'+str(side),(.094,.004,.043),(0,-.031,0),'Lens',C['LightKits'],g,.003)
  for j in range(4):cylinder('LEDCell_%s_%s'%(side,j),.007,.001,( -.033+j*.022,-.034,0),'LED',C['LightKits'],g,'Y',16)
  for x in [-.05,.05]:
   for z in [-.023,.023]:cylinder('LensScrew',.0022,.002,(x,-.032,z),'Raw carbon steel',C['LightKits'],g,'Y',8)
  box('AmberSideMarker_'+str(side),(.004,.029,.024),(side*.061,0,0),'Amber',C['LightKits'],g,.001)
 RACK_OBJECTS=[o for c in C.values() for o in c.objects];return root,C

def perforated_top(c,parent):
 v=[];f=[];N=16;th=.0125
 for ix in range(24):
  for iy in range(12):
   cx=-1.15+ix*.1;cy=-.55+iy*.1;start=len(v)
   for z in [-th/2,th/2]:
    for inner in [False,True]:
     for k in range(N):
      a=2*math.pi*k/N;cs=math.cos(a);sn=math.sin(a);r=.0078125 if inner else .05/max(abs(cs),abs(sn));v.append((cx+r*cs,cy+r*sn,z))
   for k in range(N):
    j=(k+1)%N;f.extend([(start+2*N+k,start+2*N+j,start+3*N+j,start+3*N+k),(start+j,start+k,start+N+k,start+N+j),(start+N+j,start+N+k,start+3*N+k,start+3*N+j)])
    a=v[start+k];b=v[start+j]
    if (abs(a[0]+1.2)<1e-6 and abs(b[0]+1.2)<1e-6) or (abs(a[0]-1.2)<1e-6 and abs(b[0]-1.2)<1e-6) or (abs(a[1]+.6)<1e-6 and abs(b[1]+.6)<1e-6) or (abs(a[1]-.6)<1e-6 and abs(b[1]-.6)<1e-6):f.append((start+k,start+j,start+2*N+j,start+2*N+k))
 o=mesh('FixtureTop_5eighthInHoles_4inPitch',v,f,'Bench polished steel',c,parent,(0,0,.89375));bm=bmesh.new();bm.from_mesh(o.data);bmesh.ops.remove_doubles(bm,verts=bm.verts,dist=.00001);bmesh.ops.recalc_face_normals(bm,faces=bm.faces);bm.to_mesh(o.data);bm.free();o['hole_diameter_cm']=1.5875;o['hole_pitch_cm']=10.16;o['hole_count']=288;return o

def bench(loaded=False,fixtures=True):
 global BENCH_OBJECTS
 C=collection('BENCH / Standard structure');A=collection('BENCH / Accessories');F=collection('BENCH / Removable fixture');root=bpy.data.objects.new('StandardWeldingBench_FloorDatum',None);C.objects.link(root);perforated_top(C,root)
 for x in [-1.06,1.06]:
  for y in [-.46,.46]:
   tube('Leg_80x80',.8225,.08,.08,.004,(x,y,.47625),'Z','Bench gloss orange',C,root);cylinder('LevellingFootPad',.055,.020,(x,y,.01),'Rubber',C,root);cylinder('LevellingStud',.008,.05,(x,y,.044),'Machined steel',C,root);cylinder('HexLockNut',.015,.012,(x,y,.054),'Machined steel',C,root,verts=6);cap=box('LegBottomCap',(.08,.08,.005),(x,y,.0625),'Bench gloss orange',C,root);hole(cap,(x,y,.0625),.0085)
 for y in [-.46,.46]:
  tube('UpperLongitudinalApron',2.04,.04,.06,.003,(0,y,.8575),'X','Bench gloss orange',C,root);tube('LowerLongitudinalBrace',2.04,.04,.04,.003,(0,y,.22),'X','Bench gloss graphite',C,root)
 for x in [-1.06,1.06]:
  tube('UpperSideApron',.84,.04,.06,.003,(x,0,.8575),'Y','Bench gloss orange',C,root);tube('LowerSideBrace',.84,.04,.04,.003,(x,0,.22),'Y','Bench gloss graphite',C,root)
 box('LowerToolShelf',(1.96,.77,.004),(0,0,.242),'Bench gloss graphite',A,root,.001)
 for y in [-.383,.383]:box('ShelfReturnLip',(1.96,.004,.035),(0,y,.257),'Bench gloss graphite',A,root,.001)
 for x in [-.8,.8]:
  box('FixtureStorageBin',(.26,.28,.10),(x,0,.295),'Bench gloss orange',A,root,.008)
  for i in range(3):cylinder('StoredFixturePin',.008,.11,(x-.07+i*.07,0,.38),'Machined steel',A,root)
 box('BrandPlate',(.47,.006,.054),(0,-.484,.853),'Bench gloss graphite',A,root,.002)
 brand=text_world('FOUNDRYLINE  /  WELD',(-.211,-.488,.841),.025,'Label',A,rotation=(math.pi/2,0,0));brand.parent=root
 # Real grounding lug and cable, separate from table structure.
 lug=box('GroundCableLug',(.055,.012,.055),(-1.10,-.501,.82),'Copper',A,root,.003);hole(lug,(-1.10,-.501,.831),.007,.04,'Y');cylinder('GroundTerminalBolt',.008,.018,(-1.10,-.516,.831),'Machined steel',A,root,'Y',6)
 cable('GroundLead',[(-1.10,-.52,.80),(-1.22,-.62,.53),(-1.32,-.66,.04),(-1.42,-.40,.025),(-1.52,-.60,.025)],.006,A,root)
 cylinder('GroundQuickConnector',.014,.065,(-1.52,-.60,.045),'Rubber',A,root)
 # A protected side holster and useful physical bench details.
 box('TorchHolster',(.06,.045,.12),(1.13,-.32,.79),'Bench gloss graphite',A,root,.004)
 for x in [-.98,.98]:
  for y in [-.38,.38]:cylinder('TableTopRetainingBolt',.008,.007,(x,y,.9035),'Machined steel',A,root,verts=6)
 # The same four removable riser/hold-down pairs can fixture other products.
 for side in ([-1,1] if fixtures else []):
  for row,y in enumerate([.15,-.25]):
   if loaded:
    x=side*.9;z=.9615;anchor=side*1.05;armz=1.15;padz=1.085
    box('FixtureRiser',(.10,.10,.123),(x,y,z),'Fixture steel',F,root,.001);box('RiserReplaceablePad',(.08,.08,.002),(x,y,1.024),'Machined steel',F,root,.0005)
   else:x=side*.47;y=-.12+row*.24;z=.303;anchor=x;armz=.432;padz=.3745;box('StoredFixtureRiser',(.10,.10,.123),(x,y,z),'Fixture steel',F,root,.001)
   cylinder('HoldDownAnchor',.0075,.30 if loaded else .24,(anchor,y,1.02 if loaded else armz-.08),'Machined steel',F,root);cylinder('AnchorCollar',.018,.015,(anchor,y,.9075 if loaded else .251),'Bench gloss graphite',F,root,verts=12)
   arm=box('HoldDownBridge',(.19,.035,.018),((anchor+x)/2,y,armz),'Bench gloss graphite',F,root,.002)
   hole(arm,(x,y,armz),.0065,.05);cylinder('HoldDownScrew',.006,.125 if loaded else .12,(x,y,1.1575 if loaded else .4445),'Machined steel',F,root);cylinder('SwivelPressurePad',.016,.020,(x,y,padz),'Machined steel',F,root);cylinder('ScrewTHandle',.005,.070,(x,y,1.215 if loaded else .4995),'Bench gloss orange',F,root,'X',12)
 scale=1.016
 for o in list(root.children_recursive):
  o.location*=scale
  if o.type=='MESH':
   for v in o.data.vertices:v.co*=scale
  elif o.type=='CURVE':
   o.data.bevel_depth*=scale
   for sp in o.data.splines:
    for q in sp.bezier_points:q.co*=scale;q.handle_left*=scale;q.handle_right*=scale
  elif o.type=='FONT':o.data.size*=scale;o.data.extrude*=scale
 BENCH_OBJECTS=[o for c in [C,A,F] for o in c.objects if o.type in {'MESH','CURVE','FONT'}];return root,[C,A,F]

def text_world(body,at,size,material,c,rotation=None):
 d=bpy.data.curves.new('Text_'+body[:18],'FONT');d.body=body;d.size=size;d.extrude=.00015;d.align_y='CENTER';o=bpy.data.objects.new(d.name,d);c.objects.link(o);o.location=at;o.data.materials.append(M[material]);
 if rotation:o.rotation_euler=rotation
 return o

def studio(target,scale=3.3,camera=(3,-4,2.8)):
 c=collection('REVIEW ONLY / Studio and dimensions');box('StudioGround',(200,200,.012),(0,0,-.018),'Studio',c,bevel=0)
 world=bpy.context.scene.world;world.use_nodes=True;world.node_tree.nodes['Background'].inputs[0].default_value=(.66,.69,.70,1);world.node_tree.nodes['Background'].inputs[1].default_value=.55
 for at,power,size in [((0,-3,5),1100,5),((-4,-1,2.8),650,4),((2.5,3.2,3.5),950,3)]:
  bpy.ops.object.light_add(type='AREA',location=at);o=bpy.context.object;move(o,c);o.data.energy=power;o.data.shape='DISK';o.data.size=size;o.rotation_euler=(Vector(target)-o.location).to_track_quat('-Z','Y').to_euler()
 bpy.ops.object.camera_add(location=camera);cam=bpy.context.object;move(cam,c);cam.rotation_euler=(Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=scale;cam.data.lens=50;bpy.context.scene.camera=cam
 scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=48;scene.cycles.use_denoising=True;scene.render.resolution_x=1800;scene.render.resolution_y=1400;scene.render.resolution_percentage=100;scene.view_settings.view_transform='AgX';scene.view_settings.look='AgX - Medium High Contrast';scene.render.image_settings.file_format='PNG';scene.world.color=(.5,.5,.5);return c,cam

def overlay(cam,c,title,lines):
 w=cam.data.ortho_scale;h=w*1400/1800;rot=cam.rotation_euler.to_matrix()
 for i,(text,size) in enumerate([(title,.085)]+[(s,.041) for s in lines]):
  local=Vector((-w/2+.12,h/2-.14-i*.11,-1));o=text_world(text,cam.location+rot@local,size,'Annotation',c);o.rotation_euler=cam.rotation_euler

def dim(c,cam,a,b,label,offset=(0,0,.045)):
 cable('Dimension', [a,b],.0013,c)
 for point in [a,b]:
  x,y,z=point;cable('DimensionTick',[(x-.012,y,z-.018),(x+.012,y,z+.018)],.0015,c)
 at=(Vector(a)+Vector(b))/2+Vector(offset);o=text_world(label,at,.043,'Annotation',c);o.rotation_euler=cam.rotation_euler;o.data.align_x='CENTER'

def clear():
 bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
 for c in list(bpy.data.collections):
  if c.name!='Collection' and not c.objects:bpy.data.collections.remove(c)
 bpy.context.scene.unit_settings.system='METRIC';bpy.context.scene.unit_settings.scale_length=1.;bpy.context.scene.unit_settings.length_unit='CENTIMETERS'

def descendants(root):return [root]+list(root.children_recursive)
def export(path,objects):
 bpy.ops.object.select_all(action='DESELECT')
 for o in objects:o.select_set(True)
 bpy.context.view_layer.objects.active=next((o for o in objects if o.type=='MESH'),objects[0]);bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'MESH','EMPTY','OTHER'},use_mesh_modifiers=True,add_leaf_bones=False,apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',bake_anim=False,use_custom_props=True)

def bounds(objects):
 pts=[o.matrix_world@Vector(p) for o in objects if o.type=='MESH' for p in o.bound_box];lo=Vector(tuple(min(p[i] for p in pts) for i in range(3)));hi=Vector(tuple(max(p[i] for p in pts) for i in range(3)));return {'min_cm':list(lo*100),'max_cm':list(hi*100),'size_cm':list((hi-lo)*100)}
def save_render(path,source):
 bpy.context.scene.render.filepath=str(path);bpy.ops.wm.save_as_mainfile(filepath=str(source));bpy.ops.render.render(write_still=True)

def run():
 clear();materials();root,classes=rack();bpy.context.view_layer.update();parts=[]
 for name,g in GROUPS.items():
  objs=descendants(g);parts.append({'visual_group':name,'inventory_class':g['inventory_class'],'assembly_transform_cm':{'translation':[v*100 for v in g.matrix_world.translation],'quaternion_wxyz':list(g.matrix_world.to_quaternion()),'scale':list(g.matrix_world.to_scale())},'bounds':bounds(objs),'objects':[o.name for o in objs if o.type=='MESH']})
 export(RACK/'HeadacheRack_AllSeparateParts.fbx',descendants(root));(RACK/'Parts').mkdir(exist_ok=True)
 for name,g in GROUPS.items():
  # Export mesh geometry in part-local space; manifest restores installed assembly transform.
  objs=descendants(g);saved={o:o.matrix_world.copy() for o in objs};inv=g.matrix_world.inverted()
  for o in objs:o.matrix_world=inv@saved[o]
  bpy.context.view_layer.update();export(RACK/'Parts'/(name+'.fbx'),objs)
  for o in objs:o.matrix_world=saved[o]
  bpy.context.view_layer.update()
 for stale in (RACK/'Parts').glob('*.fbx'):
  if stale.stem not in GROUPS:stale.unlink()
 manifest={'model':'Generic full-size pickup steel headache rack','status':'Pickup cab guard with bed-rail saddles and stake-pocket inserts; provisional universal fit','units':'centimetres; internal Blender metres; FBX unit conversion enabled','inventory_classes':['SteelPlates','SteelBrackets','FrameSections','LightKits'],'inventory_recipe_quantities':'Not assigned by this model task. Visual pieces are not new inventory SKUs.','physical_geometry_groups':parts,'installed_bounds':bounds(RACK_OBJECTS),'frame_nominal_cm':[189.23,5.08,71.12],'overall_nominal_cm':[198.12,27.94,81.28],'display_dimensions':'6 ft 6 in W x 11 in D; 28 in above bed rail, 4 in stake insert below','tube_wall_cm':.3175,'foot_plate_thickness_cm':.79375,'light_kits':2,'assembly_preview_order':['FrameSections','SteelPlates','SteelBrackets','LightKits'],'fixture_pose':{'rotation_euler_degrees_xyz':[90,0,0],'translation_cm':[0,30.48,106.68],'scale':[1,1,1],'support_height_cm':12.7,'rack_welding_plane_cm':109.22,'minimum_foot_clearance_above_table_cm':1.27},'not_a_manufacturing_specification':True}
 (RACK/'dimensions_and_groups.json').write_text(json.dumps(manifest,indent=2));c,cam=studio((0,0,.30),2.9,(2.8,-4.8,2.2));bpy.data.objects['StudioGround'].location.z=-.125;overlay(cam,c,'FOUNDRYLINE / HEADACHE RACK',['Pickup cab guard | 6 ft 6 in wide x 28 in above bed rail','Raw carbon steel | bed-rail saddles + 4 in stake-pocket inserts','Four inventory classes; editable visual groups'])
 dim(c,cam,(-.9906,-.24,.04),(.9906,-.24,.04),'6 ft 6 in',(0,-.02,.08));dim(c,cam,(1.08,0,0),(1.08,0,.7112),'28 in above rail',(.15,0,0));save_render(RACK/'Rack_Dimensioned.png',RACK/'Foundryline_HeadacheRack.blend')
 clear();root,cols=bench(False,False);bpy.context.view_layer.update();export(BENCH/'StandardWeldingBench_SeparateComponents.fbx',descendants(root));bench_manifest={'model':'Foundryline standard welding bench','status':'Draft reusable visual asset, not live gameplay','units':'centimetres','table_nominal_cm':[243.84,121.92,91.44],'display_dimensions':'8 ft W x 4 ft D x 36 in H','top_thickness_cm':1.27,'fixture_hole_diameter_cm':1.5875,'fixture_grid_pitch_cm':10.16,'fixture_hole_count':288,'leg_section_cm':[8.128,8.128],'leg_wall_cm':.4064,'suggested_clear_placement_area_cm':[340,220],'removable_fixture_riser_height_cm':12.7,'work_surface_bounds':bounds([o for o in BENCH_OBJECTS if o.name.startswith('FixtureTop_')]),'accessories':['Levelling feet','Grounding lug and cable','Lower shelf and fixture bins','Torch holster'],'overall_visible_bounds':bounds(BENCH_OBJECTS),'not_a_manufacturing_specification':True};(BENCH/'dimensions.json').write_text(json.dumps(bench_manifest,indent=2));c,cam=studio((0,0,.5),3.55,(3.6,-4.5,3.2));overlay(cam,c,'FOUNDRYLINE / STANDARD WELDING BENCH',['8 ft x 4 ft | 36 in work height','1/2 in steel top | 5/8 in fixture holes on a 4 in grid','Gloss painted frame + polished steel top | independent bench asset']);dim(c,cam,(-1.2192,-.70,.05),(1.2192,-.70,.05),'8 ft',(0,0,.08));dim(c,cam,(1.30,.60,0),(1.30,.60,.9144),'36 in',(.12,0,0));save_render(BENCH/'Bench_Dimensioned.png',BENCH/'Foundryline_StandardWeldingBench.blend')
 clear();broot,cols=bench(True);rroot,classes=rack();rroot.location=(0,.3048,1.0668);rroot.rotation_euler.x=math.pi/2;bpy.context.view_layer.update();loaded_bounds=bounds(RACK_OBJECTS);assert loaded_bounds['min_cm'][2]>=92.709,loaded_bounds
 c,cam=studio((0,0,.66),3.6,(3.1,-4.1,3.7));overlay(cam,c,'FOUNDRYLINE / RACK WELDING SETUP',['Same full-size PICKUP rack rotated into its fixture - never scaled','Four 5 in risers + screw hold-downs | weld plane about 43 in','Bed-rail saddles clear tabletop by 1/2 in; reusable standard bench']);save_render(BENCH/'Rack_OnBench_Fixture.png',BENCH/'Foundryline_RackWeldingSetup.blend')
 qa={'rack_geometry_groups':len(parts),'inventory_classes_only':list(classes),'rack_installed_nominal_cm':[198.12,27.94,81.28],'bench_top_cm':[243.84,121.92,91.44],'loaded_rack_bounds':loaded_bounds,'loaded_rack_scale':list(rroot.scale),'support_top_cm':104.14,'rack_frame_contact_plane_cm':104.14,'pressure_pad_contact_plane_cm':109.22,'foot_clearance_cm':loaded_bounds['min_cm'][2]-91.44,'finite_geometry':all(all(math.isfinite(v) for v in o.matrix_world.translation) for o in bpy.context.scene.objects),'source_scope':'Models and exports only. No Unreal map, recipes, roster or live welding implementation changed.'};(BENCH/'geometry_qa.json').write_text(json.dumps(qa,indent=2));print('RACK_BENCH_MODELS_COMPLETE',json.dumps(qa))
 clear();rr,cc=rack();c=collection('REVIEW ONLY / generic pickup reference');mat('Pickup reference',(.17,.19,.20),.15,.6);mat('Reference glass',(.035,.055,.065),.15,.4)
 # Cab rear window immediately ahead of rack; two longitudinal bed rails with true stake openings.
 cab=box('PickupCabReference',(1.84,1.30,1.32),(0,-.83,-.035),'Pickup reference',c,bevel=.10)
 box('RearCabWindow',(1.43,.015,.40),(0,-.172,.35),'Reference glass',c,bevel=.035)
 for side in [-1,1]:
  rail=box('PickupBedRailReference',(.20,2.15,.08),(side*.9144,.90,-.04),'Pickup reference',c,bevel=.008);hole(rail,(side*.9144,0,-.04),.040,.20)
  box('PickupBedSideReference',(.035,2.15,.48),(side*1.006,.90,-.32),'Pickup reference',c,bevel=.01)
 box('PickupBedFloorReference',(1.98,2.15,.04),(0,.90,-.57),'Pickup reference',c,bevel=.01)
 c,cam=studio((0,.20,.10),3.75,(3.7,4.9,2.8));bpy.data.objects['StudioGround'].location.z=-.75;overlay(cam,c,'FOUNDRYLINE / PICKUP MOUNTING REFERENCE',['Unfinished STEEL cab guard, not a freestanding floor frame','Bed-rail saddles bolt down; stake inserts locate below the rails','Generic grey pickup reference only - final truck fit is provisional']);save_render(RACK/'Rack_PickupMounted.png',RACK/'Foundryline_PickupMountingReview.blend')

def update_bench_only():
 clear();materials()
 clear();root,cols=bench(False,False);bpy.context.view_layer.update();export(BENCH/'StandardWeldingBench_SeparateComponents.fbx',descendants(root));bench_manifest={'model':'Foundryline standard welding bench','status':'Draft reusable visual asset, not live gameplay','units':'centimetres','table_nominal_cm':[243.84,121.92,91.44],'display_dimensions':'8 ft W x 4 ft D x 36 in H','top_thickness_cm':1.27,'fixture_hole_diameter_cm':1.5875,'fixture_grid_pitch_cm':10.16,'fixture_hole_count':288,'leg_section_cm':[8.128,8.128],'leg_wall_cm':.4064,'suggested_clear_placement_area_cm':[340,220],'removable_fixture_riser_height_cm':12.7,'work_surface_bounds':bounds([o for o in BENCH_OBJECTS if o.name.startswith('FixtureTop_')]),'accessories':['Levelling feet','Grounding lug and cable','Lower shelf and fixture bins','Torch holster'],'overall_visible_bounds':bounds(BENCH_OBJECTS),'not_a_manufacturing_specification':True};(BENCH/'dimensions.json').write_text(json.dumps(bench_manifest,indent=2));c,cam=studio((0,0,.5),3.55,(3.6,-4.5,3.2));overlay(cam,c,'FOUNDRYLINE / STANDARD WELDING BENCH',['8 ft x 4 ft | 36 in work height','1/2 in steel top | 5/8 in fixture holes on a 4 in grid','Gloss painted frame + polished steel top | independent bench asset']);dim(c,cam,(-1.2192,-.70,.05),(1.2192,-.70,.05),'8 ft',(0,0,.08));dim(c,cam,(1.30,.60,0),(1.30,.60,.9144),'36 in',(.12,0,0));save_render(BENCH/'Bench_Dimensioned.png',BENCH/'Foundryline_StandardWeldingBench.blend')
 clear();broot,cols=bench(True);rroot,classes=rack();rroot.location=(0,.3048,1.0668);rroot.rotation_euler.x=math.pi/2;bpy.context.view_layer.update();loaded_bounds=bounds(RACK_OBJECTS);assert loaded_bounds['min_cm'][2]>=92.709,loaded_bounds
 c,cam=studio((0,0,.66),3.6,(3.1,-4.1,3.7));overlay(cam,c,'FOUNDRYLINE / RACK WELDING SETUP',['Same full-size PICKUP rack rotated into its fixture - never scaled','Four 5 in risers + screw hold-downs | weld plane about 43 in','Bed-rail saddles clear tabletop by 1/2 in; reusable standard bench']);save_render(BENCH/'Rack_OnBench_Fixture.png',BENCH/'Foundryline_RackWeldingSetup.blend')

 # Export the removable fixture kit independently; no rack or table geometry is embedded.
 clear();fr,cols=bench(False,True);keep=set(cols[2].objects)
 for o in list(fr.children_recursive):
  if o not in keep:bpy.data.objects.remove(o,do_unlink=True)
 fr.name='WeldingFixtureKit_FloorDatum';bpy.context.view_layer.update();export(BENCH/'WeldingFixtureKit_SeparateComponents.fbx',descendants(fr));bpy.ops.wm.save_as_mainfile(filepath=str(BENCH/'Foundryline_WeldingFixtureKit.blend'))
 print('PAINTED_BENCH_AND_SEPARATE_FIXTURES_COMPLETE')

if __name__=='__main__':
 if '--bench-only' in sys.argv:update_bench_only()
 else:run()
