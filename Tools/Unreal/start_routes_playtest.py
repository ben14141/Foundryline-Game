"""Open solo Play on the Routes tab, then release all control to the player."""
import time
import unreal

started = time.monotonic()
requested = False

def tick(dt):
    global requested
    if time.monotonic() - started < 4:
        return
    worlds = unreal.EditorLevelLibrary.get_pie_worlds(False)
    if not worlds:
        if not requested:
            unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
            requested = True
        return
    for world in worlds:
        for player in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.FoundrylinePlayerController):
            if player.is_local_controller():
                unreal.FoundrylineNetworkTestTools.queue_logistics_action(player, 'NetPanel', unreal.Vector(), False)
                unreal.unregister_slate_post_tick_callback(handle)
                return

handle = unreal.register_slate_post_tick_callback(tick)
