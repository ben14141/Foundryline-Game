"""Create the separate starter map, preserving the existing construction level."""
import unreal,json
from pathlib import Path
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
path='/Game/Foundryline/Maps/L_CutPartsStarter'
if unreal.EditorAssetLibrary.does_asset_exist(path):
 levels.load_level(path)
else:
 if not levels.load_level('/Game/Foundryline/Maps/L_ConstructionSite'):raise RuntimeError('Could not load source environment')
 if not unreal.EditorLoadingAndSavingUtils.save_map(unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world(),path):raise RuntimeError('Could not save separate starter map')
if not levels.load_level(path):raise RuntimeError('Could not open saved starter copy')
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property('default_game_mode',unreal.load_class(None,'/Script/Foundryline.FoundrylineCutPartsGameMode'))
if not levels.save_current_level():raise RuntimeError('Could not save starter game mode')
Path(unreal.Paths.project_saved_dir(),'Logs/CutPartsMapCreated.json').write_text(json.dumps({'map':path,'game_mode':'FoundrylineCutPartsGameMode','saved':True},indent=2))
unreal.SystemLibrary.execute_console_command(world,'QUIT_EDITOR')
