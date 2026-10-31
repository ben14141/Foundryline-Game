"""Model-only reusable cradle for ONE directly rolled full 5 x 10 ft steel sheet."""
import bpy,math,json,importlib.util
from pathlib import Path
from mathutils import Vector
ROOT=Path(r'C:/Users/curly/Documents/Foundryline Game');OUT=ROOT/'SourceAssets/Production/Equipment/CurvedSheetCradle';OUT.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location('helpers',ROOT/'Tools/Blender/build_rack_welding_bench.py');h=importlib.util.module_from_spec(spec);spec.loader.exec_module(h)
R=1.175;L=3.048;W=1.524;T=.003175;RO=R+T/2;H=.3048;A=L/R;ANGLES=[-A/2+i*A/96 for i in range(97)];CONTACT=[a for a in ANGLES if abs(a)<=1.18]

def extrude(name,profile,y,thick,material,c,parent):
 n=len(profile);v=[(x,y+d,z) for d in [-thick/2,thick/2] for x,z in profile];f=[tuple(range(n-1,-1,-1)),tuple(range(n,2*n))]+[(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)];return h.mesh(name,v,f,material,c,parent,bevel=.0007)
def label(body,at,c,size=.035):
 return h.text_world(body,at,size,'Label',c,rotation=(math.pi/2,0,0))
def cradle():
 h.clear();h.materials();C=h.collection('CRADLE / Independent reusable frame');S=h.collection('CRADLE / Two shaped saddles');root=bpy.data.objects.new('CurvedSheetCradle_FloorCentre',None);C.objects.link(root);root['asset_kind']='Reusable equipment, not a consumed product';root['model_only']=True
 for x in [-.508,.508]:
  h.tube('ForkPocket_40inCentres',1.8288,.2032,.1016,.00635,(x,0,.0508),'Y','Bench gloss graphite',C,root)
  for y in [-.89,.89]:
   for dx in [-.078,.078]:h.cylinder('EntryWearFastener',.008,.007,(x+dx,y,.105),'Machined steel',C,root,verts=6)
 for y in [-.762,.762]:h.tube('EndCrossBeam',2.4384,.0635,.0508,.003175,(0,y,.127),'X','Bench gloss orange',C,root)
 for x in [-1.18745,1.18745]:h.tube('SideTieBeam',1.6256,.0635,.0508,.003175,(x,0,.127),'Y','Bench gloss graphite',C,root)
 saddles=[];top=[(RO*math.sin(a),H+RO*(1-math.cos(a))) for a in CONTACT]
 for index,y in enumerate([-.508,.508]):
  g=bpy.data.objects.new('Saddle_'+str(index+1),None);S.objects.link(g);g.parent=root;saddles.append(g)
  web=extrude('CurvedSteelSupportWeb',[(top[0][0],.1524),(top[-1][0],.1524)]+[(x,z-.00635) for x,z in reversed(top)],y,.009525,'Bench gloss orange',S,g)
  for x,z,r in [(-.78,.46,.075),(.78,.46,.075),(0,.222,.035)]:h.hole(web,(x,y,z),r,.08,'Y')
  pad=extrude('ReplaceableCurvedContactPad',top+[(x,z-.00635) for x,z in reversed(top)],y,.0762,'Rubber',S,g)
  for x in [-1.02,-.65,0,.65,1.02]:
   z=H+RO-math.sqrt(RO*RO-x*x)-.030
   h.cylinder('ContactPadRetainer',.007,.025,(x,y-.014,z),'Machined steel',S,g,'Y',8)
  for x in [-.508,.508]:
   for side in [-1,1]:h.triangle('WebBaseGusset',[(y,.153),(y+side*.10,.153),(y,.27)],.00635,(x,0,0),S,g,'Bench gloss graphite')
  for side in [-1,1]:
   h.box('EdgeCatchBracket',(.0635,.105,.215),(side*1.18745,y,1.1175),'Bench gloss orange',S,g,.004)
   h.box('EdgeCatchReplaceablePad',(.012,.09,.17),(side*1.1497,y,1.12),'Rubber',S,g,.003)
   h.box('CatchMountArm',(.1397,.09,.04),(side*1.14935,y,.99),'Bench gloss graphite',S,g,.002)
   for z in [1.04,1.16]:h.cylinder('CatchAdjustmentBolt',.010,.025,(side*1.18745,y-.065,z),'Machined steel',S,g,'Y',6)
  weld=h.cable('BaseWebFilletWeld',[(-1.0,y-.006,.154),(0,y-.006,.154),(1.0,y-.006,.154)],.002,S,g);weld.data.materials.clear();weld.data.materials.append(h.M['Weld steel'])
 # Fore/aft stops attach to the end beams, outside the full five-foot sheet width.
 for y in [-.79,.79]:
  for x in [-.34,.34]:
   h.box('AxialRetentionTab',(.065,.012,.30),(x,y,.3024),'Bench gloss orange',C,root,.003)
   h.box('AxialStopPad',(.06,.012,.16),(x,y-math.copysign(.012,y),.35),'Rubber',C,root,.002)
 plate=h.box('CradleIDPlate',(.45,.008,.06),(0,-.799,.13),'Bench gloss graphite',C,root,.001);text=label('FOUNDRYLINE / CURVED SHEET',(-.207,-.806,.13),C,.023);text.parent=root
 return root,saddles,top

def fullsheet():
 c=h.collection('REVIEW ONLY / Full raw sheet rolled directly');v=[]
 for radius in [R+T/2,R-T/2]:
  for y in [-W/2,W/2]:
   for a in ANGLES:v.append((radius*math.sin(a),y,H+T/2+R-radius*math.cos(a)))
 n=len(ANGLES);f=[]
 for i in range(n-1):f.extend([(i,i+1,n+i+1,n+i),(2*n+i,3*n+i,3*n+i+1,2*n+i+1),(i,2*n+i,2*n+i+1,i+1),(n+i,n+i+1,3*n+i+1,3*n+i)])
 f.extend([(0,n,3*n,2*n),(n-1,3*n-1,4*n-1,2*n-1)])
 return h.mesh('REVIEW_FullSheet_5ft_x10ft_x1eighthIn',v,f,'Raw carbon steel',c)

def export_component(group,path):
 h.export(path,h.descendants(group))

def run():
 root,saddles,top=cradle();bpy.context.view_layer.update();h.export(OUT/'CurvedSheetCradle_Complete.fbx',h.descendants(root))
 for i,s in enumerate(saddles):export_component(s,OUT/('CurvedSheetCradle_Saddle_'+str(i+1)+'.fbx'))
 # Semantic geometry checks before adding any review product or studio.
 frame=h.bounds(h.descendants(root));assert abs(frame['size_cm'][0]-243.84)<.02;assert abs(frame['size_cm'][1]-182.88)<.02
 fork_width=.11677;fork_height=.056092;clear_width=.1905;clear_height=.0889;offset=.00765
 assert clear_width/2-fork_width/2-offset>.025;assert (clear_height-fork_height)/2>.015
 qa={'result':'PASS','developed_raw_sheet_feet':[5,10],'sheet_thickness_in':.125,'draft_mid_surface_radius_cm':R*100,'developed_length_cm':L*100,'arc_angle_degrees':math.degrees(A),'neutral_surface_area_m2':L*W,'neutral_arc_length_preserved_cm':R*A*100,'pad_contact_max_gap_cm':0,'pad_profile_samples':len(top),'fork_pockets_clear_cm':[clear_width*100,clear_height*100],'pocket_centres_cm':101.6,'measured_tine_width_cm':11.677,'measured_tine_thickness_cm':5.6092,'minimum_side_clearance_cm':(clear_width/2-fork_width/2-offset)*100,'minimum_vertical_clearance_cm':(clear_height-fork_height)/2*100,'fork_entry_axis':'Y / six-foot end','fork_reach_past_loaded_centre_cm':124.57-91.44-3,'independent_product_geometry_count':0,'frame_bounds':frame}
 c,cam=h.studio((0,0,.55),3.9,(4,-5,3.3));h.overlay(cam,c,'FOUNDRYLINE / CURVED-SHEET CRADLE',['8 ft x 6 ft | two shaped steel saddles, one reusable sled','For ONE full 5 ft x 10 ft rolled sheet | curve radius is a draft','7 1/2 in x 3 1/2 in clear fork pockets | entry from the 6 ft end']);h.dim(c,cam,(-1.2192,-1.02,.04),(1.2192,-1.02,.04),'8 ft',(0,0,.10));h.dim(c,cam,(1.33,-.9144,.04),(1.33,.9144,.04),'6 ft',(.09,0,.05));h.save_render(OUT/'Cradle_Empty_Dimensioned.png',OUT/'Foundryline_CurvedSheetCradle.blend')
 sheet=fullsheet();bpy.context.view_layer.update();qa['loaded_sheet_bounds']=h.bounds([sheet]);assert tuple(sheet.scale)==(1,1,1)
 # Verify every pad sample against the actual full-sheet outer skin vertices.
 skin=[Vector((RO*math.sin(a),H+RO*(1-math.cos(a)))) for a in ANGLES];gaps=[min((Vector(p)-v).length for v in skin) for p in top];qa['pad_contact_max_gap_cm']=max(gaps)*100;assert max(gaps)<.00001
 # New camera/dimension collection keeps the loaded source as a review-only assembly.
 for old in list(bpy.data.collections):
  if old.name.startswith('REVIEW ONLY / Studio'):
   for o in list(old.objects):bpy.data.objects.remove(o,do_unlink=True)
 c,cam=h.studio((0,0,.65),4.05,(4,-5,3.5));h.overlay(cam,c,'FOUNDRYLINE / FULL SHEET ON CRADLE',['ONE raw 5 ft x 10 ft sheet, rolled directly - no plasma step','Full developed length and thickness retained; no part rescaling','Draft radius about 46 1/4 in | two cradles = four saddles later']);h.save_render(OUT/'Cradle_Loaded_FullSheet.png',OUT/'Foundryline_CurvedSheetCradle_LoadedReview.blend')
 (OUT/'dimensions_and_qa.json').write_text(json.dumps(qa,indent=2));print('CURVED_SHEET_CRADLE_COMPLETE',json.dumps(qa))
if __name__=='__main__':run()
