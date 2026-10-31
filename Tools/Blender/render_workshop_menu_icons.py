"""Original transparent menu illustrations, rendered in an independent Blender process."""
from pathlib import Path
import math
r=Path(r'C:/Users/curly/Documents/Foundryline Game');ns={};exec(compile((r/'Tools/Blender/render_build_catalogue.py').read_text().split('manifest=[]')[0],'catalogue_helpers','exec'),ns)
bpy=ns['bpy'];box=ns['box'];orange=ns['orange'];grey=ns['grey'];white=ns['white'];steel=ns['mat']('Brushed steel',(.43,.46,.48),.8)
def cylinder(name,at,radius,depth,m):
 bpy.ops.mesh.primitive_cylinder_add(vertices=48,radius=radius,depth=depth,location=at);o=bpy.context.object;o.name=name;o.data.materials.append(m);b=o.modifiers.new('Soft edge','BEVEL');b.width=.018;b.segments=3;return o
ns['clear']();box('Clipboard',(0,0,.83),(1.2,.1,1.56),grey);box('Paper',(0,-.061,.81),(1.04,.018,1.28),white);box('Steel clip',(0,-.095,1.5),(.43,.09,.17),steel)
for i in range(4):
 z=1.21-i*.24;box('Order check',(-.36,-.079,z),(.11,.025,.11),orange);box('Order entry',(.08,-.079,z),(.6,.025,.035),grey)
ns['render']('MenuOrders')
ns['clear']();box('Employee badge',(0,.42,.72),(1.38,.09,1.1),grey);box('Badge paper',(0,.362,.68),(1.22,.022,.86),white);box('Badge orange stripe',(0,.345,1.06),(1.22,.018,.12),orange)
for i in range(3):box('Badge writing',(.13,.34,.82-i*.17),(.64,.025,.035),grey)
verts=[];faces=[];rings=12;segments=48
for i in range(rings+1):
 t=(math.pi/2)*i/rings
 for j in range(segments):
  a=2*math.pi*j/segments;verts.append((.67*math.sin(t)*math.cos(a),-.14+.56*math.sin(t)*math.sin(a),.32+.55*math.cos(t)))
for i in range(rings):
 for j in range(segments):a=i*segments+j;b=i*segments+(j+1)%segments;faces.append((a,b,b+segments,a+segments))
m=bpy.data.meshes.new('Helmet dome');m.from_pydata(verts,[],faces);m.update();o=bpy.data.objects.new('Safety helmet',m);bpy.context.scene.collection.objects.link(o);o.data.materials.append(orange)
for p in m.polygons:p.use_smooth=True
brim=cylinder('Helmet brim',(0,-.14,.32),.77,.055,orange);brim.scale.y=.83;box('Crown ridge',(0,-.14,.865),(.1,.78,.065),orange)
ns['render']('MenuTeam')
ns['clear']();box('Workshop cashbox',(0,.12,.38),(1.5,.78,.66),grey);box('Orange lid',(0,.12,.73),(1.55,.83,.08),orange);box('Handle',(0,.12,.83),(.5,.16,.1),steel);box('Latch',(0,-.3,.56),(.15,.07,.19),steel)
for x,count in [(-.65,3),(-.1,5),(.46,2)]:
 for i in range(count):cylinder('Coin',(x,-.55,.035+i*.06),.225,.058,steel)
bpy.ops.object.text_add(location=(.35,-.291,.23),rotation=(math.pi/2,0,0));o=bpy.context.object;o.data.body='$';o.data.size=.32;o.data.extrude=.007;o.data.materials.append(orange);bpy.ops.object.convert(target='MESH')
ns['render']('MenuMoney')
