import bpy,json
from pathlib import Path
r=Path(r'C:/Users/curly/Documents/Foundryline Game/SourceAssets')
paths=['Production/Equipment/WeldingBench/Foundryline_StandardWeldingBench.blend','Production/Equipment/Welder/QualityReview_2026-10-29/Foundryline_MIGWelder_QualityReview.blend','Vehicles/Forklift/QualityReview_2026-10-29/Forklift_QualityReview.blend','Production/AssemblySupply/AssemblySupply_Original.blend','Inventory/Storage/ModularPalletRacking/PalletRack_1Level.blend','Buildings/ManufacturingSign/Foundryline_ManufacturingSign.blend']
for p in paths:
 with bpy.data.libraries.load(str(r/p),link=False) as (a,b):print('ASSET '+p+' '+json.dumps(a.objects))
