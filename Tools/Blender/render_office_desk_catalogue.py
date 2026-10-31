"""Current original machine cutouts; run in an isolated background Blender process.
Never opens/saves the live scene or changes source FBX/blend models.
"""
import bpy, json
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'SourceAssets/UI/BuildCatalogue'
OUT.mkdir(parents=True,exist_ok=True)
MODELS={'OfficeDesk':'OfficeWorkstation.fbx'}
report={}
for name,relative in MODELS.items():
 bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
 source=ROOT/'SourceAssets/Operations'/relative
 bpy.ops.import_scene.fbx(filepath=str(source))
 scene=bpy.context.scene
 scene.render.engine='CYCLES';scene.cycles.device='CPU';scene.cycles.samples=32;scene.cycles.use_denoising=True
 scene.render.resolution_x=512;scene.render.resolution_y=320;scene.render.resolution_percentage=100
 scene.render.film_transparent=True;scene.render.image_settings.file_format='PNG';scene.render.image_settings.color_mode='RGBA'
 scene.view_settings.view_transform='AgX';scene.world.color=(.4,.4,.4)
 bpy.context.view_layer.update()
 models=[o for o in scene.objects if o.type=='MESH' and not o.hide_render]
 assert models, name+' has no renderable geometry'
 points=[o.matrix_world@Vector(c) for o in models for c in o.bound_box]
 low=Vector(tuple(min(p[i] for p in points) for i in range(3)));high=Vector(tuple(max(p[i] for p in points) for i in range(3)))
 center=(low+high)/2;radius=max((high-low).length,.1)
 bpy.ops.object.camera_add(location=center+Vector((1.25,1.7,1.2)).normalized()*radius*2)
 camera=bpy.context.object;camera.rotation_euler=(center-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.type='ORTHO';scene.camera=camera
 bpy.context.view_layer.update();inverse=camera.matrix_world.inverted();projected=[inverse@p for p in points]
 width=max(p.x for p in projected)-min(p.x for p in projected);height=max(p.y for p in projected)-min(p.y for p in projected)
 camera.data.ortho_scale=max(width,height*1.6)*1.16
 for offset,power,size in [((1,-2,3),85,4),((-2,-1,1),45,3),((1,2,2),70,3)]:
  bpy.ops.object.light_add(type='AREA',location=center+Vector(offset)*radius*.65)
  light=bpy.context.object;light.data.energy=power*radius*radius;light.data.shape='DISK';light.data.size=size*radius*.5
  light.rotation_euler=(center-light.location).to_track_quat('-Z','Y').to_euler()
 output=OUT/('T_Catalog_'+name+'.png');scene.render.filepath=str(output);bpy.ops.render.render(write_still=True)
 image=bpy.data.images.load(str(output),check_existing=False)
 alpha=list(image.pixels)[3::4]
 assert min(alpha)==0 and max(alpha)>.9, name+' must be an opaque model on a transparent canvas'
 report[name]={'source':str(source.relative_to(ROOT)),'image':str(output.relative_to(ROOT)),'size':list(image.size),'transparent':True,'geometry_objects':len(models)}
 print('MACHINE_CATALOGUE_RENDERED '+name,flush=True)
(ROOT/'Saved/Logs/OfficeDeskCatalogueRenderQA.json').write_text(json.dumps({'passed':True,'models':report},indent=2))
