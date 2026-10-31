"""Isolated source-model cutouts for the workshop computer; no live Blender or authored model edits."""
from pathlib import Path
r=Path(r'C:/Users/curly/Documents/Foundryline Game')
ns={};exec(compile((r/'Tools/Blender/render_build_catalogue.py').read_text().split('manifest=[]')[0],'catalogue_render_helpers','exec'),ns)
for name,folder,fbx in [('Stored1','PlasmaTable','PlasmaTable_Complete.fbx'),('Stored2','PressBrake','PressBrake_Complete.fbx'),('Stored3','SheetRoller','SheetRoller_Complete.fbx')]:
 ns['clear']();ns['fbx'](Path('SourceAssets/Production/Equipment')/folder/fbx);ns['render'](name)
ns['clear']();wood=ns['mat']('Pallet timber',(.32,.17,.07));steel=ns['mat']('Raw steel',(.28,.31,.34),.75)
for x in [-1.15,0,1.15]:ns['box']('Timber bearer',(x,0,.075),(.11,1.42,.15),wood)
for y in [-.6,-.3,0,.3,.6]:ns['box']('Pallet deck',(0,y,.16),(2.8,.14,.04),wood)
for i in range(3):ns['box']('60 x 120 inch steel sheet',(0,0,.19+i*.013),(3.048,1.524,.003175),steel)
ns['render']('SteelSheets')
