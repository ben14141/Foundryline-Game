try:
 import bpy,math,json
 from pathlib import Path
 from mathutils import Vector
 root=Path(r'C:/Users/curly/Documents/Foundryline Game');src=root/'SourceAssets/StarterHauler';bpy.ops.wm.open_mainfile(filepath=str(src/'StarterFlatbed.blend'))
 for i,row in enumerate(json.loads((src/'manifest.json').read_text())['wheels']):
  before=set(bpy.data.objects);bpy.ops.import_scene.fbx(filepath=str(src/f'Wheel{i}.fbx'))
  for o in set(bpy.data.objects)-before:
   if o.type=='MESH':o.location=Vector(row['centre_cm'])/100
 # Assemble the measured loading configuration in model space for visual fitting.
 before=set(bpy.data.objects);bpy.ops.import_scene.fbx(filepath=str(src/'LoadingSideGate.fbx'))
 for o in set(bpy.data.objects)-before:
  if o.type=='MESH':o.location=(-1.344,1.28,1.129513);o.rotation_euler=(math.pi,0,0)
 def cargo_asset(file,name,position,rotation=0):
  before=set(bpy.data.objects);bpy.ops.import_scene.fbx(filepath=str(root/'SourceAssets/MaterialHandling'/file))
  mat=bpy.data.materials.get(name) or bpy.data.materials.new(name)
  for o in set(bpy.data.objects)-before:
   if o.type=='MESH':o.location=position;o.rotation_euler.z=rotation;o.data.materials.clear();o.data.materials.append(mat)
 for x in [-2.756,-1.232]:cargo_asset('Inventory/Storage/Pallets/Pallet_Wood.fbx','CargoWood',(x,.60,1.13),math.pi/2)
 for i in range(3):cargo_asset('Inventory/Materials/Steel/SteelSheet_60x120.fbx','CargoSteel',(-1.994,.60,1.31+i*.003175),math.pi/2)
 tex=bpy.data.images.load(str(root/'SourceAssets/MegapolySmallTrucks/Textures/Tex_256Gradient.png'))
 print('MATERIALS',len(bpy.data.materials),flush=True)
 for mat in bpy.data.materials:
  mat.use_nodes=True;bs=mat.node_tree.nodes.get('Principled BSDF');bs.inputs['Roughness'].default_value=.5
  if mat.name=='CargoWood':bs.inputs['Base Color'].default_value=(.28,.12,.045,1)
  elif mat.name=='CargoSteel':bs.inputs['Base Color'].default_value=(.23,.27,.30,1);bs.inputs['Metallic'].default_value=.8
  elif 'cabpaint' in mat.name.lower():bs.inputs['Base Color'].default_value=(.86,.88,.9,1)
  elif 'glass' in mat.name.lower():bs.inputs['Base Color'].default_value=(.025,.055,.08,1);bs.inputs['Roughness'].default_value=.15
  else:
   n=mat.node_tree.nodes.new('ShaderNodeTexImage');n.image=tex;mat.node_tree.links.new(n.outputs['Color'],bs.inputs['Base Color'])
 bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.01));m=bpy.data.materials.new('floor');m.diffuse_color=(.18,.18,.18,1);bpy.context.object.data.materials.append(m)
 bpy.ops.object.light_add(type='AREA',location=(-4,-4,8));bpy.context.object.data.energy=1800;bpy.context.object.data.shape='DISK';bpy.context.object.data.size=8
 bpy.ops.object.camera_add(location=(10,13,7));cam=bpy.context.object;cam.rotation_euler=(Vector((0,0,1.3))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=13;bpy.context.scene.camera=cam
 bpy.ops.wm.save_as_mainfile(filepath=str(src/'StarterFlatbed_AssemblyReview.blend'));print('RENDERING',flush=True);s=bpy.context.scene;s.render.engine='CYCLES';s.cycles.samples=24;s.world=bpy.data.worlds.new('ReviewWorld');s.world.color=(.3,.3,.3);s.render.resolution_x=1100;s.render.resolution_y=750;s.render.resolution_percentage=100;s.render.filepath=str(root/'Saved/Screenshots/Reviews/StarterTruck_LoadFitReview.png');bpy.ops.render.render(write_still=True)
except Exception:
 import traceback
 traceback.print_exc()
