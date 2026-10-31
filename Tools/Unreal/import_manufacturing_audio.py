"""Import calibrated private derivatives, preserving all purchased source assets."""
import unreal,json
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve();source=root/'SourceAssets/Audio/ManufacturingDerived';dest='/Game/Foundryline/Audio/Manufacturing';tools=unreal.AssetToolsHelpers.get_asset_tools()
meta=json.loads((source/'manifest.json').read_text())
submix=unreal.load_asset(dest+'/SM_Manufacturing') or tools.create_asset('SM_Manufacturing',dest,unreal.SoundSubmix,unreal.SoundSubmixFactory())
unreal.EditorAssetLibrary.save_loaded_asset(submix)
category=unreal.load_asset(dest+'/SC_Manufacturing') or tools.create_asset('SC_Manufacturing',dest,unreal.SoundClass,unreal.SoundClassFactory())
props=category.get_editor_property('properties');props.set_editor_property('volume',.8);props.set_editor_property('apply_effects',False);props.set_editor_property('default_submix',submix);category.set_editor_property('properties',props);unreal.EditorAssetLibrary.save_loaded_asset(category)
report=[]
for profile in meta['profiles']:
 name='SW_'+profile['role'];task=unreal.AssetImportTask();task.filename=str(source/(profile['role']+'.wav'));task.destination_path=dest+'/Loops';task.destination_name=name;task.automated=True;task.replace_existing=True;task.save=True
 tools.import_asset_tasks([task]);wave=unreal.load_asset(dest+'/Loops/'+name);assert wave,name
 wave.set_editor_property('looping',profile['loop']);wave.set_editor_property('volume',1.0);wave.set_editor_property('sound_class_object',category);wave.set_editor_property('loading_behavior',unreal.SoundWaveLoadingBehavior.RETAIN_ON_LOAD)
 assert unreal.EditorAssetLibrary.save_loaded_asset(wave)
 report.append({'role':profile['role'],'asset':wave.get_path_name(),'duration':wave.duration,'loop':wave.get_editor_property('looping')})
(root/'Saved/Logs/ManufacturingAudioImport.json').write_text(json.dumps(report,indent=2));unreal.log('MANUFACTURING_AUDIO_IMPORT_PASSED '+str(len(report)))
