"""True-scale steel fabrication kit; source blanks fit 60 x 120 inch stock."""
import bpy,math,json,bmesh
from pathlib import Path
from mathutils import Vector,Matrix
exec((Path(__file__).parent/'build_material_handling_assets.py').read_text().split('def import_factory')[0])
OUT=R/'SourceAssets/Products/Components/Fabrication';OUT.mkdir(parents=True,exist_ok=True)
T=.003175;parts=[]
def holes(o,centers,axis='Z',radius=.01):
 for co in centers:
  tool=cyl('DrillTool',co,radius,.5,dark,rot=(0,0,0) if axis=='Z' else (0,math.pi/2,0),verts=24)
  bpy.context.view_layer.objects.active=o;mod=o.modifiers.new('Pierced hole','BOOLEAN');mod.object=tool;mod.operation='DIFFERENCE';bpy.ops.object.modifier_apply(modifier=mod.name);bpy.data.objects.remove(tool,do_unlink=True)
 return o
flat=holes(box('BracketBlank',(0,0,T/2),(.25,.35,T),sheetmat,bevel=.0005),[(x,y,0) for x in [-.08,.08] for y in [-.12,0,.12]])
parts.append(flat)
# Continuous quarter-circle bend, rather than two intersecting plates.
ro=.009525;ri=ro-T
profile=[(.125,0),(ro,0)]
profile += [(ro+ro*math.cos(-math.pi/2-i*math.pi/32),ro+ro*math.sin(-math.pi/2-i*math.pi/32)) for i in range(1,17)]
profile += [(0,.125),(T,.125),(T,ro)]
profile += [(ro+ri*math.cos(-math.pi+i*math.pi/32),ro+ri*math.sin(-math.pi+i*math.pi/32)) for i in range(1,17)]
profile += [(.125,T)]
vs=[(x,y,z) for y in [-.175,.175] for x,z in profile];n=len(profile)
faces=[tuple(reversed(range(n))),tuple(range(n,2*n))]+[(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)]
me=bpy.data.meshes.new('RadiusedBracket');me.from_pydata(vs,[],faces);me.materials.append(sheetmat)
bracket=bpy.data.objects.new('BentBracket',me);scene.collection.objects.link(bracket)
holes(bracket,[(.08,y,0) for y in [-.12,0,.12]])
holes(bracket,[(0,y,.08) for y in [-.12,0,.12]],'X');parts.append(bracket)
rail=holes(box('RailBlank',(0,0,T/2),(.30,3.048,T),sheetmat,bevel=.0005),[(x,y,0) for x in [-.10,.10] for y in [-1.35,-.45,.45,1.35]],radius=.011)
parts.append(rail)
channel=[holes(box('RailWeb',(0,0,T/2),(.10,3.048,T),sheetmat,bevel=.0005),[(0,y,0) for y in [-1.35,-.45,.45,1.35]],radius=.011)]
for x in [-.05,.05]:channel.append(box('ChannelFlange',(x,0,.05),(T,3.048,.10),sheetmat,bevel=.0005))
parts.append(join('FrameRail',channel))
parts.append(box('ShellBlank',(0,0,T/2),(1.175*math.pi/3,1.3895375,T),sheetmat,bevel=.0005))
vs=[];faces=[];count=32;radius=1.175
for i in range(count+1):
 a=-math.pi/6+i/count*math.pi/3
 for y in [-.69476875,.69476875]:vs.append((radius*math.sin(a),y,radius*(1-math.cos(a))))
for i in range(count):faces.append((i*2,i*2+1,i*2+3,i*2+2))
me=bpy.data.meshes.new('SixtyDegreeShell');me.from_pydata(vs,[],faces);me.materials.append(sheetmat);shell=bpy.data.objects.new('ShellSection',me);scene.collection.objects.link(shell);bpy.context.view_layer.objects.active=shell;mod=shell.modifiers.new('Real steel thickness','SOLIDIFY');mod.thickness=T;bpy.ops.object.modifier_apply(modifier=mod.name);parts.append(shell)
# Complete dished heads are authored separately by build_dished_heads.py.
# Purchased kits remain visually distinct from fabricated steel inventory.
parts.append(join('RunningGearKit',[box('KitCase',(0,0,.22),(1.05,.9,.44),graphite,bevel=.025),box('CaseLatch',(.53,0,.23),(.02,.15,.07),steel,bevel=.004)]))
parts.append(join('FittingsKit',[box('FittingsCase',(0,0,.15),(.65,.55,.30),orange,bevel=.02),box('CaseLid',(0,0,.31),(.67,.57,.035),dark,bevel=.006)]))
entries=[]
for o in parts:
 bpy.context.view_layer.objects.active=o;bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
 # Put the authored pivot at bottom centre without changing physical dimensions.
 lo=Vector([min(v.co[i] for v in o.data.vertices) for i in range(3)]);hi=Vector([max(v.co[i] for v in o.data.vertices) for i in range(3)])
 shift=Vector(((lo.x+hi.x)/2,(lo.y+hi.y)/2,lo.z))
 for v in o.data.vertices:v.co-=shift
 o.location=(0,0,0);uv_planar(o);bpy.context.view_layer.update()
 entries.append({'name':o.name,'dimensions_cm':list(o.dimensions*100),'material':'MH_CarbonSteel' if o.name not in ['RunningGearKit','FittingsKit'] else 'mixed','sheet_thickness_mm':3.175})
 bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o;bpy.ops.export_scene.fbx(filepath=str(OUT/(o.name+'.fbx')),use_selection=True,object_types={'MESH'},bake_anim=False,axis_forward='-Y',axis_up='Z',mesh_smooth_type='FACE')
for i,o in enumerate(parts):o.location=((i%4)*3.8,(i//4)*4.2,0)
bpy.ops.mesh.primitive_plane_add(size=100,location=(5,2,-.01));bpy.context.object.data.materials.append(mat('ReviewFloor',(.08,.10,.13),.8))
bpy.ops.object.camera_add(location=(14,-13,14));camera=bpy.context.object;camera.rotation_euler=(Vector((5.5,2,.1))-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.type='ORTHO';camera.data.ortho_scale=18;scene.camera=camera
for loc,power,size in [((4,-4,10),2200,7),((-2,6,7),1800,6),((13,8,8),1800,5)]:
 bpy.ops.object.light_add(type='AREA',location=loc);light=bpy.context.object;light.data.energy=power;light.data.size=size;light.rotation_euler=(Vector((5,2,0))-light.location).to_track_quat('-Z','Y').to_euler()
scene.world=bpy.data.worlds.new('FabricationReview');scene.world.color=(.2,.2,.2);scene.render.engine='CYCLES';scene.cycles.samples=40;scene.render.resolution_x=1800;scene.render.resolution_y=1200;scene.render.resolution_percentage=100;scene.render.filepath=str(OUT/'FabricatedParts_Overview.png');bpy.ops.render.render(write_still=True)
# Close views keep the small parts readable on a phone.
for name,indices,scale in [('Brackets_Detail',[0,1],.9),('Rails_Detail',[2,3],4.5)]:
 saved=[o.location.copy() for o in parts]
 for o in parts:o.hide_render=True
 for k,i in enumerate(indices):parts[i].hide_render=False;parts[i].location=((k-.5)*(.37 if name.startswith('Brackets') else .65),0,0)
 target=Vector((0,0,.05));camera.location=target+Vector((1,-1.5,1.5))*(1 if name.startswith('Brackets') else 3)
 camera.rotation_euler=(target-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.ortho_scale=scale
 scene.render.filepath=str(OUT/(name+'.png'));bpy.ops.render.render(write_still=True)
 for o,loc in zip(parts,saved):o.location=loc;o.hide_render=False
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Foundryline_FabricatedParts.blend'));(OUT/'manifest.json').write_text(json.dumps({'units':'metres in Blender, centimetres in Unreal','stock_cm':[152.4,304.8,.3175],'assets':entries,'notes':'60-degree shell sections fit the single sheet format. Complete dished heads are separate assets. Recipe quantities and cut times are prototype balancing data.'},indent=2));print('FABRICATED_PARTS_COMPLETE',len(parts))
