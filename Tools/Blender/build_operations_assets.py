"""Game-owned operations terminals and starter parts. Run in isolated background Blender."""
import bpy,math,json
from mathutils import Vector
from pathlib import Path
OUT=Path(r'C:/Users/curly/Documents/Foundryline Game/SourceAssets/Operations');OUT.mkdir(parents=True,exist_ok=True)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
bpy.context.scene.unit_settings.system='METRIC';bpy.context.scene.unit_settings.scale_length=1
mats={}
for name,color,metal,rough in [('Graphite',(.035,.045,.055,1),.65,.3),('White',(.85,.83,.78,1),.15,.6),('FoundryOrange',(.85,.20,.035,1),.4,.35),('MachinedSteel',(.38,.43,.48,1),.8,.28),('CarbonSteel',(.18,.22,.25,1),.8,.43),('Rubber',(.008,.012,.016,1),0,.7),('Screen',(.015,.08,.13,1),.1,.25)]:
 m=bpy.data.materials.new('ops_'+name.lower());m.use_nodes=True;p=next(n for n in m.node_tree.nodes if n.type=='BSDF_PRINCIPLED');p.inputs['Base Color'].default_value=color;p.inputs['Metallic'].default_value=metal;p.inputs['Roughness'].default_value=rough
 if name=='Screen':p.inputs['Emission Color'].default_value=(.015,.12,.2,1);p.inputs['Emission Strength'].default_value=.45
 mats[name]=m
assets=[];group=[]
def box(name,at,size,mat='Graphite',bevel=.3,rot=None):
 bpy.ops.mesh.primitive_cube_add(size=1,location=[v/100 for v in at]);o=bpy.context.object;o.name=name;o.dimensions=[v/100 for v in size];bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 if rot:o.rotation_euler=[math.radians(v) for v in rot]
 o.data.materials.append(mats[mat]);
 if mat=='Screen':
  ratio=(size[0]/size[2])/(150/54.7)
  for loop in o.data.loops:
   v=o.data.vertices[loop.vertex_index].co;o.data.uv_layers.active.data[loop.index].uv=(.5-v.x/(size[0]/100),.5+v.z/(size[2]/100)/ratio)
 if bevel:b=o.modifiers.new('Machined edges','BEVEL');b.width=bevel/100;b.segments=2
 group.append(o);return o
def cyl(name,at,r,depth,mat='MachinedSteel',rot=None):
 bpy.ops.mesh.primitive_cylinder_add(vertices=16,radius=r/100,depth=depth/100,location=[v/100 for v in at]);o=bpy.context.object;o.name=name;o.data.materials.append(mats[mat]);
 if rot:o.rotation_euler=[math.radians(v) for v in rot]
 group.append(o);return o
def export(name,domain):
 bpy.ops.object.select_all(action='DESELECT')
 for o in group:o.select_set(True)
 bpy.context.view_layer.objects.active=group[0]
 bpy.ops.object.convert(target='MESH');bpy.ops.object.join();obj=bpy.context.object;obj.name=name;bpy.context.scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
 bpy.ops.export_scene.fbx(filepath=str(OUT/(name+'.fbx')),use_selection=True,object_types={'MESH'},bake_anim=False,add_leaf_bones=False,axis_forward='-Y',axis_up='Z',mesh_smooth_type='FACE')
 assets.append({'name':name,'domain':domain});obj.hide_set(True);group.clear()
# Floor-mounted loading tablet. 1:1 dimensions in centimetres.
box('WeightedBase',(0,0,3),(42,34,6),'Graphite',1);box('BaseOrangeInset',(0,0,6.3),(31,23,.7),'FoundryOrange');box('Pedestal',(0,-3,52),(9,9,93),'MachinedSteel');box('CableChannel',(0,-8,50),(3,2,86),'Rubber');cyl('Swivel',(0,0,98),5,20,rot=(0,90,0));box('MountBracket',(0,-1,102),(20,5,15),'Graphite')
box('TabletShell',(0,0,113),(38,3,25),'Graphite',.8,rot=(25,0,0));box('TabletScreen',(0,1.7,113.5),(33.5,.25,20),'Screen',.15,rot=(25,0,0));box('OrangeLowerBezel',(0,1.8,101.8),(32,.6,1.1),'FoundryOrange')
for x in (-16,16):
 for z in (103,123):cyl('Fastener',(x,1.8,z),.45,.4,rot=(90,0,0))
export('LoadingTablet','UI/Operations')
# Office workstation uses the same three-monitor geometry as the dedicated builder.
exec((Path(__file__).parent/'office_workstation_geometry.py').read_text())
export('OfficeWorkstation','UI/Operations')
# Fabrication stock: actual 1/8 inch steel, with small bolt holes modelled in plates.
def plate(name,w,d,holes=False):
 o=box(name,(0,0,.15875),(w,d,.3175),'CarbonSteel',.08)
 if holes:
  for x in (-w*.34,w*.34):
   for y in (-d*.34,d*.34):
    bpy.ops.mesh.primitive_cylinder_add(vertices=16,radius=.006,depth=.02,location=(x/100,y/100,.0015875));tool=bpy.context.object;mod=o.modifiers.new('Bolt hole','BOOLEAN');mod.operation='DIFFERENCE';mod.object=tool;bpy.context.view_layer.objects.active=o
    bpy.ops.object.modifier_apply(modifier=mod.name);bpy.data.objects.remove(tool,do_unlink=True)
 return o
plate('MountingPlate',20.32,17.78,True);export('MountingPlate','Products/Components/Fabrication')
verts=[(-.1651,-.1651,0),(.1651,-.1651,0),(-.1651,.1651,0)];me=bpy.data.meshes.new('Gusset');me.from_pydata(verts,[],[(0,1,2)]);o=bpy.data.objects.new('Gusset',me);bpy.context.collection.objects.link(o);o.data.materials.append(mats['CarbonSteel']);sol=o.modifiers.new('Steel thickness','SOLIDIFY');sol.thickness=.003175;group.append(o);export('GussetPlate','Products/Components/Fabrication')
plate('ReinforcementPlate',38.1,38.1,True);export('ReinforcementPlate','Products/Components/Fabrication')
box('ChannelWeb',(0,0,.15875),(25.4,38.1,.3175),'CarbonSteel',.08)
for x in (-12.54125,12.54125):box('ChannelFlange',(x,0,3.33375),(.3175,38.1,6.35),'CarbonSteel',.08)
export('SupportChannel','Products/Components/Fabrication')
(OUT/'manifest.json').write_text(json.dumps({'assets':assets},indent=2));bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Foundryline_Operations.blend'))
