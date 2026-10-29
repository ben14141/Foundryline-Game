from pathlib import Path
import tarfile,json
source=Path(r'C:/Users/curly/OneDrive/Desktop/Product Models/VaultCache/FabLibrary/Megapoly_Art_-_Trucks-4083beb2/unity/megapolyart_trucks.unitypackage')
out=Path('SourceAssets/MegapolySmallTrucks').resolve();out.mkdir(parents=True,exist_ok=True);rows=[]
with tarfile.open(source,'r:gz') as t:
 members={m.name:m for m in t.getmembers()}
 for name,m in members.items():
  if not name.endswith('/pathname'):continue
  path=t.extractfile(m).read().decode('utf8').splitlines()[0].strip('\r\x00')
  if '/Trucks/' not in path:continue
  asset=name.rsplit('/',1)[0]+'/asset'
  if asset not in members or not path.lower().endswith(('.fbx','.prefab','.png','.mat','.pdf')):continue
  rel=Path(path.split('/Trucks/',1)[1]);target=out/rel
  if not target.resolve().is_relative_to(out):raise RuntimeError('Unsafe asset path')
  target.parent.mkdir(parents=True,exist_ok=True)
  if not target.exists():target.write_bytes(t.extractfile(members[asset]).read())
  rows.append({'path':str(rel),'bytes':target.stat().st_size})
(out/'extraction.json').write_text(json.dumps(rows,indent=2));print(json.dumps(rows,indent=2))
