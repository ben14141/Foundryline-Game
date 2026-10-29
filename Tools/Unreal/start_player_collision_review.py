"""Start a fresh transient starter PIE and run the physical walkthrough."""
import unreal,runpy,builtins
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
builtins.foundryline_collision_chain_tree_review=True
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
runpy.run_path(str(root/'Tools/Unreal/review_player_collision.py'))
