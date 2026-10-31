"""Create game-owned, world-centimetre architectural finishes. Never edits vendor packages."""
import unreal,json
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve();E=unreal.MaterialEditingLibrary
folder='/Game/Foundryline/Construction/Materials'
definitions={
 'M_WallCladding':((.8,.22,.04),.44,.08,'cladding'),
 'M_InteriorFinish':((.72,.73,.71),.72,0,'interior'),
 'M_ArchitecturalTrim':((.025,.03,.035),.32,.65,'trim'),
 'M_FoundationConcrete':((.34,.36,.37),.78,0,'concrete')}
report={'materials':[],'world_units':'centimetres','rib_pitch_cm':22.86,'sheet_width_cm':91.44,'floor_joint_cm':600}
for name,(tint,roughness,metallic,style) in definitions.items():
    path=folder+'/'+name;m=unreal.load_asset(path)
    if not m:m=unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,folder,unreal.Material,unreal.MaterialFactoryNew())
    E.delete_all_material_expressions(m)
    m.set_editor_property('tangent_space_normal',False)
    m.set_editor_property('used_with_instanced_static_meshes',True)
    def node(cls,x,y):return E.create_material_expression(m,cls,x,y)
    p=node(unreal.MaterialExpressionWorldPosition,-1000,0)
    n=node(unreal.MaterialExpressionVertexNormalWS,-1000,150)
    c=node(unreal.MaterialExpressionCameraPositionWS,-1000,300)
    t=node(unreal.MaterialExpressionVectorParameter,-1000,450);t.set_editor_property('parameter_name','Tint');t.set_editor_property('default_value',unreal.LinearColor(*tint,1))
    def custom(code,output,x,y):
        value=node(unreal.MaterialExpressionCustom,x,y);value.set_editor_property('code',code);value.set_editor_property('output_type',output)
        inputs=[]
        for key in ['P','N','C','Tint']:
            v=unreal.CustomInput();v.set_editor_property('input_name',key);inputs.append(v)
        value.set_editor_property('inputs',inputs)
        for key,src in [('P',p),('N',n),('C',c),('Tint',t)]:E.connect_material_expressions(src,'',value,key)
        return value
    common='''float fade=1-smoothstep(1800.0,6000.0,distance(C,P));
float along=abs(N.x)>0.5?P.y:P.x;
float phase=along*6.2831853/22.86;
float aa=max(fwidth(along),0.02);
float rib=(0.5+0.5*cos(phase))*fade*(1-abs(N.z));
float seamDist=min(frac(along/91.44),1-frac(along/91.44))*91.44;
float seam=(1-smoothstep(0.25,0.25+aa,seamDist))*fade*(1-abs(N.z));
float grain=sin(P.x*.71+P.y*.37+P.z*.59)*sin(P.x*.29-P.y*.53+P.z*.41);
'''
    if style=='cladding':color=common+'return Tint.rgb*(0.97+rib*.035-seam*.065);'
    elif style=='concrete':color='''float2 q=abs(frac(P.xy/600.0+0.5)-0.5)*600.0;
float aa=max(max(fwidth(P.x),fwidth(P.y)),.04);
float joint=1-smoothstep(.18,.18+aa,min(q.x,q.y));
float nearFade=1-smoothstep(1800.0,6000.0,distance(C,P));
float grain=sin(P.x*.31+P.y*.47+P.z*.23)*sin(P.x*.61-P.y*.33);
return Tint.rgb*(.98+grain*.025*nearFade-joint*.13*abs(N.z));'''
    else:color=common+'return Tint.rgb*(.99+grain*.008*fade);'
    value=custom(color,unreal.CustomMaterialOutputType.CMOT_FLOAT3,-400,0);E.connect_material_property(value,'',unreal.MaterialProperty.MP_BASE_COLOR)
    if style=='cladding':
        normal=custom(common+'''float3 tangent=abs(N.x)>.5?float3(0,1,0):float3(1,0,0);
return normalize(N+tangent*sin(phase)*.13*fade*(1-abs(N.z)));''',unreal.CustomMaterialOutputType.CMOT_FLOAT3,-400,250)
        E.connect_material_property(normal,'',unreal.MaterialProperty.MP_NORMAL)
    else:E.connect_material_property(n,'',unreal.MaterialProperty.MP_NORMAL)
    for prop,amount,y in [(unreal.MaterialProperty.MP_ROUGHNESS,roughness,550),(unreal.MaterialProperty.MP_METALLIC,metallic,650)]:
        value=node(unreal.MaterialExpressionConstant,-400,y);value.set_editor_property('r',amount);E.connect_material_property(value,'',prop)
    E.recompile_material(m)
    if not unreal.EditorAssetLibrary.save_loaded_asset(m):raise RuntimeError('Could not save '+path)
    report['materials'].append(m.get_path_name())
(root/'Saved/Logs/ArchitecturalFinishAssets.json').write_text(json.dumps(report,indent=2))
unreal.log('FOUNDRYLINE_ARCHITECTURAL_FINISHES_READY')
