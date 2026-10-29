import unreal,json
from pathlib import Path
rows=[]
for name in ['SM_Lamp_01','SM_Lamp_02','SM_Lamp_03','SM_Lamp_04','SM_Lights_01']:
 m=unreal.load_asset('/Game/Factory/Meshes/Props/Light/'+name)
 if m:
  b=m.get_bounding_box();rows.append({'name':name,'min':str(b.min),'max':str(b.max)})
Path(unreal.Paths.project_saved_dir()+'Logs/HangingLightAssets.json').write_text(json.dumps(rows,indent=2))
