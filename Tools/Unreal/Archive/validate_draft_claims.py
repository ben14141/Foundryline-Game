"""Four-player native owning-controller RPC draft contention check. No saves.
Controller pointer-preview ticks are disabled in this fixture; native RPC input
supplies fixed draft endpoints rather than moving a desktop cursor.
"""
import unreal,time,json,traceback
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
if not (root/'Foundryline.uproject').is_file():raise RuntimeError('Foundryline only')
report=root/'Saved/Logs/DraftClaimsNetwork.json'
qa={'started':time.monotonic(),'stage':0,'actions':0,'checks':{},'errors':[],'completed':False}
def write():report.write_text(json.dumps({k:v for k,v in qa.items() if k!='started'},indent=2))
def worlds():return list(unreal.EditorLevelLibrary.get_pie_worlds(False))
def pc(w):return next(a for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if a.is_local_controller())
def site(w):return next(a for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.Actor) if a.get_class().get_name()=='FoundrylineBuildSite')
def queue(p,start=unreal.IntVector(1000,4000,0),end=unreal.IntVector(1900,5300,0),release=False):return unreal.FoundrylineNetworkTestTools.queue_draft(p,'Building',start,end,release)
def finish():
 for w in worlds():pc(w).set_actor_tick_enabled(True)
 qa['completed']=True;unreal.unregister_slate_post_tick_callback(handle);write();unreal.log('FOUNDRYLINE_DRAFT_QA_DONE')
def tick(dt):
 try:
  t=time.monotonic()-qa['started']
  if qa['stage']==0 and t>3:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();qa['stage']=1;return
  times=[12,12.4,12.8,13,13.4,13.6,14,16]
  i=qa['actions']
  if qa['stage'] and i<len(times) and t>times[i]:
   ws=worlds();server=next(w for w in ws if unreal.GameplayStatics.get_game_mode(w));cs=sorted([w for w in ws if w!=server],key=lambda w:w.get_path_name());host=site(server)
   if i==0:
    for w in ws:pc(w).set_actor_tick_enabled(False)
    qa['checks']['four_players']=len(ws)==4;queue(pc(cs[0]));qa['before']=host.get_property_revision()
   elif i==1:queue(pc(cs[1]))
   elif i==2:
    qa['checks']['one_overlapping_claim_wins']=host.get_draft_claim_count()==1
    qa['reason']=pc(cs[1]).get_draft_request_reason();qa['checks']['loser_gets_planning_reason']='Another player' in qa['reason']
    denied=not host.build_rectangle('Building',unreal.IntVector(1000,4000,0),unreal.IntVector(1900,5300,0))
    qa['checks']['commit_cannot_steal_other_draft']=denied and host.get_property_revision()==qa['before'] and 'Another player' in host.get_construction_feedback_text()
   elif i==3:queue(pc(cs[1]),unreal.IntVector(1900,4000,0),unreal.IntVector(2800,5300,0))
   elif i==4:
    qa['checks']['shared_edge_allowed']=host.get_draft_claim_count()==2 and not pc(cs[1]).get_draft_request_reason()
    qa['checks']['claims_replicated_to_all']=all(site(w).get_draft_claim_count()==2 for w in ws)
   elif i==5:queue(pc(cs[0]),release=True)
   elif i==6:qa['checks']['cancel_releases_only_own_claim']=host.get_draft_claim_count()==1
   elif i==7:
    qa['checks']['abandoned_claims_expire']=all(site(w).get_draft_claim_count()==0 for w in ws)
    accepted=host.build_rectangle('Building',unreal.IntVector(1000,4000,0),unreal.IntVector(1900,5300,0))
    qa['checks']['expired_space_can_be_built']=accepted and host.get_construction_feedback_title()!='CHANGE APPLIED'
    qa['checks']['second_commit_cannot_overlap']=not host.build_rectangle('Building',unreal.IntVector(1000,4000,0),unreal.IntVector(1900,5300,0))
   qa['actions']+=1;write()
  if qa['actions']==len(times) and t>18:finish()
 except Exception:
  qa['errors'].append(traceback.format_exc());unreal.log_error(qa['errors'][-1]);finish()
handle=unreal.register_slate_post_tick_callback(tick);write()
