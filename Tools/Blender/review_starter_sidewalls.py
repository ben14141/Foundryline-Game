"""Render and inspect both actual exported side-panel hinge positions."""
from pathlib import Path
import bpy,json,math
from mathutils import Vector
r=Path(__file__).resolve().parents[2];o=r/'SourceAssets/StarterHauler';j=json.loads((o/'manifest.json').read_text())
bpy.ops.wm.open_mainfile(filepath=str(o/'StarterFlatbed.blend'));s=bpy.context.scene
for i,row in enumerate(j['wheels']):
 before=set(bpy.data.objects);bpy.ops.import_scene.fbx(filepath=str(o/('Wheel'+str(i)+'.fbx')))
 for a in set(bpy.data.objects)-before:
  if a.type=='MESH':a.location=Vector(row['centre_cm'])/100
for m in bpy.data.materials:
 m.use_nodes=True;p=m.node_tree.nodes.get('Principled BSDF')
 if m.name.startswith('TruckPalette'):
  t=m.node_tree.nodes.new('ShaderNodeTexImage');t.image=bpy.data.images.load(str(r/'SourceAssets/MegapolySmallTrucks/Textures/Tex_256Gradient.png'));t.interpolation='Closest';m.node_tree.links.new(t.outputs['Color'],p.inputs['Base Color']);p.inputs['Roughness'].default_value=.5
 elif m.name.startswith('TruckCabPaint'):p.inputs['Base Color'].default_value=(.82,.84,.86,1);p.inputs['Metallic'].default_value=.25;p.inputs['Roughness'].default_value=.38
 elif m.name.startswith('TruckGlass'):p.inputs['Base Color'].default_value=(.025,.055,.08,1);p.inputs['Metallic'].default_value=.35;p.inputs['Roughness'].default_value=.15
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.006));floor=bpy.context.object;fm=bpy.data.materials.new('ReviewFloor');fm.use_nodes=True;fm.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value=(.07,.085,.1,1);fm.node_tree.nodes['Principled BSDF'].inputs['Roughness'].default_value=.85;floor.data.materials.append(fm)
for p,power,size in [((3,-4,9),2300,7),((-6,1,6),1700,5),((1,5,8),1800,6)]:
 bpy.ops.object.light_add(type='AREA',location=p);a=bpy.context.object;a.data.energy=power;a.data.shape='DISK';a.data.size=size;a.rotation_euler=(Vector((0,0,1.2))-a.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add(location=(5,-12,8));c=bpy.context.object;c.data.type='ORTHO';c.data.ortho_scale=14.5;c.rotation_euler=(Vector((-.3,0,1.2))-c.location).to_track_quat('-Z','Y').to_euler();s.camera=c
s.render.engine='CYCLES';s.cycles.samples=24;s.render.resolution_x=1500;s.render.resolution_y=950;s.render.resolution_percentage=100;s.world=bpy.data.worlds.new('StarterHaulerReviewWorld');s.world.color=(.15,.15,.15)
report={'checks':{},'gates':[]}
for name,sign in [('LoadingSideGate',1),('LoadingSideGateLeft',-1)]:
 a=bpy.data.objects[name];s.frame_set(1);closed=[a.matrix_world@v.co for v in a.data.vertices];s.frame_set(30);half=[a.matrix_world@v.co for v in a.data.vertices];s.frame_set(60);opened=[a.matrix_world@v.co for v in a.data.vertices]
 report['gates'].append({'name':name,'vertices':len(a.data.vertices),'faces':len(a.data.polygons),'closed_z_min_cm':min(v.z for v in closed)*100,'closed_z_max_cm':max(v.z for v in closed)*100,'open_z_min_cm':min(v.z for v in opened)*100,'open_z_max_cm':max(v.z for v in opened)*100})
 report['checks'][name+'_hinge_height_preserved']=abs(a.location.z-1.129513)<.00001
 report['checks'][name+'_panel_hangs_below_deck_when_open']=max(v.z for v in opened)<1.14 and min(v.z for v in opened)>.40
 report['checks'][name+'_opens_outward_not_through_cargo']=min(sign*v.y for v in half)>1.25 and max(sign*v.y for v in half)>1.8
for frame,name in [(1,'StarterFlatbed_SidewallsClosed'),(30,'StarterFlatbed_SidewallsOpening'),(60,'StarterFlatbed_SidewallsDown')]:
 s.frame_set(frame);s.render.filepath=str(o/(name+'.png'));bpy.ops.render.render(write_still=True)
s.frame_set(1);bpy.ops.wm.save_as_mainfile(filepath=str(o/'StarterFlatbed_AssemblyReview.blend'))
(o/'SidewallHingeQA.json').write_text(json.dumps(report,indent=2));assert all(report['checks'].values()),report
print('STARTER_SIDEWALL_REVIEW_READY')
