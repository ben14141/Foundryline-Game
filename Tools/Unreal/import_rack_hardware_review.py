import unreal,runpy,time
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve()
runpy.run_path(str(root/'Tools/Unreal/import_modular_rack_hardware.py'),run_name='__main__')
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
started=time.monotonic()
def tick(dt):
 if time.monotonic()-started>5:
  unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(None,'QUIT_EDITOR')
handle=unreal.register_slate_post_tick_callback(tick)
