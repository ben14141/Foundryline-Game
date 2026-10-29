import unreal,json
from pathlib import Path
m=unreal.load_asset('/Game/Foundryline/Production/Materials/M_FactoryFloorDecal');e=unreal.MaterialEditingLibrary
rows=[]
for n in e.get_material_expressions(m):
 row={'name':n.get_name(),'class':n.get_class().get_name(),'input_names':list(e.get_material_expression_input_names(n)),'inputs':[x.get_name() if x else None for x in e.get_inputs_for_material_expression(m,n)]}
 try:row['param']=str(n.get_editor_property('parameter_name'))
 except Exception:pass
 rows.append(row)
(Path(unreal.Paths.project_saved_dir())/'Logs/FloorPaintMaterialGraph.json').write_text(json.dumps(rows,indent=2))
