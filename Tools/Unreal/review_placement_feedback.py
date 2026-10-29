"""Bounded visual review of a rejected placement. No map/save writes."""
import unreal,time
started=time.monotonic();stage=0;last=0

def tick(dt):
 global stage,last
 t=time.monotonic()-started
 if stage==0 and t>8:
  unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play();stage=1
 if stage==1 and 15<t<60 and t-last>1:
  for w in unreal.EditorLevelLibrary.get_pie_worlds(False):
   for s in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.Actor):
    if s.get_class().get_name()=='FoundrylineBuildSite':s.build_rectangle('Building',unreal.IntVector(-2800,1700,0),unreal.IntVector(-400,3500,0))
  last=t
 if t>60:unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
