"""Print an asset-file inventory without loading or modifying Unreal packages."""
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FACTORY = ROOT / 'Content/Factory'
files = sorted(p for p in FACTORY.rglob('*') if p.is_file())
print('# Factory LowPoly file inventory\n')
print('Imported package root: `Content/Factory`. Original paths are preserved.\n')
print(f'{len(files)} files; {sum(p.stat().st_size for p in files):,} total bytes.\n')
print('## Package folders\n')
counts = Counter(str(p.parent.relative_to(FACTORY)).replace('\\', '/') for p in files)
for folder, count in sorted(counts.items()):
    print(f'- `{folder}`: {count} files')
print('\n## Map files\n')
maps = [p for p in files if p.suffix.lower() == '.umap']
for path in maps:
    print(f'- `{path.relative_to(ROOT).as_posix()}`')
if not maps:
    print('No .umap files found.')
print('\nFile inventory only: dimensions, pivots, collision, roof separation, dependency')
print('integrity and rendered appearance require later Unreal inspection.')
