"""Painted forklift rectangle catalogue cutout, no road centreline or background."""
import bpy
from mathutils import Vector
from pathlib import Path
root=Path(__file__).resolve().parents[2]
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
scene=bpy.context.scene
scene.render.engine='CYCLES';scene.cycles.device='CPU';scene.cycles.samples=32;scene.cycles.use_denoising=True
scene.render.resolution_x=512;scene.render.resolution_y=320;scene.render.resolution_percentage=100
scene.render.film_transparent=True;scene.render.image_settings.file_format='PNG';scene.render.image_settings.color_mode='RGBA';scene.view_settings.view_transform='AgX';scene.world.color=(.4,.4,.4)
def material(name,color):
 m=bpy.data.materials.new(name);m.use_nodes=True;bsdf=m.node_tree.nodes.get('Principled BSDF');bsdf.inputs['Base Color'].default_value=(*color,1);bsdf.inputs['Roughness'].default_value=.62;return m
def box(name,location,size,mat):
 bpy.ops.mesh.primitive_cube_add(size=1,location=location);obj=bpy.context.object;obj.name=name;obj.dimensions=size;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);obj.data.materials.append(mat)
concrete=material('Area concrete',(.33,.35,.34));yellow=material('Safety yellow paint',(.97,.64,.035));tint=material('Driving floor',(.42,.43,.36))
box('Concrete section',(0,0,-.035),(8,6,.07),concrete)
box('Connected forklift area',(0,0,.008),(7.4,5.4,.015),tint)
for x in [-3.7,3.7]:box('Side perimeter',(x,0,.022),(.1,5.5,.015),yellow)
for y in [-2.7,2.7]:box('End perimeter',(0,y,.022),(7.5,.1,.015),yellow)
center=Vector((0,0,0));radius=10
bpy.ops.object.camera_add(location=(9,-11,9));camera=bpy.context.object;camera.rotation_euler=(center-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.type='ORTHO';camera.data.ortho_scale=11.5;scene.camera=camera
for offset,power,size in [((1,-2,3),85,4),((-2,-1,1),45,3),((1,2,2),70,3)]:
 bpy.ops.object.light_add(type='AREA',location=center+Vector(offset)*radius*.65);light=bpy.context.object;light.data.energy=power*radius*radius;light.data.shape='DISK';light.data.size=size*radius*.5;light.rotation_euler=(center-light.location).to_track_quat('-Z','Y').to_euler()
scene.render.filepath=str(root/'SourceAssets/UI/BuildCatalogue/T_Catalog_ToolForklift.png');bpy.ops.render.render(write_still=True)
