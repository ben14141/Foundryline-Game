"""Game-owned office staircase catalogue cutout; isolated background Blender only.
Matches FoundrylineOfficeStairComponent in cm. Never opens/saves a live scene.
"""
import bpy,json
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'SourceAssets/UI/BuildCatalogue';OUT.mkdir(parents=True,exist_ok=True)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)

def material(name,colour,metal=0,rough=.5):
 m=bpy.data.materials.new(name);m.use_nodes=True;p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*colour,1);p.inputs['Metallic'].default_value=metal;p.inputs['Roughness'].default_value=rough;return m
step=material('Foundryline_Stair_Galvanized',(.43,.25,.12),0,.5)
rail=material('Foundryline_Stair_Orange',(.025,.032,.038),.35,.32)
structure=material('Foundryline_Stair_Graphite',(.79,.78,.73),0,.62)
nosing=material('Foundryline_Stair_Nosing',(.14,.17,.18),.55,.42)

glass=material('Office_Clear_Glass',(.75,.87,.93),0,.12);glass.node_tree.nodes.get('Principled BSDF').inputs['Transmission Weight'].default_value=.95

def box(name,at,size,mat,bevel=0):
 bpy.ops.mesh.primitive_cube_add(size=1,location=Vector(at)/100);o=bpy.context.object;o.name=name;o.dimensions=Vector(size)/100;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(mat)
 if bevel:
  b=o.modifiers.new('Manufactured edges','BEVEL');b.width=bevel/100;b.segments=2;o.modifiers.new('Weighted normals','WEIGHTED_NORMAL')
 return o

def beam(name,a,b,thick,mat):
 if thick==2:return None
 a,b=Vector(a),Vector(b);o=box(name,(a+b)/2,((b-a).length,thick,thick),mat,.25);o.rotation_euler=(b-a).to_track_quat('X','Z').to_euler();return o

def post(at,height=110):
 p=Vector(at);box('Rail_Post',p+Vector((0,0,height/2)),(3,3,height),rail,.25);box('Bolted_Baseplate',(min(296.5,max(3.5,p.x)),min(508.5,max(3.5,p.y)),p.z+.5),(7,7,1),structure,.18)

for flight in range(2):
 x0,x1=(167,297) if flight else (3,133)
 for i in range(9):
  y=382-(i+.5)*28 if flight else 130+(i+.5)*28
  top=20+flight*160+(i+1)*(160/9)
  front=y+14 if flight else y-14
  box('Tread_%d_%02d'%(flight,i),((x0+x1)/2,y,top-2),(130,28,4),step,.35)
  box('Riser_%d_%02d'%(flight,i),((x0+x1)/2,front,top-80/9),(130,2,160/9),structure,.18)
  box('AntiSlip_Nosing',((x0+x1)/2,front+(-1.5 if flight else 1.5),top+.15),(130,3,.3),nosing,.05)
 lowy,highy=(382,130) if flight else (130,382)
 lowz=20+flight*160;highz=lowz+160
 for x in [x0+14,x1-14]:beam('Steel_Stringer',(x,lowy,lowz-12),(x,highy,highz-12),12,structure)
 for x in [x0-1.5,x1+1.5]:
  for h,t in [(110,3),(55,2)]:beam('Flight_Handrail',(x,lowy,lowz+h),(x,highy,highz+h),t,rail)
  for i in range(4):
   a=i/3;post((x,lowy+(highy-lowy)*a,lowz+(highz-lowz)*a))
box('Mid_Landing',(150,447,174),(300,130,12),step,.5)
for x in [1.5,298.5]:beam('Landing_Handrail',(x,382,290),(x,510.5,290),3,rail);post((x,510.5,180))
beam('Landing_Backrail',(1.5,510.5,290),(298.5,510.5,290),3,rail);post((150,510.5,180))
for x in [1.5,298.5]:
 for h,t in [(450,3),(395,2)]:beam('Upper_Opening_Guard',(x,131.5,h),(x,510.5,h),t,rail)
 for y in [131.5,258,384,510.5]:post((x,y,340))
for a,b in [((1.5,510.5,450),(298.5,510.5,450)),((1.5,131.5,450),(165.5,131.5,450))]:
 beam('Upper_Guard',a,b,3,rail);beam('Upper_Midrail',Vector(a)-Vector((0,0,55)),Vector(b)-Vector((0,0,55)),2,rail)
post((150,510.5,340));post((165.5,131.5,340))
# The short upper landing shows the slab connection supplied by the host's floor.
box('Upper_Floor_Landing_Context',(150,65,330),(300,130,20),step,.5)
box('Upper_Floor_Left_Rim_Context',(-13.5,321,330),(33,382,20),step,.4)
box('Upper_Floor_Right_Rim_Context',(313.5,321,330),(33,382,20),step,.4)
box('Upper_Floor_Back_Rim_Context',(150,525.5,330),(360,33,20),step,.4)

def panel(a,b,bottom,top,mat):
 a,b=Vector(a),Vector(b);verts=[(a+Vector((0,0,bottom)))/100,(b+Vector((0,0,bottom)))/100,(b+Vector((0,0,top)))/100,(a+Vector((0,0,top)))/100];mesh=bpy.data.meshes.new('Office_Balustrade');mesh.from_pydata(verts,[],[(0,1,2,3)]);mesh.materials.append(mat);o=bpy.data.objects.new('Office_Balustrade',mesh);bpy.context.collection.objects.link(o)
for flight in range(2):
 lowy,highy=(382,130) if flight else (130,382);lowz=20+flight*160;highz=lowz+160
 for x in ([165.5,298.5] if flight else [1.5,134.5]):panel((x,lowy,lowz),(x,highy,highz),15,100,glass);panel((x,lowy,lowz),(x,highy,highz),-25,2,structure)
for x in [1.5,298.5]:panel((x,382,180),(x,510.5,180),15,100,glass);panel((x,131.5,340),(x,510.5,340),15,100,glass)
panel((1.5,510.5,180),(298.5,510.5,180),15,100,glass)
for a,b in [((1.5,510.5,340),(298.5,510.5,340)),((1.5,131.5,340),(165.5,131.5,340))]:panel(a,b,15,100,glass)

scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.device='CPU';scene.cycles.samples=32;scene.cycles.use_denoising=True
scene.render.resolution_x=512;scene.render.resolution_y=320;scene.render.resolution_percentage=100;scene.render.film_transparent=True
scene.render.image_settings.file_format='PNG';scene.render.image_settings.color_mode='RGBA';scene.view_settings.view_transform='AgX';scene.world.color=(.4,.4,.4)
bpy.context.view_layer.update();models=[o for o in scene.objects if o.type=='MESH'];points=[o.matrix_world@Vector(c) for o in models for c in o.bound_box]
low=Vector(tuple(min(p[i] for p in points) for i in range(3)));high=Vector(tuple(max(p[i] for p in points) for i in range(3)));center=(low+high)/2;radius=(high-low).length
bpy.ops.object.camera_add(location=center+Vector((1.25,-1.7,1.4)).normalized()*radius*2);camera=bpy.context.object;camera.rotation_euler=(center-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.type='ORTHO';scene.camera=camera
bpy.context.view_layer.update();inv=camera.matrix_world.inverted();projected=[inv@p for p in points];w=max(p.x for p in projected)-min(p.x for p in projected);h=max(p.y for p in projected)-min(p.y for p in projected);camera.data.ortho_scale=max(w,h*1.6)*1.12
for offset,power,size in [((1,-2,3),85,4),((-2,-1,1),45,3),((1,2,2),70,3)]:
 bpy.ops.object.light_add(type='AREA',location=center+Vector(offset)*radius*.65);lamp=bpy.context.object;lamp.data.energy=power*radius*radius;lamp.data.shape='DISK';lamp.data.size=size*radius*.5;lamp.rotation_euler=(center-lamp.location).to_track_quat('-Z','Y').to_euler()
output=OUT/'T_Catalog_OfficeStairs.png';scene.render.filepath=str(output);bpy.ops.render.render(write_still=True)
image=bpy.data.images.load(str(output),check_existing=False);alpha=list(image.pixels)[3::4];assert min(alpha)==0 and max(alpha)>.9
report={'passed':True,'image':str(output),'size':list(image.size),'transparent':True,'flight_clear_width_cm':130,'floor_rise_cm':320,'risers':18,'upper_floor_landing_context':True,'geometry_objects':len(models)}
(ROOT/'Saved/Logs/OfficeStairCatalogueRenderQA.json').write_text(json.dumps(report,indent=2));print('OFFICE_STAIR_CATALOGUE_READY '+str(output),flush=True)
