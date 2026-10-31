"""Game-owned asphalt, concrete kerbs and road paint, authored at real centimetre scale."""
import unreal,json
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve();folder='/Game/Foundryline/Construction/Materials';E=unreal.MaterialEditingLibrary
styles={
 'M_PavementAsphalt':((.032,.036,.041),.86,'asphalt'),
 'M_CurbConcrete':((.43,.445,.43),.84,'curb'),
 'M_RoadPaintWhite':((.83,.85,.8),.48,'paint'),
 'M_RoadPaintYellow':((.96,.56,.035),.48,'paint')}
report={'units':'centimetres','materials':[],'aggregate_cm':.65,'paint_width_cm':10.16,'curb_reveal_cm':15.24}
for name,(tint,roughness,style) in styles.items():
    m=unreal.load_asset(folder+'/'+name)
    if not m:m=unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,folder,unreal.Material,unreal.MaterialFactoryNew())
    E.delete_all_material_expressions(m);m.set_editor_property('tangent_space_normal',False);m.set_editor_property('used_with_instanced_static_meshes',True)
    def node(cls,x,y):return E.create_material_expression(m,cls,x,y)
    inputs={'P':node(unreal.MaterialExpressionWorldPosition,-1000,0),'N':node(unreal.MaterialExpressionVertexNormalWS,-1000,160),'C':node(unreal.MaterialExpressionCameraPositionWS,-1000,320),'Tint':node(unreal.MaterialExpressionVectorParameter,-1000,480)}
    inputs['Tint'].set_editor_property('parameter_name','Tint');inputs['Tint'].set_editor_property('default_value',unreal.LinearColor(*tint,1))
    def custom(code,typ,y):
        v=node(unreal.MaterialExpressionCustom,-400,y);v.set_editor_property('code',code);v.set_editor_property('output_type',typ)
        ports=[]
        for key,src in inputs.items():
            p=unreal.CustomInput();p.set_editor_property('input_name',key);ports.append(p)
        v.set_editor_property('inputs',ports)
        for key,src in inputs.items():E.connect_material_expressions(src,'',v,key)
        return v
    common='''float2 uv=P.xy;
float detail=(1-smoothstep(500.0,2400.0,distance(P,C)))*(1-saturate(max(fwidth(uv.x),fwidth(uv.y))/1.2));
float fine=sin(uv.x*9.666+sin(uv.y*7.12))*sin(uv.y*10.32+sin(uv.x*6.33));
float mid=sin(uv.x*.11+sin(uv.y*.075))*sin(uv.y*.091+sin(uv.x*.17));
float broad=sin(uv.x*.0031+sin(uv.y*.0043))*sin(uv.y*.0027);
'''
    if style=='asphalt':code=common+'return Tint.rgb*(1+fine*.19*detail+mid*.055+broad*.06);'
    elif style=='curb':code=common+'return Tint.rgb*(1+fine*.055*detail+mid*.035);'
    else:code=common+'return Tint.rgb*(.99+fine*.018*detail+mid*.012);'
    value=custom(code,unreal.CustomMaterialOutputType.CMOT_FLOAT3,0);E.connect_material_property(value,'',unreal.MaterialProperty.MP_BASE_COLOR)
    strength=.065 if style=='asphalt' else .025 if style=='curb' else .008
    normal=custom(common+f'return normalize(N+float3(sin(uv.x*9.66),cos(uv.y*10.32),0)*{strength}*detail*abs(N.z));',unreal.CustomMaterialOutputType.CMOT_FLOAT3,260);E.connect_material_property(normal,'',unreal.MaterialProperty.MP_NORMAL)
    v=node(unreal.MaterialExpressionConstant,-400,550);v.set_editor_property('r',roughness);E.connect_material_property(v,'',unreal.MaterialProperty.MP_ROUGHNESS)
    v=node(unreal.MaterialExpressionConstant,-400,650);v.set_editor_property('r',0);E.connect_material_property(v,'',unreal.MaterialProperty.MP_METALLIC)
    E.recompile_material(m)
    if not unreal.EditorAssetLibrary.save_loaded_asset(m):raise RuntimeError('Failed to save '+name)
    report['materials'].append(m.get_path_name())
(root/'Saved/Logs/PavementMaterials.json').write_text(json.dumps(report,indent=2));unreal.log('FOUNDRYLINE_PAVEMENT_MATERIALS_READY');unreal.SystemLibrary.quit_editor()
