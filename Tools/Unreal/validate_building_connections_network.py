"""Four-player section and office joins, plus a fifth late join. No saves."""
import json
import time
import traceback
from pathlib import Path
import unreal

root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
assert (root/'Foundryline.uproject').is_file()
N=unreal.FoundrylineNetworkTestTools
report=root/'Saved/Logs/BuildingConnectionsMultiplayer.json'
state={'stage':0,'checks':{},'errors':[],'completed':False}
started=time.monotonic()

def worlds(): return list(unreal.EditorLevelLibrary.get_pie_worlds(False))
def player(w): return next(p for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller())
def site(w): return next(s for s in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.FoundrylineBuildSite))
def queue(p,t,a,b): N.queue_construction_tool(p,t,unreal.IntVector(*a,0),unreal.IntVector(*b,0),0,unreal.LinearColor(1,1,1,1))
def describe(w,p): return N.describe_construction(player(w),unreal.IntVector(*p,0))
def agree(): return len({site(w).get_property_digest() for w in worlds()})==1
def write(): report.write_text(json.dumps(state,indent=2))
def done():
    state['completed']=True;write()
    unreal.unregister_slate_post_tick_callback(handle)
    unreal.SystemLibrary.execute_console_command(unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world(),'QUIT_EDITOR')

def tick(dt):
    try:
        schedule=[4,14,19,24,29,34,39,45]
        s=state['stage']
        if s>=len(schedule) or time.monotonic()-started<schedule[s]: return
        if s==0: unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
        else:
            ws=worlds();host=next(w for w in ws if unreal.GameplayStatics.get_game_mode(w))
            cs=sorted([w for w in ws if w!=host],key=lambda w:w.get_path_name())
            if s==1:
                state['checks']['four_players']=len(ws)==4
                queue(player(cs[0]),'Building',(1000,4000),(1900,5300))
            elif s==2:
                state['checks']['client_shop_shared']=agree() and all('Building_900_1300' in describe(w,(1400,4500)) for w in ws)
                queue(player(cs[1]),'Shop2',(2000,4100),(2800,5200))
            elif s==3:
                state['checks']['second_client_flush_tier2_join']=agree() and all('Shop2_900_1300 X=1900 Y=4000' in describe(w,(2300,4500)) for w in ws)
                queue(player(cs[2]),'OfficeArea',(100,4100),(900,5200))
            elif s==4:
                state['checks']['third_client_flush_office_join']=agree() and all('Office_900_1300 X=100 Y=4000' in describe(w,(500,4500)) for w in ws)
                state['checks']['all_actions_accepted']=all(player(w).get_last_construction_error()==0 and not player(w).is_construction_pending() for w in ws)
                state['checks']['late_join_requested']=N.request_late_join()
            elif s==5:
                state['checks']['fifth_player_received_connected_sections']=len(ws)==5 and agree() and all('Office_900_1300' in describe(w,(500,4500)) and 'Shop2_900_1300' in describe(w,(2300,4500)) for w in ws)
                state['checks']['direct_client_mutation_denied']=N.client_direct_execution_blocked(player(cs[0]))
                queue(player(cs[1]),'Demolish',(2300,4500),(2300,4500))
            elif s==6:
                state['checks']['disconnect_synced_without_losing_office']=agree() and all(describe(w,(2300,4500))=='EMPTY' and 'Office_900_1300' in describe(w,(500,4500)) for w in ws)
                state['checks']['no_pending_requests']=not any(player(w).is_construction_pending() for w in ws)
            elif s==7: done();return
        state['stage']+=1;write()
    except Exception:
        state['errors'].append(traceback.format_exc());done()

handle=unreal.register_slate_post_tick_callback(tick)
write()
