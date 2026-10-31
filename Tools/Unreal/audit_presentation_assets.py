"""One editor pass: smooth employees, Softline import, and freight asset checks."""
import json, unreal
from pathlib import Path
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
root=Path(unreal.Paths.project_dir()).resolve()
q={'checks':{},'errors':[]}
try:
    exec((root/'Tools/Unreal/activate_handyman.py').read_text(),{})
    exec((root/'Tools/Unreal/import_softline_ui.py').read_text(),{})
    profile=unreal.load_asset('/Game/Foundryline/Employees/Handyman/DA_HandymanEmployees')
    outfits=profile.get_editor_property('outfits')
    q['checks']['smooth_employee_outfits']=len(outfits)==3 and all('_SE' in p.get_name() for o in outfits for p in o.get_editor_property('parts'))
    q['checks']['smooth_main_mesh']=profile.get_editor_property('mesh').get_name()=='SK_Handyman_modular_SE_'
    sounds=list((root/'Content/Foundryline/UI/Audio/Softline').rglob('*.uasset'))
    q['checks']['softline_50_loaded']=len(sounds)==50 and all(unreal.load_asset('/Game/'+p.relative_to(root/'Content').with_suffix('').as_posix()) is not None for p in sounds)
    meshes=[]
    for name in ['Trailer_Body']+['Trailer_Wheel_'+str(i) for i in range(4)]+['Trailer_Leg_'+str(i) for i in range(2)]:
        m=unreal.load_asset('/Game/Foundryline/Vehicles/FlatbedTrailer/SM_'+name)
        if not m:raise RuntimeError('Missing native flatbed mesh '+name)
        materials=m.get_editor_property('static_materials')
        meshes.append({'mesh':m.get_path_name(),'materials':[str(s.get_editor_property('material_interface')) for s in materials]})
        if not materials or any(s.get_editor_property('material_interface') is None for s in materials):raise RuntimeError('Unassigned flatbed material '+name)
    q['checks']['flatbed_seven_meshes_materials']=len(meshes)==7
    q['flatbed']=meshes
except Exception as e:
    import traceback
    q['errors'].append(traceback.format_exc())
q['completed']=True
(root/'Saved/Logs/PresentationAssetQA.json').write_text(json.dumps(q,indent=2))
if q['errors'] or not all(q['checks'].values()):unreal.log_error(json.dumps(q))
