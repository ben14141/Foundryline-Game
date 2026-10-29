"""Four-client grid-sensitive edits and a fifth late join. No saves."""
import json,time,traceback
from pathlib import Path
import unreal
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
assert (root/'Foundryline.uproject').is_file()
N=unreal.FoundrylineNetworkTestTools
report=root/'Saved/Logs/OfficeAttachmentNetworkQA.json'
q={'stage':0,'checks':{},'errors':[],'completed':False}
started=time.monotonic()
def worlds():return list(unreal.EditorLevelLibrary.get_pie_worlds(False))
def pc(w):return next(p for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller())
def site(w):return next(s for s in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.FoundrylineBuildSite))
def queue(w,t,a,b):return N.queue_construction_tool(pc(w),t,unreal.IntVector(*a,0),unreal.IntVector(*b,0),0,unreal.LinearColor(1,1,1,1))
def desc(w,p):return N.describe_construction(pc(w),unreal.IntVector(*p,0))
def agree():return len({site(w).get_property_digest() for w in worlds()})==1 and not any(pc(w).is_construction_pending() for w in worlds())
def write():report.write_text(json.dumps(q,indent=2))
def done():
 q['completed']=True;write();unreal.unregister_slate_post_tick_callback(handle)
 unreal.SystemLibrary.execute_console_command(unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world(),'QUIT_EDITOR')
def tick(dt):
 try:
  s=q['stage'];schedule=[4,14,19,24,29,34]
  if s>=len(schedule) or time.monotonic()-started<schedule[s]:return
  if s==0:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
  else:
   ws=worlds();h=next(w for w in ws if unreal.GameplayStatics.get_game_mode(w));cs=sorted([w for w in ws if w!=h],key=lambda w:w.get_path_name())
   if s==1:
    q['checks']['four_players']=len(ws)==4
    queue(cs[0],'OfficeArea',(-3500,4000),(-3100,4400))
   elif s==2:
    q['checks']['standalone_office_shared']=agree() and all('Office_400_400' in desc(w,(-3300,4200)) for w in ws)
    queue(cs[1],'Building',(-3500,4400),(-3100,4800))
   elif s==3:
    q['checks']['shop_attachment_accepted_on_all_peers']=agree() and all('Building_400_400' in desc(w,(-3300,4600)) for w in ws)
    q['checks']['late_join_requested']=N.request_late_join()
   elif s==4:
    q['checks']['fifth_player_receives_connection']=len(ws)==5 and agree() and all('Office_400_400' in desc(w,(-3300,4200)) and 'Building_400_400' in desc(w,(-3300,4600)) for w in ws)
   elif s==5:done();return
  q['stage']+=1;write()
 except Exception:q['errors'].append(traceback.format_exc());done()
handle=unreal.register_slate_post_tick_callback(tick);write()
