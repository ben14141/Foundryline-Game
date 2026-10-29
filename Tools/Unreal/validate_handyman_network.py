"""Five-player job/ownership/late-join regression with read-only character appearance comparison."""
import unreal,json
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
scope={'__file__':str(root/'Tools/Unreal/validate_employee_jobs_network.py')}
exec(compile(Path(scope['__file__']).read_text(),scope['__file__'],'exec'),scope)
original_finish=scope['finish']
def finish_with_appearance():
 q=scope['q'];q['handyman_visuals']=[];peers=[]
 for world in scope['worlds']():
  reports=[]
  for plant in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.FoundrylineFactoryPlant):
   for component in plant.get_components_by_class(unreal.FoundrylineEmployeeVisualComponent):
    reports.append(json.loads(component.get_presentation_report()))
  q['handyman_visuals'].append({'world':world.get_path_name(),'reports':reports})
  peers.append({r.get('key'):r for r in reports})
 all_reports=[r for peer in peers for r in peer.values()]
 q['checks']['handyman_models_loaded_all_listen_and_client_peers']=bool(all_reports) and all(r.get('skeletal') and r.get('mesh','').startswith('/Engine/Transient.FoundrylineWorker_') and r.get('profile','').startswith('/Game/Foundryline/') for r in all_reports)
 appearances={}
 for r in all_reports:appearances.setdefault(r['key'],set()).add((r.get('variant'),r.get('profile'),tuple(r.get('outfit',[]))))
 q['checks']['same_employee_has_same_model_on_all_peers_and_late_join']=bool(appearances) and all(len(v)==1 for v in appearances.values())
 q['checks']['visible_variants_stay_in_authored_range']=bool(all_reports) and all(0<=r.get('variant',-1)<=2 for r in all_reports)
 q['checks']['no_duplicate_employee_components_any_peer']=all(len(p)==len(w['reports']) for p,w in zip(peers,q['handyman_visuals']))
 original_finish()
 (root/'Saved/Logs/HandymanNetworkQA.json').write_text(json.dumps(scope['q'],indent=2))
scope['finish']=finish_with_appearance
