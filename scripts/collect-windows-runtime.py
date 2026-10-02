"""Bundle the exact MinGW DLL import closure, plugins and dependency notices."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--prefix', type=Path, required=True)
    parser.add_argument('--stage', type=Path, required=True)
    args = parser.parse_args()
    prefix, stage = args.prefix.resolve(), args.stage.resolve()
    binary = stage / 'bin'
    tool = prefix / 'bin/objdump.exe'
    available = {p.name.lower(): p for p in (prefix / 'bin').glob('*.dll')}
    queue = list(binary.rglob('*.dll')) + list(binary.glob('*.exe'))
    visited, bundled, unresolved = set(), {}, set()
    while queue:
        path = queue.pop()
        if path.name.lower() in visited:
            continue
        visited.add(path.name.lower())
        result = subprocess.run([str(tool), '-p', str(path)], capture_output=True, text=True,
                                encoding='utf-8', errors='replace', check=True)
        for name in re.findall(r'DLL Name:\s*(\S+)', result.stdout):
            key = name.lower()
            source = available.get(key)
            if source:
                destination = binary / source.name
                if not destination.exists():
                    shutil.copy2(source, destination)
                bundled[key] = source
                queue.append(destination)
            elif key.startswith(('api-ms-win-', 'ext-ms-win-')):
                continue
            elif not (Path(os.environ['SystemRoot']) / 'System32' / name).exists():
                unresolved.add(name)
    if unresolved:
        raise RuntimeError('Unresolved runtime dependencies: ' + ', '.join(sorted(unresolved)))

    # Map runtime files to the installed MSYS2 package metadata, not guessed versions.
    packages = {}
    for folder in (prefix.parent / 'var/lib/pacman/local').iterdir():
        desc, files = folder / 'desc', folder / 'files'
        if not desc.exists() or not files.exists():
            continue
        fields = {}
        key = None
        for line in desc.read_text(encoding='utf-8').splitlines():
            if line.startswith('%'):
                key = line.strip('%')
                fields[key] = []
            elif line and key:
                fields[key].append(line)
        owned = set(files.read_text(encoding='utf-8').splitlines())
        selected = [p for p in bundled.values()
                    if p.relative_to(prefix.parent).as_posix() in owned]
        if not selected:
            continue
        name = fields['NAME'][0]
        short = re.sub(r'^mingw-w64-(?:ucrt-x86_64|x86_64)-', '', name)
        source_notices = prefix / 'share/licenses' / short
        if source_notices.is_dir():
            shutil.copytree(source_notices, stage / 'share/doc/libremax-architect/dependencies' / short,
                            dirs_exist_ok=True)
        packages[name] = dict(version=fields['VERSION'][0], license=fields.get('LICENSE', []),
                              upstream=fields.get('URL', []),
                              packageSource='https://packages.msys2.org/packages/' + name,
                              files=sorted(p.name for p in selected))
    entries = []
    for path in sorted(binary.rglob('*')):
        if path.is_file():
            entries.append(dict(path=path.relative_to(stage).as_posix(), bytes=path.stat().st_size,
                                sha256=hashlib.sha256(path.read_bytes()).hexdigest()))
    manifest = stage / 'share/doc/libremax-architect/runtime-manifest.json'
    manifest.write_text(json.dumps(dict(packages=packages, files=entries), indent=2), encoding='utf-8')
    print(f'Runtime closure: {len(bundled)} DLLs; {len(packages)} dependency packages', flush=True)


if __name__ == '__main__':
    main()
