"""Read an isolated QA checkpoint through the normal loader; never resave it."""
import unreal,json,time,traceback
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve();out=root/'Saved/Logs/ProgressionTimingContractTerms.json'
N=unreal.FoundrylineNetworkTestTools;L=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
L.load_level('/Game/Foundryline/Maps/L_FirstShopStarter');L.editor_request_begin_play()
unreal.EditorPythonScripting.set_keep_python_script_alive(True);started=time.monotonic()
def tick(dt):
    if time.monotonic()-started<12:return
    try:
        host=next(p for w in unreal.EditorLevelLibrary.get_pie_worlds(False) if unreal.GameplayStatics.get_game_mode(w) for p in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController) if p.is_local_controller())
        assert N.test_factory_checkpoint(host,'Foundryline_QA_ProgressionPlayed',True),'Checkpoint restore failed'
        result=json.loads(N.commerce_report(host));result['source_slot']='Foundryline_QA_ProgressionPlayed';result['completed']=True;result['checks']={'restored':True};result['errors']=[]
    except Exception:result={'completed':False,'errors':[traceback.format_exc()],'checks':{}}
    out.write_text(json.dumps(result,indent=2));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(None,'QUIT_EDITOR')
handle=unreal.register_slate_post_tick_callback(tick)
