"""Original office furnishing kits. Isolated background Blender; cm authored at 1:1."""
import bpy,math,json
from pathlib import Path
from mathutils import Vector
R=Path(__file__).resolve().parents[2];OUT=R/'SourceAssets/Office/Decor';OUT.mkdir(parents=True,exist_ok=True)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
bpy.context.scene.unit_settings.system='METRIC';bpy.context.scene.unit_settings.scale_length=1

def mat(name,c,metal=0,rough=.5):
 m=bpy.data.materials.new('Office_'+name);m.diffuse_color=(*c,1);m.use_nodes=True;p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*c,1);p.inputs['Metallic'].default_value=metal;p.inputs['Roughness'].default_value=rough;return m
wood=mat('Walnut',(.19,.085,.035),0,.38);oak=mat('Oak',(.45,.25,.105),0,.48);dark=mat('Graphite',(.025,.032,.045),.35,.32);white=mat('Paint',(.78,.79,.77),0,.6);brass=mat('Brass',(.56,.36,.10),.8,.25);steel=mat('Steel',(.43,.47,.52),.85,.25);blue=mat('Blueprint',(.025,.13,.27),0,.65);ink=mat('PlanInk',(.83,.92,.96),0,.7);green=mat('Leaf',(.05,.20,.09),0,.65);paper=mat('Paper',(.86,.85,.78),0,.85);fabric=mat('Upholstery',(.045,.065,.10),0,.85);orange=mat('Orange',(.84,.22,.045),.15,.4)
assets={};group=[]
def box(name,p,s,m,b=.25):
 bpy.ops.mesh.primitive_cube_add(size=1,location=Vector(p)/100);o=bpy.context.object;o.name=name;o.dimensions=Vector(s)/100;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(m)
 if b:mod=o.modifiers.new('Rounded edges','BEVEL');mod.width=b/100;mod.segments=3;o.modifiers.new('Normals','WEIGHTED_NORMAL')
 group.append(o);return o

def cyl(name,p,r,d,m,axis=(0,0,1),vertices=20):
 bpy.ops.mesh.primitive_cylinder_add(vertices=vertices,radius=r/100,depth=d/100,location=Vector(p)/100);o=bpy.context.object;o.name=name;o.rotation_euler=Vector(axis).to_track_quat('Z','Y').to_euler();o.data.materials.append(m);mod=o.modifiers.new('Edge rounds','BEVEL');mod.width=.001;mod.segments=2;o.modifiers.new('Normals','WEIGHTED_NORMAL');group.append(o);return o

def beam(name,a,b,r,m):a,b=Vector(a),Vector(b);return cyl(name,(a+b)/2,r,(b-a).length,m,b-a,12)
def text(label,p,size,m,normal=(0,1,0)):
 bpy.ops.object.text_add(location=Vector(p)/100);o=bpy.context.object;o.name='Mounted_'+label;o.data.body=label;o.data.align_x='CENTER';o.data.size=size/100;o.data.extrude=.00015;o.rotation_euler=Vector(normal).to_track_quat('Z','Y').to_euler();o.data.materials.append(m);group.append(o);return o

def line(points,m=ink,r=.10):
 for a,b in zip(points,points[1:]):beam('Blueprint_line',a,b,r,m)
def blueprint(cx,y,z,w,h):
 box('Technical_plan',(cx,y,z+h/2),(w,.35,h),blue,.03)
 def pt(x,t):return (cx+(x-.5)*w,y+.25,z+t*h)
 for x1,z1,x2,z2 in [(.06,.08,.94,.92),(.12,.3,.55,.69),(.61,.3,.87,.67),(.12,.2,.87,.23)]:line([pt(x1,z1),pt(x2,z1),pt(x2,z2),pt(x1,z2),pt(x1,z1)])
 for x in [.22,.44,.67,.80]:
  points=[pt(x+.025*math.cos(a*math.tau/20),.47+.04*math.sin(a*math.tau/20)) for a in range(21)];line(points)
 for t in [.34,.42,.5,.58,.66]:line([pt(.16,t),pt(.51,t)],ink,.055)
 for x in [.12,.55,.87]:line([pt(x,.17),pt(x,.26)],ink,.07)
 text('GENERAL ARRANGEMENT',pt(.5,.82),h*.045,ink);text('FAB / ASSY A-01    REV A',pt(.5,.12),h*.035,ink)

def start(name):
 global group;group=[];assets[name]=group

def cabinet(executive):
 m=wood if executive else dark;w=160 if executive else 140;d=45 if executive else 55
 box('Cabinet_body',(0,0,49),(w,d,78),m,1);box('Top',(0,0,89),(w+1,d+1,3),wood if executive else oak,.6)
 for x in [-w/2+8,w/2-8]:
  for y in [-d/2+8,d/2-8]:cyl('Foot',(x,y,6),2.5,12,dark)
 for i in range(3 if executive else 5):
  h=22 if executive else 13;y=d/2+.5;z=19+i*(24 if executive else 14)
  for side in [-1,1] if executive else [0]:
   width=w/2-4 if executive else w-6;x=side*w/4
   box('Drawer',(x,y,z),(width,2,h),m,.35);beam('Pull',(x-13,y+2,z+2),(x+13,y+2,z+2),.6,brass if executive else steel)
   if not executive:box('Label_holder',(x+45,y+1.4,z),(17,.4,4),steel,.15);text('A-0'+str(i+1),(x+45,y+1.65,z-1),1.8,paper)
 if executive:
  for i in range(7):box('Executive_books',(-57+i*5,-4,101),(4,22,20),[dark,blue,wood][i%3],.2)
  box('Award_plinth',(40,0,93),(24,18,4),dark,.3);cyl('Award_column',(40,0,106),1.5,25,brass);cyl('Achievement_medal',(40,0,120),9,2,brass,(0,1,0));text('FL',(40,1.1,117),5,dark)
 else:
  for i in range(3):cyl('Rolled_plan',(-28+i*20,0,94),3,42,paper,(0,1,0));cyl('Plan_band',(-28+i*20,0,94),3.1,2,orange,(0,1,0))

start('ExecutiveCredenza');cabinet(True)
start('EngineeringCabinet');cabinet(False)
start('OfficePlant');cyl('Ceramic_pot',(0,0,18),17,36,white,vertices=32);cyl('Soil',(0,0,36),15,1,wood)
for i in range(18):
 a=i*2.4;top=(math.cos(a)*22,math.sin(a)*22,65+(i%6)*10);beam('Stem',(0,0,35),top,.4,green);bpy.ops.mesh.primitive_uv_sphere_add(segments=10,ring_count=6,radius=1,location=Vector(top)/100);o=bpy.context.object;o.name='Office_leaf';o.scale=(.055,.19,.025);o.rotation_euler=(.2,(-.5+i%3*.3),a);o.data.materials.append(green);group.append(o)
start('GuestChair')
for x in [-24,24]:
 for y in [-24,24]:beam('Chair_leg',(x,y,3),(x*.8,y*.9,44),1.5,dark)
box('Seat',(0,0,47),(56,56,8),fabric,3);box('Back',(-26,0,72),(8,57,43),fabric,3)
for y in [-30,30]:beam('Arm_support',(-18,y,45),(-18,y,66),1.4,dark);box('Armrest',(-1,y,67),(48,6,4),wood,1.2)
start('OfficeRug');box('Woven_rug',(0,0,.3),(190,170,.6),fabric,.15)
for y in [-79,79]:box('Rug_border',(0,y,.62),(181,2,.06),orange,.02)
for x in [-89,89]:box('Rug_border',(x,0,.62),(2,160,.06),orange,.02)
start('PresidentDeskSet');box('Leather_desk_pad',(0,0,.4),(44,25,.8),dark,.8);box('Nameplate_base',(5,7,2),(29,8,3),brass,.3);box('Nameplate',(5,8,5),(29,2,7),dark,.3);text('PRESIDENT',(5,9.1,3.8),2.7,brass);cyl('Pen_cup',(-16,-2,5),3.2,10,wood)
for i in range(4):cyl('Executive_pen',(-17+i*.7,-2,10),.22,15,brass)
box('Notebook',(10,-7,1.6),(19,13,2),wood,.3);box('Notebook_pages',(10,-7,1.7),(18,12,1.3),paper,.1)
start('EngineerDeskSet');box('Drawing_folder',(1,-2,.5),(34,25,1),blue,.3)
for i in range(2):cyl('Blueprint_roll',(-10+i*12,10,3.6),2.8,34,paper,(1,0,0));cyl('Roll_band',(-10+i*12,10,3.6),2.85,1.5,orange,(1,0,0))
box('Caliper_beam',(1,-8,1.4),(24,1.5,.5),steel,.1)
for x in [-10,1]:box('Caliper_jaw',(x,-6.5,2),(1.3,5,2),steel,.1)
cyl('Pencil_cup',(-21,-4,5),3,10,dark)
for i in range(5):cyl('Pencil',(-22+i*.6,-4,10),.22,16,orange);cyl('Pencil_tip',(-22+i*.6,-4,18.3),.12,.6,dark)
start('PresidentArtwork');box('Frame',(0,0,35),(100,4,70),wood,.6);box('Mat',(0,2.1,35),(91,.4,61),paper,.15)
text('FOUNDRYLINE',(0,2.4,47),8,dark);text('OFFICE OF THE PRESIDENT',(0,2.4,33),3.7,dark);text('EXCELLENCE IN MANUFACTURING',(0,2.4,22),2.8,dark)
cyl('Company_medallion',(0,2.8,11),5,1,brass,(0,1,0))
start('BlueprintBoard');box('Framed_plan_board',(0,0,50),(150,4,100),dark,.65);blueprint(0,2.3,4,142,92)
for x in [-65,65]:
 for z in [10,90]:cyl('Plan_pin',(x,3,z),.6,.4,orange,(0,1,0))
start('DraftingTable')
for x in [-48,48]:
 for y in [-28,28]:beam('Draft_leg',(x,y,3),(x,y,90),2,dark)
box('Drawing_top',(0,0,93),(110,70,4),oak,.8);box('Plan_sheet',(0,0,95.2),(89,57,.3),blue,.1)
for x in [-35,35]:box('Plan_view',(x,0,95.5),(22,35,.1),paper,.1)
for y in [-20,20]:box('Drawing_guideline',(0,y,95.55),(80,.15,.05),ink,.01)
start('OfficeBookshelf');box('Cabinet_back',(0,-16,92),(90,3,176),wood,.5)
for x in [-43,43]:box('Bookshelf_side',(x,0,92),(4,35,176),wood,.5)
for z in [7,49,91,133,178]:box('Shelf',(0,0,z),(86,35,3),wood,.4)
for level,z in enumerate([11,53,95,137]):
 for i in range(8):box('Bound_volume',(-32+i*8,0,z+15),(6,24,29),[dark,blue,paper,wood][(i+level)%4],.25)
start('WallClock')
cyl('Clock_housing',(0,0,18),18,4,dark,(0,1,0),64);cyl('Ivory_dial',(0,2.1,18),16.8,.3,paper,(0,1,0),64)
for i in range(60):
 a=i*math.tau/60;r=15 if i%5 else 14
 beam('Dial_tick',(math.sin(a)*r,2.4,18+math.cos(a)*r),(math.sin(a)*16,2.4,18+math.cos(a)*16),.13 if i%5 else .25,dark)
beam('Hour_hand',(0,2.7,18),(-7,2.7,23),.5,dark);beam('Minute_hand',(0,2.9,18),(10,2.9,26),.3,dark);beam('Sweep_hand',(0,3.1,18),(2,3.1,5),.12,orange);cyl('Clock_hub',(0,3.3,18),.8,.5,brass,(0,1,0))
start('FramedCertificate')
box('Certificate_frame',(0,0,39),(62,4,78),wood,.5);box('Certificate_mat',(0,2.1,39),(55,.3,71),paper,.1)
text('FOUNDRYLINE',(0,2.4,65),4.3,dark);text('CRAFTSMANSHIP',(0,2.4,54),3.5,dark);text('AND EXCELLENCE',(0,2.4,48),3.1,dark)
for z,w in [(39,39),(35,43),(31,36),(27,40)]:beam('Certificate_rule',(-w/2,2.4,z),(w/2,2.4,z),.08,brass)
cyl('Embossed_seal',(-14,2.8,15),5,.7,brass,(0,1,0),32);text('APPROVED',(9,2.4,15),2.3,dark);beam('Signature',(-2,2.4,11),(20,2.4,13),.14,dark)
start('IndustrialWallArt')
for panel in range(3):
 x=(panel-1)*61;box('Triptych_frame',(x,0,36),(58,5,72),dark,.6);box('Art_panel',(x,2.6,36),(53,.4,67),blue,.1)
 for i in range(4):
  xx=x-20+i*13;h=10+((panel*3+i*7)%22);box('Factory_silhouette',(xx,2.9,15+h/2),(9,.4,h),brass,.08);box('Warm_windows',(xx,3.2,19),(3,.2,2),orange,.02)
 line([(x-24,3,12),(x+24,3,12)],ink,.15)
 for i in range(3):beam('Architectural_line',(x-24,3,45+i*6),(x+24,3,50+i*6),.14,ink)
 text(['GROW','MAKE','BUILD'][panel],(x,3.2,61),4.8,paper)
start('EngineeringWhiteboard')
box('Aluminium_frame',(0,0,47.5),(160,6,95),steel,.7);box('Dry_erase_surface',(0,3.1,47.5),(154,.4,89),white,.3)
text('DESIGN / REVIEW',(0,3.5,82),5,dark)
for x,z,w,h in [(-42,42,42,24),(28,42,44,24)]:
 line([(x-w/2,3.6,z-h/2),(x+w/2,3.6,z-h/2),(x+w/2,3.6,z+h/2),(x-w/2,3.6,z+h/2),(x-w/2,3.6,z-h/2)],blue,.18)
 line([(x-w/2-4,3.6,z-h/2-6),(x+w/2+4,3.6,z-h/2-6)],dark,.12)
text('CUT > FORM > WELD',(0,3.6,15),4,blue);text('REV B   /   CHECK FIT',(0,3.6,65),3.5,orange)
line([(-58,3.8,32),(-58,3.8,50),(-50,3.8,50),(-50,3.8,38),(-27,3.8,38),(-27,3.8,32),(-58,3.8,32)],dark,.22)
for x in [16,40]:
 line([(x+3*math.cos(a*math.tau/24),3.8,42+3*math.sin(a*math.tau/24)) for a in range(25)],blue,.18)
line([(9,3.8,42),(47,3.8,42)],orange,.1);text('A-02',(28,3.8,57),2.8,dark)
box('Marker_tray',(0,7,3),(90,12,3),steel,.4)
for i,m in enumerate([dark,blue,orange]):cyl('Capped_marker',(-25+i*15,8,5),.65,11,m,(1,0,0))
box('Board_eraser',(23,8,6),(12,6,3),dark,.5)
start('OfficeNoticeboard')
box('Notice_frame',(0,0,37.5),(100,5,75),oak,.6);box('Cork_inset',(0,2.6,37.5),(94,.4,69),wood,.2)
for i,(x,z) in enumerate([(-28,45),(2,45),(30,45),(-23,17),(19,17)]):
 box('Pinned_sheet',(x,3, z+7),(23,.3,22),paper if i%2 else white,.1);cyl('Push_pin',(x,3.5,z+16),.7,.7,orange,(0,1,0))
 for j in range(4):beam('Note_rule',(x-8,3.4,z+10-j*3),(x+7-(j%2)*3,3.4,z+10-j*3),.08,blue)
text('PROJECTS / THIS WEEK',(0,3.4,65),3.3,paper)
start('AssignedOfficeGlassDoor')
glass=mat('Glass',(.70,.83,.88),0,.12);glass.node_tree.nodes.get('Principled BSDF').inputs['Transmission Weight'].default_value=.95
frost=mat('FrostedGlass',(.77,.85,.88),0,.65);frost.node_tree.nodes.get('Principled BSDF').inputs['Transmission Weight'].default_value=.55
box('Clear_office_glazing',(45.72,0,106.68),(83.44,1.2,205.36),glass,.15)
for x in [2,89.44]:box('Narrow_stile',(x,0,106.68),(4,4.5,213.36),dark,.35)
for z in [2,211.36]:box('Door_rail',(45.72,0,z),(91.44,4.5,4),dark,.35)
box('Engraving_frosted_band',(45.72,0,144),(83.3,1.24,31),frost,.1)
for side in [-1,1]:
 box('Handle_rose',(83,side*2.5,104),(5,.8,16),steel,.4)
 beam('Lever',(82,side*5,108),(70,side*5,108),.85,steel)
 beam('Handle_neck',(82,side*2.8,108),(82,side*5,108),.85,steel)
 cyl('Key_escutcheon',(83,side*3,99),1.4,.7,steel,(0,1,0))
for z in [28,106,184]:cyl('Glass_door_hinge',(.8,0,z),1.15,10,steel)
# The office door shares the existing 36 x 84 inch leaf and hinge-at-origin convention.
# Apply dimensions and keep a separate original native mesh for each furnishing.
manifest=[]
for name,objects in assets.items():
 bpy.ops.object.select_all(action='DESELECT')
 for o in objects:
  o.select_set(True)
  if o.type=='FONT':bpy.context.view_layer.objects.active=o;bpy.ops.object.convert(target='MESH')
 bpy.context.view_layer.objects.active=next(o for o in bpy.context.selected_objects if o.type=='MESH')
 for o in list(bpy.context.selected_objects):
  bpy.context.view_layer.objects.active=o
  if o.type=='MESH':
   for mod in list(o.modifiers):
    try:bpy.ops.object.modifier_apply(modifier=mod.name)
    except:pass
 bpy.ops.export_scene.fbx(filepath=str(OUT/('SM_'+name+'.fbx')),use_selection=True,object_types={'MESH'},add_leaf_bones=False,bake_anim=False,apply_unit_scale=True,axis_forward='-Z',axis_up='Y')
 manifest.append({'name':'SM_'+name,'original':True,'units':'cm','materials':list({m.name for o in bpy.context.selected_objects if o.type=='MESH' for m in o.data.materials})})
# The source scene stores all independent parts; no vendor content is loaded.
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Foundryline_OfficeDecor.blend'));(OUT/'manifest.json').write_text(json.dumps({'assets':manifest,'original':True,'unit_scale':1},indent=2));print('OFFICE_DECOR_MODELS_READY',flush=True)
