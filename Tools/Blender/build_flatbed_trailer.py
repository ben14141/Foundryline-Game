# Builds a 53 ft aluminium flatbed semi-trailer (spread tandem, Utility 4000AE style) in the Megapoly
# palette style, hitched to the existing Megapoly tractor, with control drivers and a baked demo action.
# Units: metres, X forward, Z up. Trailer origin = ground point under the kingpin.
# Hard edges of hero parts carry a bevel weight; a Bevel modifier (weight-limited) rounds them.
import bpy, bmesh, math, os
from mathutils import Vector, Matrix

REPO = r"C:\Users\curly\Documents\Foundryline Game"
PALETTE = os.path.join(REPO, r"SourceAssets\MegapolyTrucks\Tex_256Gradient.png")
TRACTOR = os.path.join(REPO, r"SourceAssets\Vehicles\Semi\Megapoly")

# ---------- palette swatches (8x8 cells of 32 px, gradient light->dark top->bottom) ----------
def sw(r, c, t=0.5):
    return ((c * 32 + 16) / 256, (255 - (r * 32 + 4 + t * 24) + 0.5) / 256)
SW = {
    'alu': sw(7, 0, 0.45), 'alu_dark': sw(7, 1, 0.45), 'alu_bright': sw(0, 1, 0.15), 'alu_mid': sw(7, 0, 0.8),
    'wood': sw(3, 6, 0.42), 'wood2': sw(3, 5, 0.55), 'wood3': sw(4, 3, 0.5), 'wood_dark': sw(3, 7, 0.45),
    'rubber': sw(0, 7, 0.75), 'tyre': sw(0, 7, 0.45), 'steel': sw(0, 6, 0.5), 'steel_dark': sw(0, 7, 0.35),
    'galv': sw(0, 4, 0.55), 'red': sw(2, 3, 0.08), 'amber': sw(3, 2, 0.35), 'white': sw(0, 0, 0.2),
    'blue': sw(6, 2, 0.4), 'hub': sw(0, 5, 0.45), 'yellow': sw(2, 5, 0.4), 'chrome': sw(0, 0, 0.6),
}
LIGHT = 1  # material slot for lamp lenses
B = 1.0    # bevel weight

# ---------- dimensions ----------
L_TRAILER = 16.154          # 53 ft
X_F = 0.914                 # front, kingpin 36 in back
X_R = X_F - L_TRAILER       # rear
W2 = 1.295                  # half of 102 in
Z_DECK = 1.47
Z_PLATE = 1.28              # fifth-wheel height of the Megapoly tractor
AXLES = (-10.95, -14.00)    # 10 ft spread
R_WHEEL = 0.47352           # same rolling radius as the tractor
Y_DUAL = 0.98
X_GEAR = -3.6
Y_LEG = 0.58
GEAR_DROP = 0.30
KINGPIN_TO_AXLES = -(AXLES[0] + AXLES[1]) / 2

# beam underside: shallow over the fifth wheel, deep belly, raised over the axles (eased between keys)
BEAM_KEYS = [(X_F, Z_PLATE + 0.02), (-1.0, Z_PLATE + 0.02), (-3.3, 0.98), (-9.5, 0.98), (-10.3, 1.14), (X_R + 0.08, 1.14)]
def zbeam(x):
    for (x0, z0), (x1, z1) in zip(BEAM_KEYS, BEAM_KEYS[1:]):
        if x0 >= x >= x1:
            t = (x0 - x) / (x0 - x1); t = t * t * (3 - 2 * t)
            return z0 + (z1 - z0) * t
    return BEAM_KEYS[-1][1]

class Part:
    def __init__(self):
        self.bm = bmesh.new()
        self.uv = self.bm.loops.layers.uv.new('UVMap')
        self.bw = self.bm.edges.layers.float.new('bevel_weight_edge')
        self.M = Matrix.Identity(4)
    def _paint(self, faces, swatch, mat, bevel):
        for f in faces:
            f.material_index = mat
            for l in f.loops:
                l[self.uv].uv = SW[swatch]
            if bevel:
                for e in f.edges: e[self.bw] = bevel
    def _v(self, p):
        return self.bm.verts.new(self.M @ Vector(p))
    def box(self, c, s, swatch, mat=0, rot=None, bevel=B):
        m = Matrix.Translation(Vector(c))
        if rot: m = m @ rot
        m = self.M @ m @ Matrix.Diagonal((s[0], s[1], s[2], 1))
        r = bmesh.ops.create_cube(self.bm, size=1.0, matrix=m)
        self._paint({f for v in r['verts'] for f in v.link_faces}, swatch, mat, bevel)
    def span(self, a, b, w, h, swatch, mat=0, bevel=B):
        a, b = Vector(a), Vector(b); d = b - a
        rot = d.to_track_quat('X', 'Z').to_matrix().to_4x4()
        self.box((a + b) / 2, (d.length, w, h), swatch, mat, rot, bevel)
    def cyl(self, c, r, depth, axis, swatch, mat=0, segs=16, r2=None):
        rot = {'Z': Matrix.Identity(4), 'Y': Matrix.Rotation(math.radians(90), 4, 'X'),
               'X': Matrix.Rotation(math.radians(90), 4, 'Y')}[axis]
        res = bmesh.ops.create_cone(self.bm, cap_ends=True, cap_tris=False, segments=segs,
                                    radius1=r, radius2=r if r2 is None else r2, depth=depth,
                                    matrix=self.M @ Matrix.Translation(Vector(c)) @ rot)
        self._paint({f for v in res['verts'] for f in v.link_faces}, swatch, mat, 0)
    def tube(self, a, b, r, swatch, segs=10, mat=0):
        a, b = Vector(a), Vector(b); d = b - a
        rot = d.to_track_quat('Z', 'Y').to_matrix().to_4x4()
        res = bmesh.ops.create_cone(self.bm, cap_ends=True, cap_tris=False, segments=segs, radius1=r, radius2=r,
                                    depth=d.length, matrix=self.M @ Matrix.Translation((a + b) / 2) @ rot)
        self._paint({f for v in res['verts'] for f in v.link_faces}, swatch, mat, 0)
    def hexa(self, p8, swatch, mat=0, bevel=B):
        vs = [self._v(p) for p in p8]
        idx = [(0, 3, 2, 1), (4, 5, 6, 7), (0, 1, 5, 4), (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7)]
        self._paint([self.bm.faces.new([vs[j] for j in q]) for q in idx], swatch, mat, bevel)
    def lathe(self, c, loop, swatch, axis='Y', segs=24, mat=0, phase=0.0):
        # closed (radius, t) cross-section revolved about an axis through c
        def P(r, t, a):
            ca, sa = r * math.cos(a), r * math.sin(a)
            return {'Y': (c[0] + ca, c[1] + t, c[2] + sa), 'Z': (c[0] + ca, c[1] + sa, c[2] + t),
                    'X': (c[0] + t, c[1] + ca, c[2] + sa)}[axis]
        rings = [[self._v(P(r, t, phase + k * math.tau / segs)) for r, t in loop] for k in range(segs)]
        n = len(loop); faces = []
        for k in range(segs):
            A, Bn = rings[k], rings[(k + 1) % segs]
            for i in range(n):
                faces.append(self.bm.faces.new((A[i], A[(i + 1) % n], Bn[(i + 1) % n], Bn[i])))
        self._paint(faces, swatch, mat, 0)
    def loft(self, sections, swatch, mat=0, bevel=B):
        # sections: list of closed 3D point loops with equal counts, capped at both ends
        rings = [[self._v(p) for p in s] for s in sections]
        n = len(sections[0]); faces = []
        for A, Bn in zip(rings, rings[1:]):
            for i in range(n):
                faces.append(self.bm.faces.new((A[i], A[(i + 1) % n], Bn[(i + 1) % n], Bn[i])))
        faces.append(self.bm.faces.new(rings[0])); faces.append(self.bm.faces.new(list(reversed(rings[-1]))))
        self._paint(faces, swatch, mat, bevel)
    def extrude_x(self, prof, x0, x1, swatch, mat=0, bevel=B):
        self.loft([[(x0, y, z) for y, z in prof], [(x1, y, z) for y, z in prof]], swatch, mat, bevel)
    def lamp(self, c, axis, sign, r, col, segs=14):
        # rubber grommet ring and a domed lens facing +/- axis
        g = [(r * 0.9, 0), (r * 1.35, 0), (r * 1.35, 0.008), (r * 0.9, 0.008)]
        l = [(0.002, 0.004), (r, 0.004), (r * 0.97, 0.012), (r * 0.75, 0.018), (0.002, 0.021)]
        self.lathe(c, [(a, sign * t) for a, t in g], 'rubber', axis, segs)
        self.lathe(c, [(a, sign * t) for a, t in l], col, axis, segs, LIGHT)
    def to_object(self, name, coll, mats, bevel_width=0.006):
        bm = self.bm
        bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
        for f in bm.faces: f.smooth = True
        for e in bm.edges:
            e.smooth = not (e.is_manifold and e.calc_face_angle(0) > math.radians(38)) and e.is_manifold
        me = bpy.data.meshes.new(name); bm.to_mesh(me); bm.free()
        for m in mats: me.materials.append(m)
        ob = bpy.data.objects.new(name, me); coll.objects.link(ob)
        if bevel_width:
            mod = ob.modifiers.new('EdgeBevel', 'BEVEL'); mod.limit_method = 'WEIGHT'
            mod.width = bevel_width; mod.segments = 2; mod.harden_normals = True
        return ob

def palette_material(name, emission=0.0):
    m = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    m.use_nodes = True; nt = m.node_tree; nt.nodes.clear()
    img = bpy.data.images.load(PALETTE, check_existing=True)
    tex = nt.nodes.new('ShaderNodeTexImage'); tex.image = img; tex.interpolation = 'Closest'
    bsdf = nt.nodes.new('ShaderNodeBsdfPrincipled'); out = nt.nodes.new('ShaderNodeOutputMaterial')
    bsdf.inputs['Roughness'].default_value = 0.42
    nt.links.new(tex.outputs['Color'], bsdf.inputs['Base Color'])
    if emission:
        nt.links.new(tex.outputs['Color'], bsdf.inputs['Emission Color'])
        bsdf.inputs['Emission Strength'].default_value = emission
    nt.links.new(bsdf.outputs['BSDF'], out.inputs['Surface'])
    return m

# ---------- reusable assemblies ----------
def add_wheel(p, s, segs=48, single=False):
    """Dual 11R22.5 set (or a single spare) around the local origin; outer (polished) face toward s*Y."""
    tread = (-0.062, -0.054, -0.024, -0.016, 0.016, 0.024, 0.054, 0.062)   # three circumferential grooves
    loop = [(0.285, -0.14), (0.36, -0.141), (0.415, -0.137), (0.448, -0.126), (0.466, -0.112), (R_WHEEL, -0.10)]
    for a, b in zip(tread[0::2], tread[1::2]):
        loop += [(R_WHEEL, a), (R_WHEEL - 0.012, a + 0.001), (R_WHEEL - 0.012, b - 0.001), (R_WHEEL, b)]
    loop += [(R_WHEEL, 0.10), (0.466, 0.112), (0.448, 0.126), (0.415, 0.137), (0.36, 0.141), (0.285, 0.14)]
    sh = 0.155 if single else 0.0
    keep = p.M
    p.M = keep @ Matrix.Translation((0, -s * sh, 0))
    for off in ((0.155,) if single else (-0.155, 0.155)):
        off = off * (s if single else 1)
        p.lathe((0, off, 0), loop, 'tyre', 'Y', segs)
        for sg in (-1, 1):
            p.lathe((0, off + sg * 0.1395, 0), [(0.37, -0.002), (0.40, -0.002), (0.40, 0.002), (0.37, 0.002)], 'rubber', 'Y', segs)
    lo = 0.0 if single else -0.29
    p.lathe((0, 0, 0), [(0.26, min(lo, s * 0.29)), (0.285, min(lo, s * 0.29)), (0.285, max(-lo, s * 0.29) if not single else max(0, s * 0.29)), (0.26, max(-lo, s * 0.29) if not single else max(0, s * 0.29))], 'alu', 'Y', 32)   # rim barrel
    # polished outer disc: flange lip, dish, hand holes, hub pilot
    disc = [(0.105, 0.262), (0.24, 0.262), (0.278, 0.275), (0.292, 0.29), (0.296, 0.304), (0.287, 0.311),
            (0.268, 0.303), (0.20, 0.294), (0.135, 0.296), (0.105, 0.300)]
    p.lathe((0, 0, 0), [(r, s * t) for r, t in disc], 'alu_bright', 'Y', segs)
    for k in range(10):
        a = k * math.tau / 10 + math.pi / 10
        p.lathe((0.218 * math.cos(a), s * 0.291, 0.218 * math.sin(a)), [(0.012, -0.004), (0.032, -0.004), (0.032, 0.004), (0.012, 0.004)], 'steel_dark', 'Y', 12)
    hub = [(0.03, 0.296), (0.115, 0.296), (0.112, 0.315), (0.085, 0.322), (0.080, 0.345), (0.070, 0.362), (0.045, 0.371), (0.03, 0.373)]
    p.lathe((0, 0, 0), [(r, s * t) for r, t in hub], 'hub', 'Y', 24)
    p.lathe((0, 0, 0), [(0.01, s * 0.372), (0.045, s * 0.372), (0.042, s * 0.378), (0.01, s * 0.38)], 'chrome', 'Y', 16)
    for k in range(10):
        a = k * math.tau / 10
        c = (0.165 * math.cos(a), s * 0.302, 0.165 * math.sin(a))
        p.cyl((c[0], s * 0.300, c[2]), 0.022, 0.006, 'Y', 'steel', segs=12)
        p.cyl((c[0], s * 0.312, c[2]), 0.016, 0.022, 'Y', 'galv', segs=6)
    p.tube((0.24, s * 0.30, -0.05), (0.27, s * 0.335, -0.06), 0.006, 'steel', 6)   # valve stem
    p.M = keep

def add_winch(p, x, s):
    y = s * (W2 - 0.16)
    p.box((x, y, 1.18), (0.20, 0.10, 0.02), 'steel_dark')
    p.box((x, s * (W2 - 0.11), 1.215), (0.20, 0.03, 0.06), 'steel_dark')
    for dx in (-0.07, 0.07):
        p.hexa([(x + dx - 0.01, y - 0.05, 1.06), (x + dx + 0.01, y - 0.05, 1.06), (x + dx + 0.01, y + 0.05, 1.06), (x + dx - 0.01, y + 0.05, 1.06),
                (x + dx - 0.01, y - 0.05, 1.18), (x + dx + 0.01, y - 0.05, 1.18), (x + dx + 0.01, y + 0.05, 1.18), (x + dx - 0.01, y + 0.05, 1.18)], 'steel_dark')
    c = (x, y, 1.105)
    p.lathe(c, [(0.02, -0.06), (0.03, -0.06), (0.03, 0.06), (0.02, 0.06)], 'steel', 'X', 14)
    p.lathe(c, [(0.03, -0.045), (0.05, -0.045), (0.052, 0.0), (0.05, 0.045), (0.03, 0.045)], 'yellow', 'X', 18)   # wound strap
    for t in (-0.055, 0.055):
        p.lathe(c, [(0.02, t - 0.004), (0.068, t - 0.004), (0.068, t + 0.004), (0.02, t + 0.004)], 'steel_dark', 'X', 18)
    p.lathe((x + 0.075, y, 1.105), [(0.01, -0.006), (0.066, -0.006), (0.066, 0.006), (0.01, 0.006)], 'steel', 'X', 20)   # ratchet
    p.cyl((x + 0.10, y, 1.105), 0.022, 0.03, 'X', 'steel', segs=6)
    p.tube((x + 0.085, y + s * 0.045, 1.14), (x + 0.085, y + s * 0.035, 1.075), 0.006, 'steel', 6)   # pawl

def build():
    for c in [c for c in bpy.data.collections if c.name.startswith(('FlatbedTrailer', 'TractorReference'))]:
        for o in list(c.objects): bpy.data.objects.remove(o)
        bpy.data.collections.remove(c)
    for o in list(bpy.data.objects):
        if o.name.startswith(('Trailer_', 'Tractor_', 'FlatbedTrailer')): bpy.data.objects.remove(o)
    scene = bpy.context.scene
    coll = bpy.data.collections.new('FlatbedTrailer_53ft'); scene.collection.children.link(coll)
    mats = [palette_material('Trailer_Palette'), palette_material('Trailer_MarkerLight', 1.5)]
    body = Part()
    XM = (X_F + X_R) / 2

    # ---- deck: extruded aluminium planks (seams, two dark tie-down channels), oak edge boards
    prof = [(-0.915, 1.42), (0.915, 1.42), (0.915, 1.465)]
    seams = sorted({round(k * 0.1525, 4) for k in range(-5, 6)} | {0.36, -0.36}, reverse=True)
    for y in seams:
        if abs(y) > 0.89: continue
        w, d = (0.025, 0.03) if abs(abs(y) - 0.36) < 1e-3 else (0.005, 0.006)
        prof += [(y + w, 1.465), (y + w, 1.465 - d), (y - w, 1.465 - d), (y - w, 1.465)]
    prof += [(-0.915, 1.465)]
    body.extrude_x(prof, X_F - 0.06, X_R + 0.08, 'alu')
    for s in (1, -1):
        body.box((XM, s * 0.36, 1.44), (L_TRAILER - 0.2, 0.048, 0.01), 'steel_dark', bevel=0)
    # oak boards: two rows per side, staggered butt joints, carriage bolts
    for s in (1, -1):
        for row, yc in enumerate((0.989, 1.139)):
            x = X_F - 0.06; lengths = [3.66, 2.44, 3.05, 2.74] if row == 0 else [2.44, 3.66, 2.74, 3.05]
            k = 0
            while x > X_R + 0.08:
                ln = min(lengths[k % 4], x - (X_R + 0.08)); col = ('wood', 'wood2', 'wood3')[(k + row + (s > 0)) % 3]
                body.box((x - ln / 2, s * yc, 1.4435), (ln - 0.006, 0.146, 0.047), col, bevel=0.6)
                b = x - 0.3
                while b > x - ln + 0.1:
                    body.box((b, s * yc, 1.4675), (0.016, 0.016, 0.003), 'steel_dark', bevel=0); b -= 0.61
                x -= ln; k += 1
    # ---- side rails: extruded channel with rub-rail bead, winch track under it
    rail = [(1.215, 1.47), (1.288, 1.47), (1.30, 1.462), (1.30, 1.405), (1.312, 1.398), (1.312, 1.377), (1.30, 1.37),
            (1.30, 1.268), (1.292, 1.255), (1.215, 1.255), (1.215, 1.27), (1.282, 1.27), (1.282, 1.452), (1.215, 1.452)]
    track = [(1.165, 1.255), (1.25, 1.255), (1.25, 1.185), (1.236, 1.185), (1.236, 1.242), (1.18, 1.242), (1.18, 1.185), (1.165, 1.185)]
    for s in (1, -1):
        body.extrude_x([(s * y, z) for y, z in rail], X_F, X_R, 'alu')
        body.extrude_x([(s * y, z) for y, z in track], X_F - 0.3, X_R + 0.3, 'alu_dark')
        x = X_F - 0.45
        while x > X_R + 0.3:   # stake pockets with a dark opening
            body.box((x, s * 1.3005, 1.315), (0.07, 0.004, 0.075), 'alu_dark', bevel=0.5)
            body.box((x, s * 1.3015, 1.312), (0.05, 0.003, 0.055), 'rubber', bevel=0)
            x -= 0.61
        x = X_F - 0.8; i = 0
        while x > X_R + 0.4:   # conspicuity tape, 6 in red / 6 in white
            body.box((x, s * 1.3015, 1.433), (0.18, 0.002, 0.05), 'red' if i % 2 == 0 else 'white', bevel=0)
            body.box((x - 0.18, s * 1.3015, 1.433), (0.18, 0.002, 0.05), 'white' if i % 2 == 0 else 'red', bevel=0)
            x -= 0.9; i += 1
        for k in range(6): add_winch(body, -1.8 - k * 2.2, s)
        for x, col in ((X_F - 0.25, 'amber'), (-4.0, 'red'), (-7.6, 'red'), (-11.8, 'red'), (X_R + 0.25, 'red')):
            body.box((x, s * 1.3035, 1.338), (0.12, 0.006, 0.055), 'rubber', bevel=0.5)
            body.box((x, s * 1.308, 1.338), (0.095, 0.008, 0.036), col, LIGHT, bevel=1.0)
    # ---- crossmembers: channel lofts, deep between the beams, tapered outriggers to the rails
    def channel(x, y, zb):
        return [(x - 0.03, y, zb), (x + 0.03, y, zb), (x + 0.03, y, zb + 0.008), (x + 0.004, y, zb + 0.008),
                (x + 0.004, y, 1.412), (x + 0.03, y, 1.412), (x + 0.03, y, 1.42), (x - 0.03, y, 1.42)]
    x = X_F - 0.25
    while x > X_R + 0.2:
        if x > -1.3:
            body.box((x, 0, 1.36), (0.07, 2 * (W2 - 0.09), 0.12), 'alu_dark', bevel=0.5)   # coupler bolsters
        else:
            body.loft([channel(x, y, z) for y, z in ((-1.215, 1.36), (-0.555, 1.28), (-0.405, 1.30), (0.405, 1.30), (0.555, 1.28), (1.215, 1.36))], 'alu_dark', bevel=0)
        x -= 0.305
    # ---- main I-beams lofted along their tapering profile, web stiffeners
    xs = [X_F - 0.002]
    while xs[-1] - 0.25 > X_R + 0.1: xs.append(xs[-1] - 0.25)
    xs.append(X_R + 0.08)
    def ibeam(x, yc):
        zb = zbeam(x)
        pts = [(-0.075, zb), (0.075, zb), (0.075, zb + 0.022), (0.006, zb + 0.026), (0.006, 1.396), (0.075, 1.398), (0.075, 1.42),
               (-0.075, 1.42), (-0.075, 1.398), (-0.006, 1.396), (-0.006, zb + 0.026), (-0.075, zb + 0.022)]
        return [(x, yc + y, z) for y, z in pts]
    for s in (1, -1):
        body.loft([ibeam(x, s * 0.48) for x in xs], 'alu', bevel=0)
        x = -0.6
        while x > X_R + 0.4:
            for side in (1, -1):
                zb = zbeam(x) + 0.022; h = 1.396 - zb
                body.hexa([(x - 0.006, s * 0.48 + side * 0.006, zb), (x + 0.006, s * 0.48 + side * 0.006, zb),
                           (x + 0.006, s * 0.48 + side * 0.07, zb), (x - 0.006, s * 0.48 + side * 0.07, zb),
                           (x - 0.006, s * 0.48 + side * 0.006, zb + h), (x + 0.006, s * 0.48 + side * 0.006, zb + h),
                           (x + 0.006, s * 0.48 + side * 0.07, zb + h), (x - 0.006, s * 0.48 + side * 0.07, zb + h)], 'alu_dark', bevel=0)
            x -= 1.22
    # ---- upper coupler, kingpin, bolt rows
    body.box(((X_F - 1.4) / 2, 0, Z_PLATE + 0.01), (X_F + 1.4, 2 * (W2 - 0.12), 0.02), 'steel_dark')
    body.lathe((0, 0, Z_PLATE), [(0.005, -0.09), (0.046, -0.09), (0.05, -0.08), (0.05, -0.06), (0.036, -0.05),
                                 (0.036, -0.02), (0.06, -0.01), (0.06, 0.0), (0.005, 0.0)], 'steel', 'Z', 20)
    for y in (-1.1, -0.8, 0.8, 1.1):
        x = X_F - 0.15
        while x > -1.3:
            body.cyl((x, y, Z_PLATE - 0.003), 0.012, 0.008, 'Z', 'steel', segs=6); x -= 0.3
    # ---- front sill, lamps, gladhands, 7-way socket
    body.box((X_F - 0.03, 0, 1.38), (0.06, 2 * W2 + 0.02, 0.18), 'alu')
    for i in range(14):
        body.box((X_F + 0.0015, -W2 + 0.135 + i * 0.18, 1.432), (0.003, 0.18, 0.04), 'red' if i % 2 == 0 else 'white', bevel=0)
    for y in (-1.18, 1.18): body.lamp((X_F, y, 1.37), 'X', 1, 0.025, 'amber')
    for y, col in ((-0.34, 'red'), (-0.20, 'blue')):
        body.lathe((X_F, y, 1.33), [(0.005, 0.0), (0.022, 0.0), (0.022, 0.05), (0.03, 0.05), (0.03, 0.06), (0.005, 0.06)], 'steel', 'X', 12)
        body.box((X_F + 0.07, y, 1.33), (0.04, 0.07, 0.03), col)
        body.tube((X_F - 0.02, y, 1.30), (X_F - 0.30, y * 0.6 + 0.16, 1.26), 0.012, col, 8)
    body.lathe((X_F, 0.30, 1.33), [(0.005, 0.0), (0.045, 0.0), (0.045, 0.05), (0.035, 0.06), (0.005, 0.06)], 'steel_dark', 'X', 16)
    body.box((X_F + 0.055, 0.30, 1.37), (0.02, 0.09, 0.03), 'steel_dark')
    # ---- air lines (red emergency, blue service), black lamp harness with clips
    for y, z, col, r in ((0.40, 1.27, 'red', 0.009), (0.40, 1.245, 'blue', 0.009), (-0.40, 1.26, 'rubber', 0.011)):
        body.tube((X_F - 0.35, y, z), (X_R + 1.2, y, z), r, col, 8)
        x = X_F - 0.6
        while x > X_R + 1.3:
            body.box((x, y, z + 0.02), (0.02, 0.03, 0.04), 'steel_dark', bevel=0); x -= 1.22
    body.tube((X_F - 0.35, 0.16, 1.26), (X_F - 0.35, 0.40, 1.258), 0.01, 'red', 8)
    body.tube((X_R + 1.2, -0.40, 1.26), (X_R + 0.08, -0.40, 1.30), 0.011, 'rubber', 8)
    # ---- air tanks with domed heads, straps, drain valves
    for xt in (-9.05, -9.45):
        body.lathe((xt, 0, 0.80), [(0.005, -0.40), (0.07, -0.395), (0.12, -0.38), (0.145, -0.355), (0.15, -0.33),
                                   (0.15, 0.33), (0.145, 0.355), (0.12, 0.38), (0.07, 0.395), (0.005, 0.40)], 'galv', 'Y', 24)
        for s in (1, -1):
            body.lathe((xt, s * 0.25, 0.80), [(0.15, -0.015), (0.156, -0.015), (0.156, 0.015), (0.15, 0.015)], 'steel_dark', 'Y', 24)
            body.span((xt, s * 0.25, 0.95), (xt, s * 0.25, 1.30), 0.03, 0.04, 'steel_dark')
        body.cyl((xt, 0, 0.64), 0.012, 0.04, 'Z', 'steel', segs=8)
        body.tube((xt, 0.38, 0.85), (xt, 0.40, 1.245), 0.008, 'blue', 6)
    # ---- spare tyre in a cradle
    keep = body.M
    body.M = Matrix.Translation((-6.4, 0, 0.79)) @ Matrix.Rotation(math.radians(90), 4, 'X')
    add_wheel(body, 1, segs=32, single=True)
    body.M = keep
    for dx in (-0.32, 0.32):
        body.span((-6.4 + dx, -0.56, 0.625), (-6.4 + dx, 0.56, 0.625), 0.05, 0.04, 'steel_dark')
        for s in (1, -1):
            body.span((-6.4 + dx, s * 0.53, 0.605), (-6.4 + dx, s * 0.53, zbeam(-6.4 + dx) + 0.01), 0.04, 0.04, 'steel_dark')
    body.tube((-6.4, 0, 0.64), (-6.4, 0, 1.30), 0.012, 'steel', 8)   # hold-down rod
    # ---- aluminium tool box under the curb side
    tx0, tx1, ty0, ty1 = -7.55, -8.65, -0.64, -1.17; tcx = (tx0 + tx1) / 2
    body.box((tcx, (ty0 + ty1) / 2, 1.10), (abs(tx1 - tx0), abs(ty1 - ty0), 0.32), 'alu_dark')
    body.box((tcx, ty1 - 0.003, 1.10), (abs(tx1 - tx0) - 0.08, 0.006, 0.28), 'alu_bright')
    body.cyl((tcx, ty1 - 0.012, 1.245), 0.01, abs(tx1 - tx0) - 0.1, 'X', 'steel', segs=8)
    for dx in (tcx - 0.3, tcx + 0.3):
        body.box((dx, ty1 - 0.01, 1.00), (0.07, 0.012, 0.035), 'steel_dark')
        body.cyl((dx, ty1 - 0.022, 1.00), 0.012, 0.02, 'Y', 'chrome', segs=10)
    for dx in (tx0 - 0.04, tx1 + 0.04):
        body.span((dx, ty0, 0.93), (dx, ty1 - 0.02, 0.93), 0.04, 0.04, 'steel_dark')
        body.span((dx, ty1 - 0.02, 0.93), (dx, ty1 - 0.02, 1.36), 0.04, 0.04, 'steel_dark')
    # ---- rear sill: light pocket panel, stop/tail pairs, ID bar, tape, ICC under-ride bar
    body.box((X_R + 0.04, 0, 1.32), (0.08, 2 * W2 + 0.02, 0.30), 'alu')
    for i in range(14):
        body.box((X_R - 0.0015, -W2 + 0.135 + i * 0.18, 1.432), (0.003, 0.18, 0.045), 'red' if i % 2 == 0 else 'white', bevel=0)
    for s in (1, -1):
        body.box((X_R - 0.004, s * 1.04, 1.31), (0.008, 0.36, 0.17), 'steel_dark', bevel=0.5)
        for y in (0.96, 1.12): body.lamp((X_R - 0.008, s * y, 1.31), 'X', -1, 0.055, 'red', segs=20)
    body.box((X_R - 0.004, 0, 1.31), (0.008, 0.42, 0.07), 'steel_dark', bevel=0.5)
    for y in (-0.15, 0.0, 0.15): body.lamp((X_R - 0.008, y, 1.31), 'X', -1, 0.018, 'red', segs=12)
    body.box((X_R - 0.006, 0, 1.235), (0.012, 0.32, 0.035), 'steel_dark')   # licence lamp bracket
    body.lamp((X_R - 0.012, 0, 1.235), 'X', -1, 0.012, 'white', segs=10)
    body.lathe((X_R + 0.22, 0, 0.55), [(0.005, -1.15), (0.05, -1.15), (0.055, -1.14), (0.055, 1.14), (0.05, 1.15), (0.005, 1.15)], 'steel_dark', 'Y', 16)
    for i in range(10):
        body.box((X_R + 0.163, -1.035 + i * 0.23, 0.55), (0.004, 0.23, 0.06), 'red' if i % 2 == 0 else 'white', bevel=0)
    for y in (-0.48, 0.48):
        body.hexa([(X_R + 0.18, y - 0.04, 0.55), (X_R + 0.26, y - 0.04, 0.55), (X_R + 0.26, y + 0.04, 0.55), (X_R + 0.18, y + 0.04, 0.55),
                   (X_R + 0.10, y - 0.04, 1.14), (X_R + 0.30, y - 0.04, 1.14), (X_R + 0.30, y + 0.04, 1.14), (X_R + 0.10, y + 0.04, 1.14)], 'steel_dark')
        body.tube((X_R + 0.25, y, 0.62), (X_R + 0.95, y, 1.14), 0.025, 'steel_dark')
    # ---- landing gear (moving inner legs are separate objects)
    for s in (1, -1):
        y = s * Y_LEG
        body.box((X_GEAR, y, 0.72), (0.15, 0.15, 0.52), 'steel_dark')
        body.box((X_GEAR, y, 0.47), (0.17, 0.17, 0.035), 'steel_dark')
        body.box((X_GEAR, s * 0.53, 0.955), (0.30, 0.16, 0.05), 'steel_dark')
        for dx in (-0.1, 0.1):
            for dy in (-0.05, 0.05): body.cyl((X_GEAR + dx, s * 0.53 + dy, 0.983), 0.012, 0.008, 'Z', 'steel', segs=6)
        bx = X_GEAR + 1.0
        body.tube((X_GEAR + 0.05, y, 0.56), (bx, s * 0.48, zbeam(bx) + 0.01), 0.025, 'steel_dark')
        body.box((bx, s * 0.48, zbeam(bx) - 0.005), (0.12, 0.10, 0.02), 'steel_dark')
        gy = y + s * 0.125
        body.lathe((X_GEAR, gy, 0.86), [(0.005, -0.05), (0.075, -0.05), (0.08, -0.03), (0.08, 0.03), (0.075, 0.05), (0.005, 0.05)], 'steel', 'Y', 18)
    body.tube((X_GEAR, Y_LEG + 0.07, 0.62), (X_GEAR, -Y_LEG - 0.07, 0.62), 0.03, 'steel_dark')
    body.tube((X_GEAR, Y_LEG, 0.84), (X_GEAR, -Y_LEG, 0.56), 0.02, 'steel_dark')
    body.tube((X_GEAR, -Y_LEG, 0.84), (X_GEAR, Y_LEG, 0.56), 0.02, 'steel_dark')
    body.tube((X_GEAR, -Y_LEG - 0.175, 0.86), (X_GEAR, Y_LEG + 0.175, 0.86), 0.018, 'steel')
    body.tube((X_GEAR, -Y_LEG - 0.175, 0.86), (X_GEAR, -1.05, 0.86), 0.015, 'steel')
    body.tube((X_GEAR, -1.05, 0.86), (X_GEAR + 0.02, -1.05, 0.62), 0.014, 'steel')
    body.lathe((X_GEAR + 0.02, -1.05, 0.62), [(0.005, -0.01), (0.02, -0.01), (0.024, -0.06), (0.02, -0.13), (0.005, -0.13)], 'rubber', 'Y', 12)
    body.box((X_GEAR + 0.02, -1.05, 0.70), (0.06, 0.04, 0.03), 'steel_dark')   # crank hanger hook
    # ---- spread-axle air-ride suspension, brakes, mudflaps
    Z_A = R_WHEEL
    for ai, xa in enumerate(AXLES):
        body.lathe((xa, 0, Z_A), [(0.05, -0.80), (0.065, -0.78), (0.065, 0.78), (0.05, 0.80)], 'steel_dark', 'Y', 16)
        for s in (1, -1):
            drum = [(0.07, 0.66), (0.20, 0.66), (0.21, 0.672), (0.21, 0.80), (0.19, 0.81), (0.07, 0.81)]
            body.lathe((xa, 0, Z_A), [(r, s * t) for r, t in drum], 'steel', 'Y', 24)
            for k in range(12):
                a = k * math.tau / 12
                body.box((xa + 0.215 * math.cos(a), s * 0.735, Z_A + 0.215 * math.sin(a)), (0.016, 0.12, 0.016),
                         'steel_dark', rot=Matrix.Rotation(-a, 4, 'Y'), bevel=0)
            body.lathe((xa, 0, Z_A), [(r, s * t) for r, t in [(0.07, 0.64), (0.24, 0.646), (0.24, 0.652), (0.07, 0.656)]], 'steel_dark', 'Y', 24)
            body.tube((xa + 0.07, s * 0.30, Z_A + 0.05), (xa + 0.07, s * 0.65, Z_A + 0.05), 0.02, 'steel_dark', 10)
            body.box((xa + 0.07, s * 0.32, Z_A + 0.12), (0.05, 0.035, 0.17), 'steel')
            body.cyl((xa + 0.07, s * 0.32, Z_A + 0.05), 0.04, 0.04, 'Y', 'steel', segs=12)
            body.tube((xa + 0.07, s * 0.32, Z_A + 0.19), (xa + 0.29, s * 0.32, Z_A + 0.19), 0.009, 'steel', 8)
            chamber = [(0.01, 0.29), (0.07, 0.29), (0.085, 0.31), (0.09, 0.345), (0.10, 0.355), (0.10, 0.365),
                       (0.09, 0.375), (0.095, 0.41), (0.098, 0.48), (0.095, 0.52), (0.08, 0.545), (0.03, 0.555), (0.01, 0.556)]
            body.lathe((xa, s * 0.32, Z_A + 0.19), chamber, 'steel_dark', 'X', 20)
            body.lathe((xa, s * 0.32, Z_A + 0.19), [(0.10, 0.355), (0.108, 0.355), (0.108, 0.365), (0.10, 0.365)], 'steel', 'X', 20)
            body.box((xa + 0.30, s * 0.32, Z_A + 0.11), (0.03, 0.06, 0.14), 'steel_dark')
            body.tube((xa + 0.42, s * 0.32, Z_A + 0.29), (xa + 0.47, s * 0.20, Z_A + 0.40), 0.008, 'rubber', 6)
        hx = xa + 0.62
        for s in (1, -1):
            y = s * 0.48
            body.hexa([(hx - 0.07, y - 0.05, 0.58), (hx + 0.07, y - 0.05, 0.58), (hx + 0.07, y + 0.05, 0.58), (hx - 0.07, y + 0.05, 0.58),
                       (hx - 0.17, y - 0.05, 1.14), (hx + 0.17, y - 0.05, 1.14), (hx + 0.17, y + 0.05, 1.14), (hx - 0.17, y + 0.05, 1.14)], 'steel_dark')
            body.box((hx, y, 1.125), (0.42, 0.17, 0.025), 'steel_dark')
            for dx in (-0.16, 0.16):
                for dy in (-0.07, 0.07): body.cyl((hx + dx, y + dy, 1.11), 0.012, 0.01, 'Z', 'steel', segs=6)
            body.lathe((hx, y, 0.645), [(0.025, -0.075), (0.068, -0.075), (0.07, -0.065), (0.07, 0.065), (0.068, 0.075), (0.025, 0.075)], 'rubber', 'Y', 16)
            body.cyl((hx, y, 0.645), 0.03, 0.18, 'Y', 'steel', segs=6)
            # tapered trailing arm, underslung
            def arm(xp, zc, h):
                return [(xp, y - 0.05, zc - h / 2), (xp, y + 0.05, zc - h / 2), (xp, y + 0.05, zc + h / 2), (xp, y - 0.05, zc + h / 2)]
            body.loft([arm(hx, 0.63, 0.12), arm((hx + xa) / 2, 0.47, 0.11), arm(xa + 0.02, 0.36, 0.10), arm(xa - 0.25, 0.39, 0.09), arm(xa - 0.52, 0.44, 0.08)], 'steel_dark')
            body.box((xa, y, 0.405), (0.22, 0.14, 0.05), 'steel_dark')
            body.box((xa, y, 0.55), (0.20, 0.15, 0.025), 'steel_dark')
            for dx in (-0.075, 0.075):
                for dy in (-0.06, 0.06):
                    body.tube((xa + dx, y + dy, 0.33), (xa + dx, y + dy, 0.575), 0.01, 'steel', 8)
                    body.cyl((xa + dx, y + dy, 0.33), 0.016, 0.02, 'Z', 'steel', segs=6)
            ax = xa - 0.45
            bellows = [(0.005, 0.48), (0.10, 0.48), (0.10, 0.60), (0.13, 0.615), (0.155, 0.645), (0.162, 0.69), (0.155, 0.735),
                       (0.132, 0.755), (0.155, 0.775), (0.162, 0.82), (0.155, 0.865), (0.14, 0.885), (0.16, 0.89), (0.162, 0.915), (0.005, 0.915)]
            body.lathe((ax, y, 0), bellows, 'rubber', 'Z', 24)
            body.box((ax, y, 1.03), (0.24, 0.13, 0.22), 'steel_dark')
            body.tube((ax + 0.08, y, 0.92), (ax + 0.10, s * 0.40, 1.246), 0.007, 'blue', 6)
            a, b = Vector((hx - 0.08, s * 0.405, 1.00)), Vector((xa + 0.14, s * 0.405, 0.43))
            m = a.lerp(b, 0.5)
            body.tube(a, m, 0.035, 'steel', 12); body.tube(m, b, 0.016, 'chrome', 8); body.tube(m, m.lerp(b, 0.15), 0.04, 'rubber', 12)
            body.cyl(a, 0.03, 0.06, 'Y', 'rubber', segs=10); body.cyl(b, 0.03, 0.06, 'Y', 'rubber', segs=10)
        body.tube((hx, -0.43, 0.82), (hx, 0.43, 0.82), 0.045, 'steel_dark', 14)
        for s in (1, -1):
            body.tube((hx, s * 0.43, 0.70), (hx + 0.75, 0, 1.29), 0.025, 'steel_dark')
        if ai == 0:   # height control valve and linkage
            body.box((xa - 0.2, 0.40, 1.08), (0.10, 0.06, 0.08), 'steel')
            body.tube((xa - 0.2, 0.43, 1.06), (xa - 0.08, 0.43, 1.06), 0.006, 'steel', 6)
            body.tube((xa - 0.08, 0.43, 1.06), (xa - 0.25, 0.43, 0.44), 0.006, 'steel', 6)
        for s in (1, -1):
            fx = xa - 0.78
            body.box((fx, s * Y_DUAL, 0.56), (0.014, 0.62, 0.60), 'rubber', bevel=0.7)
            for zr in (0.30, 0.34, 0.38):
                body.box((fx - 0.009, s * Y_DUAL, zr), (0.006, 0.58, 0.012), 'rubber', bevel=0)
            body.box((fx - 0.012, s * Y_DUAL, 0.85), (0.012, 0.62, 0.035), 'steel_dark')
            body.tube((fx, s * 0.48, 1.14), (fx, s * 0.48, 0.87), 0.022, 'steel_dark')
            body.tube((fx, s * 0.48, 0.87), (fx, s * (Y_DUAL + 0.31), 0.87), 0.022, 'steel_dark')
            body.lathe((fx, s * 0.62, 0.87), [(0.022, -0.05), (0.032, -0.05), (0.032, 0.05), (0.022, 0.05)], 'steel', 'Y', 10)
    # ---- ABS relay valve between the axles, hoses out to each brake chamber
    vx = (AXLES[0] + AXLES[1]) / 2
    body.box((vx, 0, 1.22), (0.20, 0.18, 0.12), 'steel_dark')
    body.box((vx, 0, 1.15), (0.10, 0.10, 0.05), 'steel')
    body.box((vx + 0.14, 0.1, 1.22), (0.08, 0.06, 0.06), 'galv')
    body.tube((vx, 0, 1.28), (vx, 0.40, 1.245), 0.009, 'blue', 6)
    for xa in AXLES:
        for s in (1, -1):
            mid = (vx * 0.4 + (xa + 0.42) * 0.6, s * 0.26, 0.95)
            body.tube((vx, s * 0.08, 1.17), mid, 0.008, 'rubber', 6)
            body.tube(mid, (xa + 0.42, s * 0.32, Z_A + 0.29), 0.008, 'rubber', 6)
    body_ob = body.to_object('Trailer_Body', coll, mats)

    wheels = []
    for i, (xa, s) in enumerate([(AXLES[0], 1), (AXLES[0], -1), (AXLES[1], 1), (AXLES[1], -1)]):
        w = Part(); add_wheel(w, s)
        ob = w.to_object('Trailer_Wheel_%d' % i, coll, mats, bevel_width=0); ob.location = (xa, s * Y_DUAL, R_WHEEL); wheels.append(ob)
    legs = []
    for i, s in enumerate((1, -1)):
        p = Part()
        p.box((0, 0, 0.27), (0.11, 0.11, 0.50), 'steel')
        p.box((0, 0, 0.50), (0.125, 0.125, 0.02), 'steel_dark')
        p.cyl((0, 0, 0.045), 0.035, 0.07, 'Y', 'steel_dark', segs=12)
        p.hexa([(-0.14, -0.12, 0.0), (0.14, -0.12, 0.0), (0.14, 0.12, 0.0), (-0.14, 0.12, 0.0),
                (-0.10, -0.10, 0.035), (0.10, -0.10, 0.035), (0.10, 0.10, 0.035), (-0.10, 0.10, 0.035)], 'steel_dark')
        ob = p.to_object('Trailer_Leg_%d' % i, coll, mats); ob.location = (X_GEAR, s * Y_LEG, GEAR_DROP); legs.append(ob)

    root = bpy.data.objects.new('FlatbedTrailer_ROOT', None); root.empty_display_type = 'ARROWS'; root.empty_display_size = 1.5
    coll.objects.link(root)
    root['travel_metres'] = 0.0; root['landing_gear'] = 0.0
    for o in [body_ob] + wheels + legs: o.parent = root
    for ob in wheels:
        d = ob.driver_add('rotation_euler', 1).driver; d.type = 'SCRIPTED'
        v = d.variables.new(); v.name = 't'; v.targets[0].id = root; v.targets[0].data_path = '["travel_metres"]'
        d.expression = 't/%.5f' % R_WHEEL
    for ob in legs:
        d = ob.driver_add('location', 2).driver; d.type = 'SCRIPTED'
        v = d.variables.new(); v.name = 'g'; v.targets[0].id = root; v.targets[0].data_path = '["landing_gear"]'
        d.expression = '%.3f-%.3f*g' % (GEAR_DROP, GEAR_DROP)

    # tractor reference (same offsets as AFoundrylineFreightRig, Unreal Y mirrored)
    tcoll = bpy.data.collections.new('TractorReference'); scene.collection.children.link(tcoll)
    troot = bpy.data.objects.new('Tractor_ROOT', None); troot.empty_display_type = 'ARROWS'; tcoll.objects.link(troot)
    troot['travel_metres'] = 0.0; troot['steering_degrees'] = 0.0
    spots = {'Tractor_Body': (0, 0, 0), 'Tractor_Wheel_0': (0.00926, 95.22259, 45.53076), 'Tractor_Wheel_1': (0.00921, -95.59264, 45.53089),
             'Tractor_Wheel_2': (367.83292, 114.05562, 45.51796), 'Tractor_Wheel_3': (367.83297, -114.08374, 45.5181)}
    for name, p in spots.items():
        before = set(bpy.data.objects)
        bpy.ops.import_scene.fbx(filepath=os.path.join(TRACTOR, name + '.fbx'))
        for o in [o for o in bpy.data.objects if o not in before]:
            for c in o.users_collection: c.objects.unlink(o)
            tcoll.objects.link(o); o.name = name; o.parent = troot
            o.location = (p[0] / 100, -p[1] / 100, p[2] / 100); o.rotation_euler = (0, 0, 0)
            if name != 'Tractor_Body':
                d = o.driver_add('rotation_euler', 1).driver; d.type = 'SCRIPTED'
                v = d.variables.new(); v.name = 't'; v.targets[0].id = troot; v.targets[0].data_path = '["travel_metres"]'
                d.expression = 't/0.47352'
                if name in ('Tractor_Wheel_2', 'Tractor_Wheel_3'):
                    d = o.driver_add('rotation_euler', 2).driver; d.type = 'SCRIPTED'
                    v = d.variables.new(); v.name = 's'; v.targets[0].id = troot; v.targets[0].data_path = '["steering_degrees"]'
                    d.expression = 'radians(s)'
    return root, troot, body_ob, wheels, legs

def bake_demo(root, troot):
    scene = bpy.context.scene; fps = 30; scene.render.fps = fps
    WB, HITCH, L = 3.678, 0.42, KINGPIN_TO_AXLES
    DELTA = math.degrees(math.atan(WB / 14.0))
    def steer(d):
        if d < 15: return 0.0
        if d < 18: return DELTA * (d - 15) / 3
        if d < 38: return DELTA
        if d < 41: return DELTA * (41 - d) / 3
        return 0.0
    def profile(dist, vmax, acc):
        ta = vmax / acc; da = 0.5 * acc * ta * ta
        if 2 * da > dist: ta = math.sqrt(dist / acc); vmax = acc * ta; da = dist / 2
        tc = (dist - 2 * da) / vmax; T = 2 * ta + tc; out = []
        for f in range(int(math.ceil(T * fps)) + 1):
            t = min(f / fps, T)
            if t < ta: s = 0.5 * acc * t * t
            elif t < ta + tc: s = da + vmax * (t - ta)
            else: u = T - t; s = dist - 0.5 * acc * u * u
            out.append(s)
        return out
    for o in (root, troot): o.animation_data_clear()
    tx, ty, tyaw = -45.0, -10.0, 0.0
    ttrav = 0.0; th = 0.0; rtrav = 0.0; gear = 0.0
    hx, hy = tx + HITCH, ty
    frame = 1; markers = []
    def key(steer_deg):
        nonlocal frame
        troot.location = (tx, ty, 0); troot.rotation_euler = (0, 0, tyaw)
        troot['travel_metres'] = ttrav; troot['steering_degrees'] = steer_deg
        root.location = (hx, hy, 0); root.rotation_euler = (0, 0, th)
        root['travel_metres'] = rtrav; root['landing_gear'] = gear
        for o, paths in ((troot, ('location', 'rotation_euler', '["travel_metres"]', '["steering_degrees"]')),
                         (root, ('location', 'rotation_euler', '["travel_metres"]', '["landing_gear"]'))):
            for p in paths: o.keyframe_insert(p, frame=frame)
        frame += 1
    def drive(dist, vmax, acc, coupled, steering=True):
        nonlocal tx, ty, tyaw, ttrav, th, hx, hy, rtrav
        start = ttrav; last = 0.0
        for s in profile(dist, vmax, acc)[1:]:
            ds_total = s - last; last = s; n = 6
            for _ in range(n):
                ds = ds_total / n
                d = steer(ttrav - start) if steering else 0.0
                dyaw = ds / WB * math.tan(math.radians(d))
                mid = tyaw + dyaw / 2
                tx += ds * math.cos(mid); ty += ds * math.sin(mid); tyaw += dyaw; ttrav += ds
                if coupled:
                    nhx = tx + HITCH * math.cos(tyaw); nhy = ty + HITCH * math.sin(tyaw)
                    dhx, dhy = nhx - hx, nhy - hy
                    ax0 = hx - L * math.cos(th); ay0 = hy - L * math.sin(th)
                    th += (dhx * -math.sin(th) + dhy * math.cos(th)) / L
                    hx, hy = nhx, nhy
                    ax1 = hx - L * math.cos(th); ay1 = hy - L * math.sin(th)
                    rtrav += (ax1 - ax0) * math.cos(th) + (ay1 - ay0) * math.sin(th)
            key(steer(ttrav - start) if steering else 0.0)
    key(0.0)
    markers.append(('approach', 1))
    drive(62.0, 4.0, 1.2, True)
    markers.append(('parked', frame))
    for _ in range(15): key(0.0)
    markers.append(('landing gear down', frame))
    for i in range(60):
        gear = (i + 1) / 60; key(0.0)
    for _ in range(10): key(0.0)
    markers.append(('uncoupled, tractor pulls away', frame))
    drive(14.0, 3.0, 1.0, False, steering=False)
    markers.append(('trailer parked on its legs', frame))
    for _ in range(20): key(0.0)
    scene.frame_start = 1; scene.frame_end = frame - 1
    for m in list(scene.timeline_markers): scene.timeline_markers.remove(m)
    for n, f in markers: scene.timeline_markers.new(n, frame=f)
    for o, name in ((root, 'FlatbedTrailer_DriveTurnDrop_Demo'), (troot, 'Tractor_DriveTurnDrop_Demo')):
        o.animation_data.action.name = name
    return {'frames': frame - 1}

def report(body, wheels, legs):
    bpy.context.view_layer.update()
    dg = bpy.context.evaluated_depsgraph_get()
    def tris(o):
        me = o.evaluated_get(dg).to_mesh(); n = sum(len(p.vertices) - 2 for p in me.polygons); o.evaluated_get(dg).to_mesh_clear(); return n
    bb = [body.matrix_world @ Vector(c) for c in body.bound_box]
    return {'body_bounds_m': [[round(min(v[i] for v in bb), 3) for i in range(3)], [round(max(v[i] for v in bb), 3) for i in range(3)]],
            'body_tris_evaluated': tris(body), 'wheel_tris': tris(wheels[0]), 'leg_tris': tris(legs[0]),
            'kingpin_to_tandem_centre_m': KINGPIN_TO_AXLES, 'wheel_radius_m': R_WHEEL, 'deck_top_m': Z_DECK}

def export(dest, body, wheels, legs):
    """One FBX per part in its own local space (modifiers applied), plus manifest.json in Unreal cm (Y mirrored)."""
    import json
    os.makedirs(dest, exist_ok=True)
    dg = bpy.context.evaluated_depsgraph_get(); parts = {}
    for o in [body] + wheels + legs:
        me = bpy.data.meshes.new_from_object(o.evaluated_get(dg))
        tmp = bpy.data.objects.new(o.name + '_Export', me); bpy.context.scene.collection.objects.link(tmp)
        bpy.ops.object.select_all(action='DESELECT'); tmp.select_set(True); bpy.context.view_layer.objects.active = tmp
        tmp.name = o.name
        bpy.ops.export_scene.fbx(filepath=os.path.join(dest, o.name + '.fbx'), use_selection=True, object_types={'MESH'},
                                 apply_unit_scale=True, axis_forward='-Y', axis_up='Z', bake_anim=False, add_leaf_bones=False,
                                 mesh_smooth_type='FACE')
        parts[o.name] = {'triangles': sum(len(p.vertices) - 2 for p in me.polygons)}
        bpy.data.objects.remove(tmp); bpy.data.meshes.remove(me)
    cm = lambda v: [round(v[0] * 100, 3), round(-v[1] * 100, 3), round(v[2] * 100, 3)]
    man = {'units': 'Unreal cm, X forward, Y mirrored from Blender', 'origin': 'ground under the kingpin',
           'kingpin_cm': [0, 0, Z_PLATE * 100], 'tractor_fifth_wheel_offset_cm': [42, 0, 128],
           'kingpin_to_tandem_centre_cm': KINGPIN_TO_AXLES * 100, 'deck_top_cm': Z_DECK * 100,
           'length_cm': L_TRAILER * 100, 'width_cm': W2 * 200, 'wheel_radius_cm': R_WHEEL * 100,
           'wheels': {w.name: cm(w.location) for w in wheels},
           'legs_rest': {l.name: cm((l.location.x, l.location.y, GEAR_DROP)) for l in legs}, 'landing_gear_drop_cm': GEAR_DROP * 100,
           'material_slots': ['Trailer_Palette (Megapoly 256 gradient palette UVs)', 'Trailer_MarkerLight (same palette, emissive)'],
           'parts': parts}
    with open(os.path.join(dest, 'manifest.json'), 'w') as f: json.dump(man, f, indent=1)
    return man

root, troot, body, wheels, legs = build()
bpy.context.scene.frame_set(1)
rep = report(body, wheels, legs)
rep['demo'] = bake_demo(root, troot)
if globals().get('EXPORT_DIR'):
    bpy.context.scene.frame_set(1)
    rep['export'] = export(EXPORT_DIR, body, wheels, legs)
result = rep
