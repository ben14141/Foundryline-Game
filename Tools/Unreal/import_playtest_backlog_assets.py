"""Import scoped game assets, without loading or saving any map."""
import unreal,runpy,json,time,traceback
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve()
report={'imports':[],'errors':[]};started=time.monotonic()
try:
 for name in ['import_plasma_rail_supports.py','import_personnel_doors.py','import_starter_hauler.py']:
  runpy.run_path(str(root/'Tools/Unreal'/name),run_name='__main__');report['imports'].append(name)
except Exception:
 report['errors'].append(traceback.format_exc())
report['seconds']=time.monotonic()-started
(root/'Saved/Logs/PlaytestBacklogAssetImport.json').write_text(json.dumps(report,indent=2))
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
finished=time.monotonic()
def done(dt):
 if time.monotonic()-finished>5:
  unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.execute_console_command(None,'QUIT_EDITOR')
handle=unreal.register_slate_post_tick_callback(done)
