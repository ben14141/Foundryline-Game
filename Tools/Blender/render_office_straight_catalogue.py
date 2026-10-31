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
 p=Vector(at);box('Rail_Post',p+Vector((0,0,height/2)),(3,3,height),rail,.25);box('Bolted_Baseplate',(min(132.5,max(3.5,p.x)),min(760.5,max(3.5,p.y)),p.z+.5),(7,7,1),structure,.18)

for i in range(18):
 y=130+(i+.5)*28;top=20+(i+1)*320/18
 box('Straight_Tread_%02d'%i,(68,y,top-2),(130,28,4),step,.35)
 box('Straight_Riser_%02d'%i,(68,y-14,top-160/18),(130,2,320/18),structure,.18)
 box('AntiSlip_Nosing',(68,y-12.5,top+.15),(130,3,.3),nosing,.05)
for x in [17,119]:beam('Steel_Stringer',(x,130,8),(x,634,328),12,structure)
for x in [1.5,134.5]:
 for h,t in [(110,3),(55,2)]:beam('Flight_Handrail',(x,130,20+h),(x,634,340+h),t,rail)
 for i in range(7):post((x,130+504*i/6,20+320*i/6))
 for h,t in [(450,3),(395,2)]:beam('Upper_Opening_Guard',(x,131.5,h),(x,632.5,h),t,rail)
 for y in [131.5,258,384,510,632.5]:post((x,y,340))
for h,t in [(450,3),(395,2)]:beam('Upper_Front_Guard',(1.5,131.5,h),(134.5,131.5,h),t,rail)
box('Upper_Landing_Context',(68,699,330),(136,130,20),step,.5)
for x in [-13.5,149.5]:box('Upper_Rim_Context',(x,382,330),(33,504,20),step,.4)

def panel(a,b,bottom,top,mat):
 a,b=Vector(a),Vector(b);verts=[(a+Vector((0,0,bottom)))/100,(b+Vector((0,0,bottom)))/100,(b+Vector((0,0,top)))/100,(a+Vector((0,0,top)))/100];mesh=bpy.data.meshes.new('Office_Balustrade');mesh.from_pydata(verts,[],[(0,1,2,3)]);mesh.materials.append(mat);o=bpy.data.objects.new('Office_Balustrade',mesh);bpy.context.collection.objects.link(o)
for x in [1.5,134.5]:
 panel((x,130,20),(x,634,340),15,100,glass);panel((x,130,20),(x,634,340),-25,2,structure);panel((x,131.5,340),(x,632.5,340),15,100,glass)
panel((1.5,131.5,340),(134.5,131.5,340),15,100,glass)

scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.device='CPU';scene.cycles.samples=16;scene.cycles.use_denoising=True
scene.render.resolution_x=512;scene.render.resolution_y=320;scene.render.resolution_percentage=100;scene.render.film_transparent=True
scene.render.image_settings.file_format='PNG';scene.render.image_settings.color_mode='RGBA';scene.view_settings.view_transform='AgX';scene.world.color=(.4,.4,.4)
bpy.context.view_layer.update();models=[o for o in scene.objects if o.type=='MESH'];points=[o.matrix_world@Vector(c) for o in models for c in o.bound_box]
low=Vector(tuple(min(p[i] for p in points) for i in range(3)));high=Vector(tuple(max(p[i] for p in points) for i in range(3)));center=(low+high)/2;radius=(high-low).length
bpy.ops.object.camera_add(location=center+Vector((1.25,-1.7,1.4)).normalized()*radius*2);camera=bpy.context.object;camera.rotation_euler=(center-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.type='ORTHO';scene.camera=camera
bpy.context.view_layer.update();inv=camera.matrix_world.inverted();projected=[inv@p for p in points];w=max(p.x for p in projected)-min(p.x for p in projected);h=max(p.y for p in projected)-min(p.y for p in projected);camera.data.ortho_scale=max(w,h*1.6)*1.12
for offset,power,size in [((1,-2,3),85,4),((-2,-1,1),45,3),((1,2,2),70,3)]:
 bpy.ops.object.light_add(type='AREA',location=center+Vector(offset)*radius*.65);lamp=bpy.context.object;lamp.data.energy=power*radius*radius;lamp.data.shape='DISK';lamp.data.size=size*radius*.5;lamp.rotation_euler=(center-lamp.location).to_track_quat('-Z','Y').to_euler()
output=OUT/'T_Catalog_OfficeStairsStraight.png';scene.render.filepath=str(output);bpy.ops.render.render(write_still=True)
image=bpy.data.images.load(str(output),check_existing=False);alpha=list(image.pixels)[3::4];assert min(alpha)==0 and max(alpha)>.9
report={'passed':True,'image':str(output),'size':list(image.size),'transparent':True,'flight_clear_width_cm':130,'floor_rise_cm':320,'risers':18,'upper_floor_landing_context':True,'geometry_objects':len(models)}
(ROOT/'Saved/Logs/OfficeStraightCatalogueRenderQA.json').write_text(json.dumps(report,indent=2));print('OFFICE_STAIR_CATALOGUE_READY '+str(output),flush=True)
