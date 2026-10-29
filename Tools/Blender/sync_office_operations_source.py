import bpy,json
from pathlib import Path
r=Path(r'C:/Users/curly/Documents/Foundryline Game');combined=r/'SourceAssets/Operations/Foundryline_Operations.blend';new=r/'SourceAssets/Operations/OfficeWorkstation/Foundryline_OfficeWorkstation.blend'
bpy.ops.wm.open_mainfile(filepath=str(combined));before={o.name:list(o.dimensions) for o in bpy.data.objects if o.name!='OfficeWorkstation'}
old=bpy.data.objects.get('OfficeWorkstation')
if old:bpy.data.objects.remove(old,do_unlink=True)
with bpy.data.libraries.load(str(new)) as (f,t):t.objects=['OfficeWorkstation']
o=t.objects[0];bpy.context.scene.collection.objects.link(o);o.hide_set(True)
after={v.name:list(v.dimensions) for v in bpy.data.objects if v.name!='OfficeWorkstation'}
assert before==after,'Unrelated operations source changed'
bpy.ops.wm.save_as_mainfile(filepath=str(combined));(r/'Saved/Logs/OfficeOperationsSourceSync.json').write_text(json.dumps({'unrelated_objects_unchanged':True,'workstation_has_three_screens':sum(1 for m in o.data.materials if 'OfficeDesktopScreen' in m.name)==3},indent=2))
