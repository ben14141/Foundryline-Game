"""Run inside Foundryline's Unreal Python console; does not launch an editor.

Requires the Python Editor Script Plugin. Existing assets are skipped, never replaced.
Example: exec(open(r'C:/Users/curly/Documents/Foundryline Game/Tools/Unreal/import_softline_ui.py').read())
"""
from pathlib import Path
import unreal

project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
if not (project / 'Foundryline.uproject').is_file():
    raise RuntimeError('Open the Foundryline project before importing these sounds.')
source = project / 'SourceAssets/Audio/SoftlineUI'
files = sorted(source.rglob('*.wav'))
if len(files) != 50:
    raise RuntimeError(f'Expected 50 Softline WAVs; found {len(files)}. No imports started.')

tasks = []
for file in files:
    relative = file.relative_to(source)
    folder = '/Game/Foundryline/UI/Audio/Softline/' + relative.parent.as_posix()
    asset_path = folder + '/' + file.stem
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        unreal.log('Skipping existing asset: ' + asset_path)
        continue
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', str(file))
    task.set_editor_property('destination_path', folder)
    task.set_editor_property('destination_name', file.stem)
    task.set_editor_property('automated', True)
    task.set_editor_property('replace_existing', False)
    task.set_editor_property('save', True)
    tasks.append(task)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
failed = [task.get_editor_property('filename') for task in tasks
          if not task.get_editor_property('imported_object_paths')]
if failed:
    raise RuntimeError('Some audio imports failed: ' + ', '.join(failed))
unreal.log(f'Softline: imported {len(tasks)} sounds; skipped {len(files)-len(tasks)} existing assets.')
