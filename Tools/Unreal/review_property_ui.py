"""Start a normal, playable construction UI review without changing map assets."""
import unreal,time
review_started=time.monotonic()
def start_ui_review(delta):
 if time.monotonic()-review_started<8: return
 unreal.unregister_slate_post_tick_callback(ui_review_handle)
 unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
ui_review_handle=unreal.register_slate_post_tick_callback(start_ui_review)
