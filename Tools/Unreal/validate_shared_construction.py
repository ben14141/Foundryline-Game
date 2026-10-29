"""Native client construction input, server validation and peer convergence.
Four initial players; a fifth late-joins after edits. No saves or map writes.
"""
import unreal,time,json,traceback
from pathlib import Path
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
if not (root/'Foundryline.uproject').is_file():raise RuntimeError('Foundryline only')
report=root/'Saved/Logs/SharedConstructionNetwork.json'
qa={'started':time.monotonic(),'stage':0,'actions':0,'checks':{},'samples':[],'errors':[],'completed':False}
A=unreal.IntVector(1000,4000,0);B=unreal.IntVector(1900,5300,0);point=unreal.IntVector(1400,4500,0)
blue=unreal.LinearColor(.1,.2,.6,1)
def write():report.write_text(json.dumps({k:v for k,v in qa.items() if k!='started'},indent=2))
def worlds():return list(unreal.EditorLevelLibrary.get_pie_worlds(False))
def pc(w):return next(a for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if a.is_local_controller())
def site(w):return next(a for a in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.Actor) if a.get_class().get_name()=='FoundrylineBuildSite')
def setup():
 ws=worlds();server=next(w for w in ws if unreal.GameplayStatics.get_game_mode(w));clients=sorted([w for w in ws if w!=server],key=lambda w:w.get_path_name());return server,clients

def queue(w,tool,start=A,end=B):return unreal.FoundrylineNetworkTestTools.queue_construction_tool(pc(w),tool,start,end,1,blue)
def describe(w,cursor=point):return unreal.FoundrylineNetworkTestTools.describe_construction(pc(w),cursor)
def sample(label):
 ws=worlds();rows=[{'world':w.get_path_name(),'revision':site(w).get_property_revision(),'digest':site(w).get_property_digest(),'replies':pc(w).get_construction_reply_count(),'error':pc(w).get_last_construction_error(),'pending':pc(w).is_construction_pending(),'roofs':pc(w).are_roofs_shown(),'object':describe(w)} for w in ws]
 qa['samples'].append({'label':label,'worlds':rows});qa['checks'][label+'_converged']=len(set(r['digest'] for r in rows))==1 and not any(r['pending'] for r in rows);write()
def accepted(w):return pc(w).get_last_construction_error()==0 and not pc(w).is_construction_pending()
def finish():qa['completed']=True;unreal.unregister_slate_post_tick_callback(handle);write();unreal.log('FOUNDRYLINE_SHARED_CONSTRUCTION_QA_DONE')
def tick(dt):
 try:
  t=time.monotonic()-qa['started']
  if qa['stage']==0 and t>3:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();qa['stage']=1;return
  schedule=[12,15,16,19,20,23,24,27,28,31,32,35,36,39,40,43,44,47,48,51,52,55,56,59,60,63,64,67,68,74]
  i=qa['actions']
  if qa['stage'] and i<len(schedule) and t>schedule[i]:
   host,cs=setup()
   if i==0:
    qa['checks']['client_valid_preview']=unreal.FoundrylineNetworkTestTools.preview_construction(pc(cs[0]),A,B)
    qa['checks']['client_direct_mutation_still_denied']=unreal.FoundrylineNetworkTestTools.client_direct_execution_blocked(pc(cs[0]));queue(cs[0],'Building')
   elif i==1:sample('client1_build');qa['checks']['client1_built']=accepted(cs[0]) and 'Building_900_1300' in describe(host)
   elif i==2:queue(cs[1],'Building',unreal.IntVector(3000,4000,0),unreal.IntVector(4000,5000,0))
   elif i==3:sample('client2_build');qa['checks']['client2_built']=accepted(cs[1]) and 'Building_1000_1000' in describe(host,unreal.IntVector(3500,4500,0))
   elif i==4:queue(cs[2],'Building',unreal.IntVector(4500,3700,0),unreal.IntVector(5400,5000,0))
   elif i==5:sample('client3_build');qa['checks']['client3_built']=accepted(cs[2]) and 'Building_900_1300' in describe(host,unreal.IntVector(4900,4400,0))
   elif i==6:queue(cs[0],'Paint',point,point)
   elif i==7:sample('client_paint');qa['checks']['interior_color_synced']=accepted(cs[0]) and '0.10,0.20,0.60' in describe(host)
   elif i==8:queue(cs[1],'Resize',unreal.IntVector(3900,4900,0),unreal.IntVector(4200,5200,0))
   elif i==9:sample('client_resize');qa['checks']['resize_synced']=accepted(cs[1]) and 'Building_1200_1200' in describe(host,unreal.IntVector(3500,4500,0))
   elif i==10:queue(cs[2],'Move',unreal.IntVector(4900,4400,0),unreal.IntVector(5000,4500,0))
   elif i==11:sample('client_move');qa['checks']['move_synced']=accepted(cs[2]) and 'X=4600 Y=3800' in describe(host,unreal.IntVector(5000,4500,0))
   elif i==12:queue(cs[0],'Demolish',point,point)
   elif i==13:sample('client_remove');qa['checks']['remove_synced']=accepted(cs[0]) and describe(host)=='EMPTY'
   elif i==14:
    qa['checks']['unreserved_drafts_both_valid']=all(unreal.FoundrylineNetworkTestTools.preview_construction(pc(w),A,B) for w in cs[:2]);queue(cs[0],'Building');queue(cs[1],'Building')
   elif i==15:
    sample('simultaneous_same_space');qa['checks']['one_concurrent_winner']=sum(accepted(w) for w in cs[:2])==1 and describe(host)!='EMPTY'
   elif i==16:queue(cs[2],'Foreign',point,point);qa['before_unauthorized']=site(host).get_property_revision()
   elif i==17:sample('other_company_rejected');qa['checks']['other_company_denied']=pc(cs[2]).get_last_construction_error()==8 and site(host).get_property_revision()==qa['before_unauthorized']
   elif i==18:unreal.SystemLibrary.set_timer_for_next_tick(pc(cs[0]),'ToggleWalk');queue(cs[1],'Paint',point,point)
   elif i==19:sample('mixed_views_building');qa['checks']['mixed_view_still_independent']=pc(cs[0]).are_roofs_shown() and not pc(cs[1]).are_roofs_shown() and accepted(cs[1])
   elif i==20:unreal.SystemLibrary.set_timer_for_next_tick(pc(cs[0]),'ToggleWalk')
   elif i==21:sample('returned_topdown');qa['checks']['topdown_restored']=not pc(cs[0]).are_roofs_shown()
   elif i==22:queue(cs[2],'Stale')
   elif i==23:sample('stale_rejected');qa['checks']['stale_denied']=pc(cs[2]).get_last_construction_error()==2
   elif i==24:queue(cs[1],'Building')
   elif i==25:sample('overlap_rejected');qa['checks']['fresh_overlap_denied']=pc(cs[1]).get_last_construction_error()==9
   elif i==26:queue(cs[2],'Unknown')
   elif i==27:sample('unknown_definition_rejected');qa['checks']['unknown_definition_denied']=pc(cs[2]).get_last_construction_error()==4
   elif i==28:qa['checks']['late_join_requested']=unreal.FoundrylineNetworkTestTools.request_late_join()
   elif i==29:sample('late_join');qa['checks']['fifth_player_received_shared_edits']=len(worlds())==5
   qa['actions']+=1;write()
  if qa['actions']==len(schedule) and t>77:finish()
 except Exception:
  qa['errors'].append(traceback.format_exc());unreal.log_error(qa['errors'][-1]);finish()
handle=unreal.register_slate_post_tick_callback(tick);write()
