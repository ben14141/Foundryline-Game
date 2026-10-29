"""Create the project's matte surface-paint decal material only."""
import unreal
path='/Game/Foundryline/Production/Materials'
name='M_FactoryFloorDecal'
e=unreal.MaterialEditingLibrary
def connect(a,out,b,pin):
 assert e.connect_material_expressions(a,out,b,pin),str((a.get_name(),out,b.get_name(),pin))
m=unreal.load_asset(path+'/'+name) or unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,path,unreal.Material,unreal.MaterialFactoryNew())
m.set_editor_property('material_domain',unreal.MaterialDomain.MD_DEFERRED_DECAL)
m.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
e.delete_all_material_expressions(m)
t=e.create_material_expression(m,unreal.MaterialExpressionTextureSampleParameter2D,-400,0)
t.set_editor_property('parameter_name','PaintAtlas')
t.set_editor_property('texture',unreal.load_asset('/Engine/EngineResources/WhiteSquareTexture'))
uv=e.create_material_expression(m,unreal.MaterialExpressionTextureCoordinate,-1000,0)
def vector(name,default,x,y):
 v=e.create_material_expression(m,unreal.MaterialExpressionVectorParameter,x,y);v.set_editor_property('parameter_name',name);v.set_editor_property('default_value',unreal.LinearColor(*default));return v
def rg(v,x,y):
 n=e.create_material_expression(m,unreal.MaterialExpressionComponentMask,x,y);n.set_editor_property('r',True);n.set_editor_property('g',True);n.set_editor_property('b',False);n.set_editor_property('a',False);connect(v,'',n,'');return n
scale=rg(vector('UVScale',(1,1,0,0),-1400,200),-1100,200)
offset=rg(vector('UVOffset',(0,0,0,0),-1400,400),-900,400)
mul=e.create_material_expression(m,unreal.MaterialExpressionMultiply,-800,0);connect(uv,'',mul,'A');connect(scale,'',mul,'B')
add=e.create_material_expression(m,unreal.MaterialExpressionAdd,-600,0);connect(mul,'',add,'A');connect(offset,'',add,'B');connect(add,'',t,'UVs')
tint=vector('Tint',(1,1,1,1),-400,-250)
color=e.create_material_expression(m,unreal.MaterialExpressionMultiply,-100,0);connect(t,'RGB',color,'A');connect(tint,'',color,'B')
e.connect_material_property(color,'',unreal.MaterialProperty.MP_BASE_COLOR)
e.connect_material_property(t,'A',unreal.MaterialProperty.MP_OPACITY)
r=e.create_material_expression(m,unreal.MaterialExpressionConstant,-300,240)
r.set_editor_property('r',.85)
e.connect_material_property(r,'',unreal.MaterialProperty.MP_ROUGHNESS)
e.recompile_material(m)
unreal.EditorAssetLibrary.save_loaded_asset(m)
unreal.log('FOUNDRYLINE_FLOOR_PAINT_MATERIAL_READY')
