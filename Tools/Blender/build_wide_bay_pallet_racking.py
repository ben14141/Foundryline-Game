"""Original standard rack family with wider beams, no scaling of products or uprights."""
from pathlib import Path
source=Path(__file__).with_name('build_modular_pallet_racking.py')
code=source.read_text(encoding='utf-8')
for old,new in [('ModularPalletRacking','WideBayPalletRacking'),('2.82','4.42'),('2.74','4.34'),('-1.40,1.40','-2.20,2.20'),('-1.45,1.45','-2.25,2.25'),('-1.41','-2.21'),('(1.41','(2.21'),('(-1.45','(-2.25'),('(-1.495','(-2.295'),('[-70,70]','[-110,110]'),('range(20)','range(31)'),('i*.14-1.33','i*.14-2.10')]:
 code=code.replace(old,new)
exec(compile(code,str(source),'exec'),{'__file__':str(source),'__name__':'__main__'})
