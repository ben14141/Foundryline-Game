from pathlib import Path
import unreal
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
if not (root/'Foundryline.uproject').is_file():
    raise RuntimeError('Foundryline only')
task=unreal.AssetImportTask()
task.set_editor_property('filename',str(root/'SourceAssets/UI/Brand/Foundryline_Logo_Original.png'))
task.set_editor_property('destination_path','/Game/Foundryline/UI/Brand')
task.set_editor_property('destination_name','T_FoundrylineLogo')
task.set_editor_property('automated',True)
task.set_editor_property('save',True)
task.set_editor_property('replace_existing',False)
path='/Game/Foundryline/UI/Brand/T_FoundrylineLogo'
if not unreal.EditorAssetLibrary.does_asset_exist(path):
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
tex=unreal.load_asset(path)
if not tex:
    raise RuntimeError('Brand board import failed')
tex.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON)
tex.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)
tex.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
unreal.EditorAssetLibrary.save_loaded_asset(tex)
unreal.log('FOUNDRYLINE_BRAND_IMPORTED')
