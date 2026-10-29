"""Start normal Play without camera-driving or review automation."""
import unreal,time
started=time.monotonic()
def tick(dt):
 if time.monotonic()-started<4:return
 if not unreal.EditorLevelLibrary.get_pie_worlds(False):unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
 unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
