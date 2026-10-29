import unreal,json
from pathlib import Path
rows=[]
for w in unreal.EditorLevelLibrary.get_pie_worlds(False):
 for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.FoundrylineWeldingStation):
  cs=a.get_components_by_class(unreal.StaticMeshComponent);gun=next(c for c in cs if c.get_name()=='MIGGun');bounds=gun.static_mesh.get_bounds()
  rows.append({'active':a.welding_effect.is_welding(),'spark_count':a.welding_effect.get_visible_spark_count(),'socket_exists':gun.does_socket_exist('WeldTip'),'tip_world':str(a.welding_effect.get_world_location()),'gun_bounds':str(bounds),'gun_transform':str(gun.get_world_transform()),'mesh_components':[(c.get_name(),str(c.get_world_location())) for c in cs]})
Path(unreal.Paths.project_saved_dir()+'Logs/WeldingVisualInspection.json').write_text(json.dumps(rows,indent=2))
