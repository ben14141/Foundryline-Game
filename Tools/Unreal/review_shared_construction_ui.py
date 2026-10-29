"""One rendered client with PIE's background server. No map/save writes."""
import unreal,time
started=time.monotonic();stage=0

def tick(dt):
 global stage
 t=time.monotonic()-started
 if stage==0 and t>8:
  unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1
 if stage==1 and t>18:
  for w in unreal.EditorLevelLibrary.get_pie_worlds(False):
   if unreal.GameplayStatics.get_game_mode(w) is None:
    ps=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController)
    p=next((p for p in ps if p.is_local_controller()),None)
    if p:
     unreal.FoundrylineNetworkTestTools.queue_construction_tool(p,'Building',unreal.IntVector(1000,4000,0),unreal.IntVector(1900,5300,0),1,unreal.LinearColor(.1,.2,.6,1));stage=2
 if stage==2 and t>24:
  for w in unreal.EditorLevelLibrary.get_pie_worlds(False):
   ps=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.PlayerController)
   p=next((p for p in ps if p.is_local_controller()),None)
   if p:unreal.log('SHARED_CLIENT_UI_RESULT error='+str(p.get_last_construction_error())+' replies='+str(p.get_construction_reply_count()))
  unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
