import unreal
m=unreal.Material()
for cls in [unreal.MaterialExpressionIf,unreal.MaterialExpressionOneMinus,unreal.MaterialExpressionDotProduct]:
 n=unreal.MaterialEditingLibrary.create_material_expression(m,cls)
 unreal.log('FACTORY_PINS '+cls.__name__+' '+str(unreal.MaterialEditingLibrary.get_material_expression_input_names(n)))
