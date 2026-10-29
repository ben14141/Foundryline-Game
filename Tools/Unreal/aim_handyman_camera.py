"""Camera-only helper. Call builtins.foundryline_aim_handyman('operator'/'driver'/'welder'/'walking')."""
import unreal,json,builtins
_camera=None
def aim(kind='operator'):
 global _camera
 worlds=unreal.EditorLevelLibrary.get_pie_worlds(False)
 if not worlds:raise RuntimeError('Start Foundryline PIE first')
 world=worlds[0];player=unreal.GameplayStatics.get_player_controller(world,0);found=[]
 for actor in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.Actor):
  for component in actor.get_components_by_class(unreal.FoundrylineEmployeeVisualComponent):
   row=json.loads(component.get_presentation_report())
   if not row.get('skeletal'):continue
   match=row.get('seated') if kind=='driver' else row.get('moving') and not row.get('seated') if kind=='walking' else row.get('activity')=='Welding' if kind=='welder' else row.get('activity')=='Operating'
   if match:found.append((component,row))
 if not found:raise RuntimeError('No actual '+kind+' employee in this PIE world')
 component,row=found[0];at=component.get_world_location();forward=component.get_forward_vector();right=unreal.Vector(-forward.y,forward.x,0);height=173 if row.get('seated') else 128
 eye=at+right*235+forward*75+unreal.Vector(0,0,height+35);target=at+forward*15+unreal.Vector(0,0,height)
 if _camera is None or not unreal.SystemLibrary.is_valid(_camera):_camera=unreal.FoundrylineNetworkTestTools.create_review_camera(player)
 _camera.set_actor_location(eye,False,False);_camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(eye,target),False);_camera.get_components_by_class(unreal.CameraComponent)[0].set_field_of_view(52);player.set_view_target_with_blend(_camera,0)
 return {'employee':row,'camera':_camera.get_path_name()}
builtins.foundryline_aim_handyman=aim
