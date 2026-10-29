import unreal,json
from pathlib import Path
rows=[]
for path in ['/Game/Factory/Meshes/Road/Ground/SM_Ground_400','/Game/Factory/Meshes/Road/SM_Ground_Line_A1','/Game/Factory/Meshes/Road/SM_Lane_400','/Game/Factory/Meshes/Road/Truck/SM_Truck_400']:
 m=unreal.load_asset(path)
 if m:
  b=m.get_bounding_box();rows.append({'path':path,'min':str(b.min),'max':str(b.max),'materials':[str(x.material_interface.get_path_name()) if x.material_interface else '' for x in m.static_materials]})
Path(unreal.Paths.project_saved_dir()+'Logs/StreetAssets.json').write_text(json.dumps(rows,indent=2))
