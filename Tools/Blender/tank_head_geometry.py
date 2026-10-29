"""Visual ASME F&D proportions, not a pressure-vessel design calculation."""
import bpy, math

def make_head(name, material, half=False, diameter=2.35, thickness=.003175):
    radius=diameter/2
    crown=diameter
    knuckle=.06*diameter
    flange=.05
    radial_center=radius-knuckle
    crown_z=-math.sqrt((crown-knuckle)**2-radial_center**2)
    tangent=math.atan2(-crown_z,radial_center)
    profile=[(radius,-flange),(radius,0)]
    for i in range(1,17):
        a=tangent*i/16
        profile.append((radial_center+knuckle*math.cos(a),knuckle*math.sin(a)))
    r,z=profile[-1]
    theta=math.atan2(r,z-crown_z)
    for i in range(1,32):
        a=theta*(1-i/32)
        profile.append((crown*math.sin(a),crown_z+crown*math.cos(a)))
    sweep=math.pi if half else 2*math.pi
    count=64 if half else 128
    segments=count+1 if half else count
    vertices=[]
    for r,z in profile:
        for j in range(segments):
            a=sweep*j/count
            vertices.append((r*math.cos(a),r*math.sin(a),z+flange))
    faces=[]
    for i in range(len(profile)-1):
        for j in range(count):
            nxt=j+1 if half else (j+1)%count
            faces.append((i*segments+j,i*segments+nxt,(i+1)*segments+nxt,(i+1)*segments+j))
    pole=len(vertices);vertices.append((0,0,crown_z+crown+flange))
    for j in range(count):
        nxt=j+1 if half else (j+1)%count
        faces.append(((len(profile)-1)*segments+j,(len(profile)-1)*segments+nxt,pole))
    mesh=bpy.data.meshes.new(name);mesh.from_pydata(vertices,[],faces);mesh.materials.append(material);mesh.update()
    obj=bpy.data.objects.new(name,mesh);bpy.context.scene.collection.objects.link(obj)
    bpy.context.view_layer.objects.active=obj
    solid=obj.modifiers.new('Actual steel gauge','SOLIDIFY');solid.thickness=thickness;solid.offset=-1;solid.use_even_offset=True
    bpy.ops.object.modifier_apply(modifier=solid.name)
    for p in mesh.polygons:p.use_smooth=True
    return obj,{'head_style':'ASME flanged and dished visual profile','diameter_cm':diameter*100,'crown_radius_cm':crown*100,'knuckle_radius_cm':knuckle*100,'straight_flange_cm':flange*100,'dish_depth_cm':(crown_z+crown)*100,'thickness_mm':thickness*1000,'segment':'half' if half else 'complete','certification':'Game art only; no ASME certification or pressure rating'}
