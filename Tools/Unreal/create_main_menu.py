"""Create the isolated title map and import its reference artwork; no gameplay maps changed."""
import unreal
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve()
source=root/'SourceAssets/UI/MainMenu/MainMenuReference.png'
if source.exists():
    task=unreal.AssetImportTask()
    task.filename=str(source)
    task.destination_path='/Game/Foundryline/UI/MainMenu'
    task.destination_name='T_MainMenuReference'
    task.automated=True
    task.replace_existing=True
    task.save=True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture=unreal.load_asset('/Game/Foundryline/UI/MainMenu/T_MainMenuReference')
    texture.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)
    unreal.EditorAssetLibrary.save_loaded_asset(texture)
# The supplied white/orange logo already has real alpha; preserve it directly.
logo=unreal.AssetImportTask()
logo.filename=str(root/'SourceAssets/UI/MainMenu/Foundryline_Menu_Logo.png')
logo.destination_path='/Game/Foundryline/UI/MainMenu'
logo.destination_name='T_MainMenuLogo'
logo.automated=True
logo.replace_existing=True
logo.save=True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([logo])
texture=unreal.load_asset('/Game/Foundryline/UI/MainMenu/T_MainMenuLogo')
if not texture:raise RuntimeError('Menu logo failed to import')
texture.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)
texture.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON)
texture.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
unreal.EditorAssetLibrary.save_loaded_asset(texture)
folder='/Game/Foundryline/UI/MainMenu'
frost=unreal.load_asset(folder+'/M_MenuFrost') if unreal.EditorAssetLibrary.does_asset_exist(folder+'/M_MenuFrost') else None
if not frost:frost=unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_MenuFrost',folder,unreal.Material,unreal.MaterialFactoryNew())
frost.set_editor_property('material_domain',unreal.MaterialDomain.MD_SURFACE)
frost.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
frost.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
unreal.MaterialEditingLibrary.delete_all_material_expressions(frost)
def node(kind):return unreal.MaterialEditingLibrary.create_material_expression(frost,kind)
def wire(a,port,b,target):
    # The material editor shortens generic Input to an empty pin and Coordinates to UVs.
    alias={'Input':'','Coordinates':'UVs'}
    connected=unreal.MaterialEditingLibrary.connect_material_expressions(a,port,b,alias.get(target,target))
    if not connected:raise RuntimeError('Material connection failed: '+str(a.get_class().get_name())+'.'+port+' -> '+str(b.get_class().get_name())+'.'+target)
def scalar(value):
    n=node(unreal.MaterialExpressionConstant);n.set_editor_property('r',value);return n
def vector(name,value):
    n=node(unreal.MaterialExpressionVectorParameter);n.set_editor_property('parameter_name',name);n.set_editor_property('default_value',unreal.LinearColor(*value));return n
def rg(n):
    mask=node(unreal.MaterialExpressionComponentMask)
    mask.set_editor_property('r',True);mask.set_editor_property('g',True);mask.set_editor_property('b',False);mask.set_editor_property('a',False)
    wire(n,'',mask,'Input');return mask
coord=node(unreal.MaterialExpressionTextureCoordinate)
scale=node(unreal.MaterialExpressionMultiply);wire(coord,'',scale,'A');wire(rg(vector('UVScale',(1,1,0,0))),'',scale,'B')
origin=node(unreal.MaterialExpressionAdd);wire(scale,'',origin,'A');wire(rg(vector('UVOrigin',(0,0,0,0))),'',origin,'B')
total=None
for x,y in [(-1,-1),(0,-1),(1,-1),(-1,0),(0,0),(1,0),(-1,1),(0,1),(1,1)]:
    offset=node(unreal.MaterialExpressionConstant2Vector);offset.set_editor_property('r',x*.0035);offset.set_editor_property('g',y*.0035)
    uv=node(unreal.MaterialExpressionAdd);wire(origin,'',uv,'A');wire(offset,'',uv,'B')
    sample=node(unreal.MaterialExpressionTextureSample);sample.texture=unreal.load_asset(folder+'/T_MainMenuReference');wire(uv,'',sample,'Coordinates')
    if total:
        add=node(unreal.MaterialExpressionAdd);wire(total,'RGB' if isinstance(total,unreal.MaterialExpressionTextureSample) else '',add,'A');wire(sample,'RGB',add,'B');total=add
    else:total=sample
average=node(unreal.MaterialExpressionMultiply);wire(total,'',average,'A');wire(scalar(1/9),'',average,'B')
desat=node(unreal.MaterialExpressionDesaturation);wire(average,'',desat,'Input');wire(scalar(.65),'',desat,'Fraction')
shade=node(unreal.MaterialExpressionMultiply);wire(desat,'',shade,'A');wire(scalar(.42),'',shade,'B')
tint=node(unreal.MaterialExpressionAdd);wire(shade,'',tint,'A');wire(scalar(.055),'',tint,'B')
unreal.MaterialEditingLibrary.connect_material_property(tint,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
reveal=node(unreal.MaterialExpressionScalarParameter);reveal.set_editor_property('parameter_name','Reveal');reveal.set_editor_property('default_value',1.)
alpha=node(unreal.MaterialExpressionMultiply);wire(reveal,'',alpha,'A');wire(scalar(.72),'',alpha,'B')
unreal.MaterialEditingLibrary.connect_material_property(alpha,'',unreal.MaterialProperty.MP_OPACITY)
unreal.MaterialEditingLibrary.recompile_material(frost);unreal.EditorAssetLibrary.save_loaded_asset(frost)
world=unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
mode=unreal.load_class(None,'/Script/Foundryline.FoundrylineMainMenuMode')
if not mode: raise RuntimeError('Compile the main-menu classes first')
world.get_world_settings().set_editor_property('default_game_mode',mode)
if not unreal.EditorLoadingAndSavingUtils.save_map(world,'/Game/Foundryline/Maps/L_MainMenu'):
    raise RuntimeError('Could not save main menu map')
unreal.log('FOUNDRYLINE_MAIN_MENU_READY')
