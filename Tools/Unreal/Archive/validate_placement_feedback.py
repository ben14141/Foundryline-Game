"""Four-player authoritative placement feedback check. No save/map writes."""
import unreal,time,json,traceback
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
if not (root/'Foundryline.uproject').is_file():raise RuntimeError('Foundryline only')
report=root/'Saved/Logs/PlacementFeedbackNetwork.json'
qa={'stage':0,'started':time.monotonic(),'errors':[],'checks':{},'clients':[],'completed':False}
def write():report.write_text(json.dumps({k:v for k,v in qa.items() if k!='started'},indent=2))
def site(w):return next(a for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.Actor) if a.get_class().get_name()=='FoundrylineBuildSite')
def finish():
 qa['completed']=True;unreal.unregister_slate_post_tick_callback(handle);write();unreal.log('FOUNDRYLINE_FEEDBACK_QA_DONE')
def tick(dt):
 try:
  t=time.monotonic()-qa['started']
  if qa['stage']==0 and t>3:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();qa['stage']=1;return
  if qa['stage']==1 and t>12:
   ws=list(unreal.EditorLevelLibrary.get_pie_worlds(False));server=next(w for w in ws if unreal.GameplayStatics.get_game_mode(w));host=site(server)
   built=host.build_rectangle('Building',unreal.IntVector(1000,4000,0),unreal.IntVector(1900,5300,0))
   qa['checks']['host_confirmed_actual_commit']=built and host.get_construction_feedback_title()!='CHANGE APPLIED' and not host.is_construction_feedback_error()
   rejected=not host.build_rectangle('Building',unreal.IntVector(1000,4000,0),unreal.IntVector(1900,5300,0))
   qa['host_rejection']=host.get_construction_feedback_text()
   qa['checks']['host_rejection_explains_overlap']=rejected and host.is_construction_feedback_error() and 'Overlaps' in qa['host_rejection']
   for w in ws:
    if w==server:continue
    s=site(w);before=s.get_property_revision();denied=not s.build_rectangle('Building',unreal.IntVector(3000,4000,0),unreal.IntVector(4000,5000,0))
    qa['clients'].append({'world':w.get_path_name(),'denied':denied,'revision_unchanged':before==s.get_property_revision(),'title':s.get_construction_feedback_title(),'reason':s.get_construction_feedback_text()})
   qa['checks']['all_three_clients_get_authority_reason']=len(qa['clients'])==3 and all(c['denied'] and c['revision_unchanged'] and c['title']=='NOT PLACED' and 'host-controlled' in c['reason'] for c in qa['clients'])
   qa['stage']=2;write()
  if qa['stage']==2 and t>17:
   ws=list(unreal.EditorLevelLibrary.get_pie_worlds(False));digests=[site(w).get_property_digest() for w in ws]
   qa['checks']['accepted_property_matches_four_peers']=len(digests)==4 and len(set(digests))==1;finish()
 except Exception:
  qa['errors'].append(traceback.format_exc());unreal.log_error(qa['errors'][-1]);finish()
handle=unreal.register_slate_post_tick_callback(tick);write()
