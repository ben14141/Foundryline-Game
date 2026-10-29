"""Upgrade only the existing sheet roller, retaining its runtime pivots and asset names."""
import bpy,math,json
from pathlib import Path
from mathutils import Vector,Matrix
source=(Path(__file__).parent/'build_material_handling_assets.py').read_text()
exec(source.split('def import_factory')[0])
OUT=R/'SourceAssets/Production/Equipment/SheetRoller';OUT.mkdir(parents=True,exist_ok=True)
displaymat=mat('SR_CNCDisplay',(.015,.025,.035),.48);bs=next(n for n in displaymat.node_tree.nodes if n.type=='BSDF_PRINCIPLED');tex=displaymat.node_tree.nodes.new('ShaderNodeTexImage');tex.image=bpy.data.images.load(str(OUT/'T_SheetRoller_CNC.png'));tex.image.pack()
for socket in ['Base Color','Emission Color']:displaymat.node_tree.links.new(tex.outputs['Color'],bs.inputs[socket])
bs.inputs['Emission Strength'].default_value=.8
upper_carriers=[];console_parts=[]
def enhance_roller(items):
 mesh=bpy.data.meshes.new('RollerDisplay');mesh.from_pydata([(.905,-1.576,1.3825),(1.395,-1.576,1.3825),(1.395,-1.576,1.6575),(.905,-1.576,1.6575)],[],[(0,1,2,3)]);mesh.materials.append(displaymat);o=bpy.data.objects.new('MonitorDisplay',mesh);scene.collection.objects.link(o);items.append(o)
 for side in [-1,1]:
  for x,z,radius in [(-.22,.92,.06),(.22,.92,.06),(0,1.12,.08)]:items.append(cyl('RollShaftJournal',(x,side*1.835,z),radius,.20,steel))
  for y in [side*1.73,side*2.05]:
   for x in [-.35,.35]:items.append(cyl('FrameFlangeNut',(x,y,.20),.023,.025,steel,rot=(0,0,0),verts=6))
  items.append(box('SafetyRail',( .58,side*.88,.73),(.05,1.70,.045),yellow,bevel=.005))
  items.append(box('SafetyRailPost',(.58,side*1.66,.52),(.045,.045,.40),graphite,bevel=.004))
  for z in [.35,.41,.47,.53,.59,.65,.71]:items.append(box('MotorCoolingFin',(-.34,-2.31,z),(.39,.016,.018),steel,bevel=.002))
 # Drive-end hydraulic service block, capped ports and visible piping.
 items.append(box('HydraulicValveBlock',(-.30,-2.43,.46),(.40,.16,.14),steel,bevel=.008))
 for x in [-.44,-.30,-.16]:
  items.append(cyl('ValvePort',(x,-2.53,.48),.025,.045,dark))
  items.append(cyl('ValveSolenoid',(x,-2.43,.59),.030,.12,graphite,rot=(0,0,0)))
  items.append(cable('HydraulicLine',[(x,-2.54,.48),(x,-2.57,.78),(x,-2.47,1.02),(x,-2.10,1.08)],.009))
 # Pressure gauge, dial marks and needle.
 items.append(cyl('PressureGauge',(.31,-2.43,.98),.080,.055,steel))
 items.append(cyl('PressureDial',(.31,-2.462,.98),.068,.006,white))
 for i in range(11):
  a=math.radians(35+i*25);x=.31+math.cos(a)*.053;z=.98+math.sin(a)*.053
  items.append(box('GaugeTick',(x,-2.467,z),(.014,.004,.003),graphite,rot=(0,-a,0)))
 items.append(box('GaugeNeedle',(.31,-2.473,.996),(.004,.003,.046),red,rot=(0,-.6,0)))
 for y in [-1.65,1.65]:
  items.append(cyl('LocalEmergencyStop',(.48,y,.98),.034,.045,red,rot=(0,math.pi/2,0)))
 items.append(label('RollSerial','SR-3520 / 001',(.435,-1.99,.29),(math.pi/2,0,math.pi/2),.025,white))
snippet=source[source.index('# Longer plate roller:'):source.index('# Export each mesh around')]
snippet=snippet.replace("(x,0,z),.12,3.52,steel)","(x,0,z),.174 if name=='Upper' else .12,3.52,steel)")
snippet=snippet.replace(" box('MonitorDisplay',(1.15,-1.568,1.52),(.49,.008,.275),glass,bevel=.004),",'')
snippet=snippet.replace("for index,length in enumerate([.35,.27,.39,.18]):\n roller.append(box('DisplayReadout',(1.13,-1.575,1.61-index*.057),(length,.006,.017),white))",'')
def prepare_roller(items):
 enhance_roller(items)
 console_prefix=('Computer','Monitor','Keyboard','Mouse','DisplayReadout')
 for o in list(items):
  if o.name.startswith(console_prefix):
   items.remove(o);console_parts.append(o)
   if o.name.startswith('ComputerControlCable'):
    # Extend the operator-side cable while preserving its machine-side endpoint.
    for v in o.data.vertices:
     p=o.matrix_world@v.co;weight=max(0,min(1,(p.x-.55)/.50));v.co.y-=1.20*weight
   else:o.location.y-=1.20
  elif o.name.startswith(('BearingHousing','BearingCap','BearingBolt','GreaseFitting','RollShaftJournal')):
   x,z=o.location.x,o.location.z
   if (x*x+(z-1.12)**2)<min((x-.22)**2+(z-.92)**2,(x+.22)**2+(z-.92)**2):items.remove(o);upper_carriers.append(o)
 for side in [-1,1]:
  for x in [-.12,.12]:items.append(box('UpperCarrierGuide',(x,side*1.91,1.24),(.035,.12,.46),steel,bevel=.003))
  items.append(box('CarrierHydraulicCrosshead',(0,side*1.91,1.50),(.39,.28,.08),graphite,bevel=.009))
  upper_carriers.append(cyl('CarrierChromeRod',(0,side*1.91,1.30),.035,.26,steel,rot=(0,0,0)))
snippet=snippet.replace("base=join('SheetRoller_Body',roller)","prepare_roller(roller)\nbase=join('SheetRoller_Body',roller)")
exec(snippet)
console=join('SheetRoller_OperatorConsole',console_parts);register(console,'Production/Equipment/SheetRoller','operator_console')
carriers=join('SheetRoller_UpperCarriers',upper_carriers,(0,0,1.12));bpy.context.view_layer.objects.active=carriers;bpy.ops.object.transform_apply(location=False,rotation=True,scale=True);register(carriers,'Production/Equipment/SheetRoller','upper_bearing_carriers')
for name in ['LowerFront','LowerRear','Upper']:
 o=bpy.data.objects['SheetRoller_'+name];pivot=o.location.copy();pieces=[o];radius=.174 if name=='Upper' else .12
 for side in [-1,1]:
  pieces.append(cyl('RotatingRollHub',(pivot.x,side*1.77,pivot.z),radius*.65,.075,steel))
  for j in range(4):
   a=j*math.pi/2+.4;pieces.append(cyl('RollHubDriveBolt',(pivot.x+math.cos(a)*radius*.43,side*1.811,pivot.z+math.sin(a)*radius*.43),.010,.012,dark,verts=6))
 o.name='PreviousRoll';replacement=join('SheetRoller_'+name,pieces,pivot)

# Registration projects generic UVs: restore the monitor's authored screen UVs.
for p in console.data.polygons:
 if console.data.materials[p.material_index]==displaymat:
  for loop in p.loop_indices:
   v=console.matrix_world@console.data.vertices[console.data.loops[loop].vertex_index].co;console.data.uv_layers.active.data[loop].uv=((v.x-.905)/.49,(v.z-1.3825)/.275)
copies=[]
for item in [base,console,carriers]+[bpy.data.objects['SheetRoller_'+n] for n in ['LowerFront','LowerRear','Upper']]:
 c=item.copy();c.data=item.data.copy();scene.collection.objects.link(c);copies.append(c)

for copy in copies:
 if 'Upper' in copy.name:copy.location.z+=.154
complete=join('SheetRoller_Complete',copies);complete.hide_render=True
register(complete,'Production/Equipment/SheetRoller','roller_complete')
# register projects UVs, so copy back the screen mapping on the complete preview mesh.
for p in complete.data.polygons:
 if complete.data.materials[p.material_index]==displaymat:
  for loop in p.loop_indices:
   v=complete.matrix_world@complete.data.vertices[complete.data.loops[loop].vertex_index].co;complete.data.uv_layers.active.data[loop].uv=((v.x-.905)/.49,(v.z-1.3825)/.275)
entries=[]
for a in manifest:
 o=bpy.data.objects[a['name']];c=o.copy();c.data=o.data.copy();scene.collection.objects.link(c);c.animation_data_clear();c.parent=None;c.matrix_world=Matrix.Identity(4)
 bpy.ops.object.select_all(action='DESELECT');c.select_set(True);bpy.context.view_layer.objects.active=c;bpy.ops.export_scene.fbx(filepath=str(OUT/(o.name+'.fbx')),use_selection=True,object_types={'MESH'},bake_anim=False,add_leaf_bones=False,axis_forward='-Y',axis_up='Z',mesh_smooth_type='FACE');bpy.data.objects.remove(c,do_unlink=True)
 entries.append({'name':o.name,'position_cm':a['position_cm'],'dimensions_cm':list(o.dimensions*100),'domain':a['folder']})
bpy.data.objects['SteelSheet_RolledBlank'].hide_render=True
for name in ['SheetRoller_LowerFront','SheetRoller_LowerRear','SheetRoller_Upper']:
 o=bpy.data.objects[name]
 for f,angle in [(1,0),(240,math.pi*8)]:o.rotation_euler.y=angle;o.keyframe_insert('rotation_euler',frame=f)
for name in ['SheetRoller_Upper','SheetRoller_UpperCarriers']:
 o=bpy.data.objects[name]
 for frame,z in [(1,1.274),(50,1.274),(85,1.214),(190,1.214),(230,1.274),(240,1.274)]:o.location.z=z;o.keyframe_insert('location',frame=frame)
scene.frame_set(1)
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.006));bpy.context.object.data.materials.append(mat('ReviewFloor',(.085,.105,.13),.85))
bpy.ops.object.camera_add(location=(5,-6,3.8));camera=bpy.context.object;camera.data.type='ORTHO';scene.camera=camera
for loc,power,size in [((4,-4,7),1700,5),((-4,0,5),1400,4),((1,5,6),1200,4)]:
 bpy.ops.object.light_add(type='AREA',location=loc);light=bpy.context.object;light.data.energy=power;light.data.size=size;light.rotation_euler=(Vector((0,0,.8))-light.location).to_track_quat('-Z','Y').to_euler()
scene.world=bpy.data.worlds.new('RollerReviewWorld');scene.world.color=(.22,.22,.22);scene.render.engine='CYCLES';scene.cycles.samples=40;scene.render.resolution_x=1600;scene.render.resolution_y=1200;scene.render.resolution_percentage=100
def view(loc,target,scale,path):
 camera.location=loc;camera.rotation_euler=(Vector(target)-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.ortho_scale=scale;scene.render.filepath=str(OUT/path);bpy.ops.render.render(write_still=True)
view((5,-7,4.2),(.25,-.60,.78),6.7,'SheetRoller_Overview.png')
if False:view((1.30,-4.2,1.72),(1.15,-2.77,1.52),.82,'SheetRoller_CNCDetail.png')
view((2,-4,2.2),(-.05,-2.18,.85),1.6,'SheetRoller_DriveDetail.png')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Foundryline_SheetRoller.blend'))
(OUT/'manifest.json').write_text(json.dumps({'shaft_length_cm':352,'assets':entries,'status':'Independent upper bearing carriers follow pressure roll; reachable separate operator console; actual rotating drive hubs and service detail'},indent=2))
print('SHEET_ROLLER_DETAIL_COMPLETE',len(entries))
