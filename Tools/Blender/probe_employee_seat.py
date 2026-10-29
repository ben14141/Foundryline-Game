"""Read authored horizontal surfaces around the forklift seat, wheel and cabin floor."""
import bpy,json
from pathlib import Path
p=Path(r'C:/Users/curly/Documents/Foundryline Game')
bpy.ops.wm.open_mainfile(filepath=str(p/'SourceAssets/MaterialHandling/Authored/Foundryline_MaterialHandling.blend'))
o=bpy.data.objects.get('Forklift_Body');out={}
for label,region in [('rear_platform',(-1.8,-1.2,1.15,1.23)),('seat_cushion',(-1.2,-.8,1.38,1.45)),('cabin_floor',(-1.0,-.4,.8,.86))]:
 rows=[]
 for f in o.data.polygons:
  c=f.center
  if region[0]<c.x<region[1] and abs(c.y)<.5 and region[2]<c.z<region[3] and f.normal.z>.9:rows.append({'center_cm':[round(v*100,3) for v in c],'area_m2':f.area})
 out[label]=sorted(rows,key=lambda x:-x['area_m2'])[:4]
(p/'Saved/Logs/EmployeeSeatProbe.json').write_text(json.dumps(out,indent=2))
