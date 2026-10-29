"""Read-only inventory of visible physical geometry in the active PIE world."""
import unreal,json
from pathlib import Path
world=unreal.EditorLevelLibrary.get_pie_worlds(False)[0]
rows=[]
for actor in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.Actor):
 for component in actor.get_components_by_class(unreal.PrimitiveComponent):
  try:
   mesh=component.get_editor_property('static_mesh') if isinstance(component,unreal.StaticMeshComponent) else None
   center,extent,_=unreal.SystemLibrary.get_component_bounds(component)
   rows.append({'actor':actor.get_name(),'class':actor.get_class().get_name(),'component':component.get_name(),'mesh':mesh.get_name() if mesh else '', 'center':[center.x,center.y,center.z],'extent':[extent.x,extent.y,extent.z],'collision':str(component.get_collision_enabled()),'pawn':str(component.get_collision_response_to_channel(unreal.CollisionChannel.ECC_PAWN)),'tags':[str(t) for t in component.component_tags]})
  except Exception as error:rows.append({'actor':actor.get_name(),'component':component.get_name(),'error':str(error)})
Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()),'Logs/WalkCollisionInventory.json').write_text(json.dumps(rows,indent=2))
