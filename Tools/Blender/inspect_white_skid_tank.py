import bpy,re,json,math
from pathlib import Path
from mathutils import Vector,Matrix,Quaternion
R=Path(r'C:/Users/curly/Documents/Foundryline Game');SRC=R/'SourceAssets/MegapolySmallTrucks';OUT=R/'SourceAssets/Products/SkidTank/Review_2026-10-29';OUT.mkdir(parents=True,exist_ok=True)
def read_prefab(name):
 docs=re.split(r'--- !u!',(SRC/'Prefabs'/(name+'.prefab')).read_text())[1:];names={};trans={}
 def vec(body,key,keys):
  m=re.search(key+r': \{([^}]+)\}',body);d=dict(re.findall(r'(\w+): ([^,}]+)',m[1]));return [float(d[k]) for k in keys]
 for d in docs:
  kind,i=re.match(r'(\d+) &(\d+)',d).groups()
  if kind=='1':names[int(i)]=re.search('m_Name: (.+)',d)[1].strip()
  if kind=='4':trans[int(i)]=(int(re.search('m_GameObject: \\{fileID: (\\d+)',d)[1]),int(re.search('m_Father: \\{fileID: (\\d+)',d)[1]),Matrix.LocRotScale(Vector(vec(d,'m_LocalPosition','xyz')),Quaternion(vec(d,'m_LocalRotation','wxyz')),Vector(vec(d,'m_LocalScale','xyz'))))
 def world(i):
  g,p,m=trans[i];return world(p)@m if p else Matrix.Identity(4)
 return {names[g]:world(i) for i,(g,p,m) in trans.items() if g in names}
def reconstructed(name):
 bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(SRC/'Meshes'/(name+'.fbx')));by=read_prefab(name);conv=Matrix(((1,0,0,0),(0,0,-1,0),(0,1,0,0),(0,0,0,1)));obs=[]
 palette=bpy.data.materials.new('Licensed source palette');palette.use_nodes=True;bs=palette.node_tree.nodes.get('Principled BSDF');tex=palette.node_tree.nodes.new('ShaderNodeTexImage');tex.image=bpy.data.images.load(str(SRC/'Textures/Tex_256Gradient.png'));palette.node_tree.links.new(tex.outputs['Color'],bs.inputs['Base Color']);bs.inputs['Roughness'].default_value=.5
 for o in list(bpy.data.objects):
  if o.type!='MESH':continue
  assert o.name in by,o.name;o.parent=None;o.matrix_world=Matrix.Identity(4);o.data.transform(conv@by[o.name]@Matrix.Scale(2.54,4));o.data.materials.clear();o.data.materials.append(palette)
  for p in o.data.polygons:p.material_index=0
  obs.append(o)
 return obs
def bounds(obs):
 ps=[o.matrix_world@v.co for o in obs for v in o.data.vertices];lo=[min(p[i] for p in ps) for i in range(3)];hi=[max(p[i] for p in ps) for i in range(3)];return {'min':lo,'max':hi,'size':[b-a for a,b in zip(lo,hi)]}
def render(name,obs):
 bb=bounds(obs);target=Vector([(a+b)/2 for a,b in zip(bb['min'],bb['max'])]);bpy.ops.object.camera_add(location=target+Vector((10,-12,8)));cam=bpy.context.object;cam.rotation_euler=(target-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=13;bpy.context.scene.camera=cam
 for loc,power in [((0,-6,10),2200),((5,5,8),1600)]:
  bpy.ops.object.light_add(type='AREA',location=target+Vector(loc));l=bpy.context.object;l.data.energy=power;l.data.size=8;l.rotation_euler=(target-l.location).to_track_quat('-Z','Y').to_euler()
 w=bpy.data.worlds.new('Review world');w.use_nodes=True;w.node_tree.nodes['Background'].inputs[0].default_value=(.55,.58,.60,1);w.node_tree.nodes['Background'].inputs[1].default_value=.5;scene=bpy.context.scene;scene.world=w;scene.render.engine='CYCLES';scene.cycles.device='CPU';scene.cycles.samples=16;scene.render.threads_mode='FIXED';scene.render.threads=2;scene.render.resolution_x=1200;scene.render.resolution_y=750;scene.render.resolution_percentage=100;scene.render.filepath=str(OUT/(name+'_SourceIdentification.png'));bpy.ops.render.render(write_still=True)
if __name__=='__main__':
 rows={}
 for name in ['Truck_Tanker_1']:
  obs=reconstructed(name);rows[name]=[{'name':o.name,**bounds([o]),'faces':len(o.data.polygons)} for o in obs];render(name,obs)
 (OUT/'source_identification.json').write_text(json.dumps(rows,indent=2));print('SOURCE_TANKERS_IDENTIFIED')
