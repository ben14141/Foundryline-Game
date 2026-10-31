"""Scoped icon import followed by an unsaved native ownership/UI review."""
from pathlib import Path
import unreal
root=Path(unreal.Paths.project_dir()).resolve()
for name in ['import_machine_catalogue.py','review_owned_equipment_ui.py']:
 p=root/'Tools/Unreal'/name
 exec(compile(p.read_text(encoding='utf-8'),str(p),'exec'),globals())
