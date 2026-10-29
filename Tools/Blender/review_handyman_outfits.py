import bpy,math
from pathlib import Path
from mathutils import Vector
root=Path(r'C:/Users/curly/Documents/Foundryline Game/SourceAssets/Employees/Handyman/Imported')
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
mat=bpy.data.materials.new('VendorPalette');mat.use_nodes=True;tex=mat.node_tree.nodes.new('ShaderNodeTexImage');tex.image=bpy.data.images.load(str(root/'T_Main_colors.png'));tex.interpolation='Closest';shader=mat.node_tree.nodes.get('Principled BSDF');mat.node_tree.links.new(tex.outputs['Color'],shader.inputs['Base Color']);shader.inputs['Roughness'].default_value=.7
for index in range(3):
 for name in [f'SK_Body_{index+1}',f'SK_Head_{index+1}','SK_Boots_1',f'SK_Hat_{index+1}']:
  before=set(bpy.context.scene.objects);bpy.ops.import_scene.fbx(filepath=str(root/(name+'.fbx')),use_anim=False)
  for obj in set(bpy.context.scene.objects)-before:
   if obj.parent is None:obj.location.x+=(index-1)*1.7
   if obj.type=='MESH':obj.data.materials.clear();obj.data.materials.append(mat)
world=bpy.context.scene.world;world.use_nodes=True;world.node_tree.nodes['Background'].inputs[0].default_value=(.45,.45,.45,1);world.node_tree.nodes['Background'].inputs[1].default_value=.6
bpy.ops.object.light_add(type='AREA',location=(0,-4,5));bpy.context.object.data.energy=800;bpy.context.object.data.shape='DISK';bpy.context.object.data.size=6
bpy.ops.object.camera_add(location=(0,-6,2));cam=bpy.context.object;cam.rotation_euler=(Vector((0,0,.9))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=5.5;bpy.context.scene.camera=cam
scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=32;scene.render.resolution_x=1800;scene.render.resolution_y=900;scene.render.resolution_percentage=100;scene.view_settings.view_transform='AgX';scene.render.filepath=str(root.parent/'OutfitReview.png');bpy.ops.wm.save_as_mainfile(filepath=str(root.parent/'Foundryline_Handyman_OutfitReview.blend'));bpy.ops.render.render(write_still=True)
