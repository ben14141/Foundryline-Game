"""Read-only vendor audio export for local analysis; no vendor assets are modified."""
import unreal,json
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve();out=root/'Saved/AudioReview/Manufacturing';out.mkdir(parents=True,exist_ok=True)
rows=[]
for data in unreal.AssetRegistryHelpers.get_asset_registry().get_assets_by_path('/Game/Manufacturing_Ambiences',recursive=True):
 if str(data.asset_class_path.asset_name)!='SoundWave':continue
 wave=data.get_asset();task=unreal.AssetExportTask();task.object=wave;task.filename=str(out/(wave.get_name()+'.wav'));task.automated=True;task.prompt=False;task.replace_identical=True;task.exporter=unreal.SoundExporterWAV()
 assert unreal.Exporter.run_asset_export_task(task),wave.get_name()
 rows.append({'asset':wave.get_path_name(),'file':task.filename,'duration':wave.duration})
(out/'manifest.json').write_text(json.dumps(rows,indent=2));unreal.log('MANUFACTURING_AUDIO_EXPORT_PASSED '+str(len(rows)))
