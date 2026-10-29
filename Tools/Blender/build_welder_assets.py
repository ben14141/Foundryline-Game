"""Author a game-owned MIG welder in a dedicated background Blender scene."""
import bpy, math, json
from pathlib import Path
from mathutils import Vector, Matrix
# Reuse the established material palette and mesh helpers, without running that asset build.
exec((Path(__file__).parent/'build_material_handling_assets.py').read_text().split('def import_factory')[0])
OUT=R/'SourceAssets/Production/Equipment/Welder';OUT.mkdir(parents=True,exist_ok=True)
copper=mat('MH_Copper',(.48,.22,.085),.3,.82)
parts=[]
def add(o):parts.append(o);return o
def tube(name,points,radius,material):
 curve=bpy.data.curves.new(name,'CURVE');curve.dimensions='3D';curve.bevel_depth=radius;curve.bevel_resolution=3
 sp=curve.splines.new('BEZIER');sp.bezier_points.add(len(points)-1)
 for p,co in zip(sp.bezier_points,points):p.co=co;p.handle_left_type=p.handle_right_type='AUTO'
 o=bpy.data.objects.new(name,curve);scene.collection.objects.link(o);o.data.materials.append(material)
 bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o;bpy.ops.object.convert(target='MESH');return bpy.context.object
def rod(name,a,b,r,m):
 a,b=Vector(a),Vector(b);o=cyl(name,(a+b)/2,r,(b-a).length,m,rot=(0,0,0));o.rotation_euler=(b-a).to_track_quat('Z','Y').to_euler();return o
# Compact wheeled trolley; front is +X, all measurements in metres.
add(box('CartDeck',(0,0,.36),(1.05,.62,.08),dark,bevel=.018))
add(box('LowerTray',(.03,0,.19),(.82,.52,.045),graphite,bevel=.012))
for x in [-.40,.39]:
 for y in [-.30,.30]:
  add(cyl('RubberWheel',(x,y,.13),.125,.065,rubber))
  add(cyl('WheelHub',(x,y*1.12,.13),.054,.018,steel))
  add(box('CartLeg',(x,y*.8,.27),(.045,.045,.22),graphite,bevel=.004))
for y in [-.245,.245]:
 add(rod('PushHandleUpright',(-.40,y,.37),(-.52,y,.88),.019,dark))
add(rod('PushHandle',(-.52,-.245,.88),(-.52,.245,.88),.023,rubber))
# Power source, orange side panels, inset graphite front and rear grille.
add(box('PowerSource',(.12,0,.68),(.70,.46,.55),orange,bevel=.03))
add(box('FrontPanel',(.478,0,.70),(.018,.405,.45),graphite,bevel=.012))
add(box('RearPanel',(-.238,0,.68),(.012,.40,.43),dark,bevel=.008))
for y in [-.237,.237]:
 add(box('SideServicePanel',(.10,y,.70),(.56,.014,.36),graphite,bevel=.014))
 for x in [-.14,.34]:
  for z in [.56,.84]:add(cyl('PanelScrew',(x,y*1.04,z),.009,.01,steel,verts=6))
 for i in range(7):add(box('SideVent',(.02+i*.043,y*1.04,.60),(.018,.009,.11),dark,bevel=.003))
add(label('WelderBrand','FOUNDRYLINE',(-.12,-.253,.80),(math.pi/2,0,0),.048,white))
add(label('WelderModel','MIG 250',(-.10,-.254,.71),(math.pi/2,0,0),.047,orange))
# Carry handle and wire-feed compartment with a visible spool behind a small inset.
for x in [-.10,.30]:add(box('HandleMount',(x,0,.98),(.06,.09,.08),dark,bevel=.01))
add(rod('CarryHandle',(-.10,0,1.02),(.30,0,1.02),.024,rubber))
add(cyl('WireSpool',(-.06,.254,.70),.105,.035,copper))
for y in [.24,.277]:add(cyl('SpoolFlange',(-.06,y,.70),.118,.012,dark))
add(cyl('SpoolHub',(-.06,.289,.70),.035,.018,steel))
# Front displays and adjustment knobs face +X.
add(box('DisplayBezel',(.493,0,.83),(.02,.285,.12),dark,bevel=.007))
add(box('DigitalDisplay',(.506,0,.83),(.007,.25,.085),glass,bevel=.004))
add(label('AmpsDisplay','185 A',(.512,-.105,.807),(math.pi/2,0,math.pi/2),.052,white))
for y in [-.12,.12]:
 o=cyl('AdjustmentKnob',(.515,y,.68),.038,.04,rubber,rot=(0,math.pi/2,0));add(o)
 add(box('KnobPointer',(.54,y,.703),(.008,.009,.018),white))
add(cyl('PowerSwitch',(.514,0,.68),.022,.025,red,rot=(0,math.pi/2,0)))
for y in [-.12,.12]:
 add(cyl('LeadSocket',(.51,y,.53),.035,.042,dark,rot=(0,math.pi/2,0)))
 add(cyl('LeadConnector',(.54,y,.53),.023,.035,copper,rot=(0,math.pi/2,0)))
for i in range(8):add(box('FrontCoolingSlot',(.492,-.15+i*.043,.43),(.012,.025,.045),dark))
# Shielding-gas bottle, retaining bands, valve, dual gauges and regulator.
add(cyl('GasCylinder',(-.42,0,.91),.115,1.06,graphite,rot=(0,0,0),verts=32))
add(cyl('GasCylinderFoot',(-.42,0,.40),.127,.06,dark,rot=(0,0,0)))
for z in [.65,1.10]:
 add(cyl('BottleRetainingBand',(-.42,0,z),.12,.025,steel,rot=(0,0,0)))
add(cyl('BottleShoulder',(-.42,0,1.46),.085,.08,graphite,rot=(0,0,0)))
add(cyl('BottleValve',(-.42,0,1.54),.024,.10,copper,rot=(0,0,0)))
add(cyl('ValveHandwheel',(-.42,0,1.60),.058,.025,dark,rot=(0,0,0)))
add(box('GasRegulator',(-.42,-.07,1.53),(.10,.12,.07),copper,bevel=.008))
for x in [-.48,-.36]:
 add(cyl('GaugeRim',(x,-.15,1.57),.043,.025,steel))
 add(cyl('GaugeFace',(x,-.166,1.57),.037,.005,white))
 add(rod('GaugeNeedle',(x,-.171,1.57),(x+.018,-.171,1.59),.0025,graphite))
add(tube('GasHose',[(-.42,-.1,1.5),(-.60,-.13,1.2),(-.57,-.18,.62),(-.26,-.12,.54)],.009,rubber))
add(rod('GunHolsterSupport',(.17,-.23,.86),(.17,-.35,.86),.012,dark))
add(rod('GunHolsterLip',(.17,-.35,.86),(.20,-.35,.90),.012,dark))
body=join('Welder_Machine',parts);uv_planar(body)
# Separate MIG gun with grip pivot for later employee animation.
gunparts=[box('GunGrip',(0,0,0),(.045,.05,.135),graphite,rot=(0,-.18,0),bevel=.009),
 box('GunRubberBack',(-.024,0,-.015),(.017,.049,.10),rubber,rot=(0,-.18,0),bevel=.005),
 box('GunTrigger',(.028,0,.015),(.012,.035,.06),red,bevel=.004),
 cyl('GunBody',(.025,0,.079),.029,.095,orange,rot=(0,math.pi/2,0)),
 tube('CurvedSwanNeck',[(.065,0,.079),(.092,0,.092),(.12,0,.121),(.15,0,.135)],.0105,steel)]
def ring_tube(name,start,end,outer,inner,material):
 vs=[];fs=[];segments=48
 for x in [start,end]:
  for radius in [outer,inner]:
   for i in range(segments):
    a=2*math.pi*i/segments;vs.append((x,radius*math.cos(a),.135+radius*math.sin(a)))
 for i in range(segments):
  j=(i+1)%segments
  fs += [(i,j,96+j,96+i),(48+j,48+i,144+i,144+j),(i,48+i,48+j,j),(96+j,144+j,144+i,96+i)]
 mesh=bpy.data.meshes.new(name);mesh.from_pydata(vs,[],fs);mesh.materials.append(material);o=bpy.data.objects.new(name,mesh);scene.collection.objects.link(o)
 for p in mesh.polygons:p.use_smooth=True
 return o
# Real open nozzle bore, contact tip and exposed wire; tip position remains unchanged.
gunparts += [ring_tube('HollowCopperNozzle',.153,.225,.019,.014,copper),ring_tube('NozzleInnerLiner',.174,.218,.014,.012,dark),
 ring_tube('ContactTip',.205,.234,.005,.0015,copper),rod('WeldingWire',(.229,0,.135),(.239,0,.135),.0011,steel),
 cyl('NeckHexFitting',(.071,0,.083),.021,.018,dark,rot=(0,math.pi/2,0),verts=6),
 cyl('NozzleRetentionCollar',(.149,0,.135),.022,.015,dark,rot=(0,math.pi/2,0)),
 rod('LeadStrainRelief',(0,0,-.06),(-.016,0,-.12),.018,rubber),
 box('TriggerHinge',(.027,0,.049),(.014,.046,.015),steel,bevel=.003),
 tube('TriggerGuard',[(.026,-.023,.053),(.045,-.023,.025),(.039,-.023,-.038),(.008,-.023,-.06)],.0045,dark)]
for x in [.163,.176,.19,.204,.221]:
 bpy.ops.mesh.primitive_torus_add(major_radius=.019,minor_radius=.0012,major_segments=32,minor_segments=8,location=(x,0,.135),rotation=(0,math.pi/2,0))
 o=bpy.context.object;o.name='NozzleMachiningRing';o.data.materials.append(steel if x==.221 else copper);gunparts.append(o)
for side in [-1,1]:
 gunparts.append(box('RubberGripInset',(-.003,side*.027,-.007),(.032,.009,.095),rubber,rot=(0,-.18,0),bevel=.007))
 gunparts.append(box('ShellSeam',(.018,side*.027,.025),(.002,.005,.078),orange,rot=(0,-.18,0),bevel=.001))
 for z in [-.046,.035]:gunparts.append(cyl('GunShellScrew',(-.002,side*.034,z),.0035,.003,steel,verts=8))
 for z in [-.035,-.021,-.007,.007,.021]:
  gunparts.append(box('GripTractionRib',(-.003,side*.034,z),(.032,.003,.003),graphite,rot=(0,-.18,0),bevel=.001))
for z in [-.075,-.085,-.095,-.105]:gunparts.append(cyl('StrainReliefRib',(-.005,0,z),.020,.004,rubber,rot=(0,0,0)))
gunparts += [cyl('RearBarrelBand',(-.017,0,.079),.030,.009,dark,rot=(0,math.pi/2,0)),
 label('GunModelMark','FL / MIG',(-.012,-.032,.077),(math.pi/2,0,0),.009,white)]
gun=join('Welder_MIGGun',gunparts);bpy.context.view_layer.objects.active=gun;bpy.ops.object.transform_apply(location=False,rotation=True,scale=True);uv_planar(gun);gun.location=(.17,-.38,.87)
# Gun rests in a holster; hose routes into its grip with a floor loop for reach.
cables=[tube('GunLead',[(.56,-.12,.53),(.78,-.2,.31),(.89,-.50,.04),(.50,-.85,.035),(-.03,-.76,.035),(-.20,-.48,.12),(.15,-.38,.78)],.014,rubber),
 tube('WorkLead',[(.56,.12,.53),(.80,.21,.10),(.67,.48,.035),(.22,.57,.035),(.05,.44,.08)],.011,rubber)]
leads=join('Welder_Leads',cables);uv_planar(leads)
clamp=join('Welder_WorkClamp',[box('ClampLower',(0,0,0),(.15,.028,.025),steel,bevel=.004),box('ClampUpper',(.01,0,.027),(.14,.028,.022),steel,rot=(0,-.14,0),bevel=.004),box('ClampInsulator',(-.035,0,.04),(.065,.034,.028),rubber,bevel=.006),cyl('ClampPivot',(.015,0,.015),.013,.042,copper)])
uv_planar(clamp);clamp.location=(.05,.44,.08)
# Complete display mesh plus separate components for future animation/interaction.
assets=[body,gun,leads,clamp];bpy.context.view_layer.update()
copies=[]
for o in assets:
 c=o.copy();c.data=o.data.copy();scene.collection.objects.link(c);copies.append(c)
complete=join('Welder_Complete',copies);complete.hide_render=True;assets.append(complete)
entries=[]
for o in assets:
 bpy.context.view_layer.update();entries.append({'name':o.name,'position_cm':[round(v*100,4) for v in o.location],'dimensions_cm':[round(v*100,4) for v in o.dimensions]})
 c=o.copy();c.data=o.data.copy();scene.collection.objects.link(c);c.hide_render=False;c.location=(0,0,0)
 bpy.ops.object.select_all(action='DESELECT');c.select_set(True);bpy.context.view_layer.objects.active=c
 bpy.ops.export_scene.fbx(filepath=str(OUT/(o.name+'.fbx')),use_selection=True,object_types={'MESH'},bake_anim=False,add_leaf_bones=False,axis_forward='-Y',axis_up='Z');bpy.data.objects.remove(c,do_unlink=True)
(OUT/'manifest.json').write_text(json.dumps({'units':'centimetres in Unreal; metres in Blender','assets':entries,'status':'Modeled assets only; no welding production gameplay.'},indent=2))
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.003));floor=bpy.context.object;floor.name='ReviewFloor';floor.data.materials.append(mat('ReviewFloor',(.10,.12,.14),.85))
bpy.ops.object.camera_add(location=(2.7,-3.5,2.3));camera=bpy.context.object;camera.rotation_euler=(Vector((.05,-.05,.75))-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.type='ORTHO';camera.data.ortho_scale=2.5;scene.camera=camera
for loc,energy,size in [((2,-3,4),650,4),((-2,1,3),450,3)]:
 bpy.ops.object.light_add(type='AREA',location=loc);l=bpy.context.object;l.data.energy=energy;l.data.size=size;l.rotation_euler=(Vector((0,0,.7))-l.location).to_track_quat('-Z','Y').to_euler()
scene.world=bpy.data.worlds.new('WelderReviewWorld');scene.world.color=(.22,.22,.22);scene.render.engine='CYCLES';scene.cycles.samples=48;scene.render.resolution_x=1440;scene.render.resolution_y=1440;scene.render.resolution_percentage=100
scene.render.filepath=str(OUT/'Welder_Preview.png');bpy.ops.render.render(write_still=True)
oldloc=camera.location.copy();oldrot=camera.rotation_euler.copy();oldscale=camera.data.ortho_scale
for o in [body,leads,clamp]:o.hide_render=True
camera.location=(.55,-.85,1.10);camera.rotation_euler=(Vector((.27,-.38,.93))-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.ortho_scale=.51
scene.render.filepath=str(OUT/'MIGGun_Detail.png');bpy.ops.render.render(write_still=True)
for o in [body,leads,clamp]:o.hide_render=False
camera.location=oldloc;camera.rotation_euler=oldrot;camera.data.ortho_scale=oldscale
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Foundryline_MIGWelder.blend'))
print('WELDER_ASSETS_COMPLETE',len(entries))
