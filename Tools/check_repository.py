"""Read-only layout/include checks. This is NOT a C++ compiler or Unreal test runner."""
from pathlib import Path
import json
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
DOMAINS = ('Core Grid Construction Buildings Zones Pathing Employees Production '
           'Logistics Inventory Orders Economy Management Companies Competition '
           'World UI SaveSystem Networking').split()


def main():
    errors = []
    module = ROOT / 'Source/Foundryline'
    descriptor = json.loads((ROOT / 'Foundryline.uproject').read_text(encoding='utf-8-sig'))
    if not any(m['Name'] == 'Foundryline' for m in descriptor.get('Modules', [])):
        errors.append('Missing Foundryline runtime module')
    for domain in DOMAINS:
        for folder in (module / 'Public' / domain, module / 'Private' / domain,
                       ROOT / 'Content/Foundryline' / domain):
            if not folder.is_dir():
                errors.append(f'Missing system folder: {folder.relative_to(ROOT)}')
    checked = 0
    for path in sorted(module.rglob('*')):
        if path.suffix not in ('.h', '.cpp'):
            continue
        checked += 1
        relative = path.relative_to(module)
        expected = 'Public' if path.suffix == '.h' else 'Private'
        if relative.parts[0] != expected:
            errors.append(f'Incorrect visibility folder: {relative}')
        text = path.read_text()
        includes = re.findall(r'^#include\s+"([^"]+)"', text, re.M)
        generated = [i for i in includes if i.endswith('.generated.h')]
        if generated and (includes[-1] != generated[-1] or
                          generated != [path.stem + '.generated.h']):
            errors.append(f'Generated include ordering/name: {relative}')
        if any(token in text for token in ('UCLASS(', 'USTRUCT(', 'UENUM(')) and not generated:
            errors.append(f'Reflected header missing generated include: {relative}')
        for include in includes:
            if 'Foundryline' in include and not include.endswith('.generated.h'):
                if not (module / 'Public' / include).is_file():
                    errors.append(f'Unresolved local include in {relative}: {include}')
    input_config = (ROOT / 'Config/DefaultInput.ini').read_text() if (ROOT / 'Config/DefaultInput.ini').exists() else ''
    axis_names = set(re.findall(r'AxisName="([^"]+)"', input_config))
    action_names = set(re.findall(r'ActionName="([^"]+)"', input_config))
    for path in module.rglob('*.cpp'):
        text = path.read_text()
        for kind, name in re.findall(r'Bind(Axis|Action)\(TEXT\("([^"]+)"\)', text):
            if name not in (axis_names if kind == 'Axis' else action_names):
                errors.append(f'Missing {kind} mapping: {name} ({path.name})')
    for error in errors:
        print('ERROR:', error)
    print(f'{len(DOMAINS)} domains; {checked} C++ files inspected; {len(errors)} layout/include errors.')
    print('This checker does not run C++ compilation, UHT or Unreal automation tests; see the separate build report.')
    return bool(errors)


if __name__ == '__main__':
    sys.exit(main())
