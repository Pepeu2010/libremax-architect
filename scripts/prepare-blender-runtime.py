"""Prepare the pinned official Blender distribution, including its own runtime and licenses."""
import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import shutil
import subprocess
import tarfile
import tempfile
import zipfile


def digest(path):
    result = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            result.update(block)
    return result.hexdigest()


def archive_for(entry, cache, supplied=None):
    path = supplied.resolve() if supplied else cache / entry['filename']
    if not path.exists():
        if supplied:
            raise FileNotFoundError(path)
        cache.mkdir(parents=True, exist_ok=True)
        with tempfile.NamedTemporaryFile(dir=cache, prefix='blender-download-', delete=False) as target:
            temporary = Path(target.name)
            try:
                print('Downloading official Blender: ' + entry['url'], flush=True)
                subprocess.run(['curl', '--fail', '--location', '--retry', '3',
                                '--connect-timeout', '30', entry['url']], stdout=target, check=True)
            except BaseException:
                target.close()
                temporary.unlink(missing_ok=True)
                raise
        if digest(temporary) != entry['sha256']:
            temporary.unlink()
            raise ValueError('Official Blender archive checksum mismatch')
        temporary.replace(path)
    if digest(path) != entry['sha256']:
        raise ValueError('Blender archive checksum mismatch: ' + str(path))
    return path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--platform', choices=('windows', 'linux'), default='windows' if os.name == 'nt' else 'linux')
    parser.add_argument('--cache', type=Path, default=Path('build/blender-package/cache'))
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--archive', type=Path, help='Existing official archive; always verify its checksum')
    parser.add_argument('--source-only', action='store_true')
    args = parser.parse_args()
    configuration = json.loads(Path(__file__).with_name('blender-runtime.json').read_text(encoding='utf-8'))
    entry = configuration['source' if args.source_only else args.platform]
    archive = archive_for(entry, args.cache.resolve(), args.archive)
    output = args.output.resolve()
    if args.source_only:
        output.parent.mkdir(parents=True, exist_ok=True)
        if output != archive:
            shutil.copy2(archive, output)
        print('BLENDER_SOURCE_PASS: ' + digest(output), flush=True)
        return
    if output.exists() and any(output.iterdir()):
        raise ValueError('Runtime output must be empty; existing installations are never overwritten')
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='blender-extract-', dir=output.parent) as temporary:
        extraction = Path(temporary)
        if args.platform == 'windows':
            with zipfile.ZipFile(archive) as compressed:
                for member in compressed.infolist():
                    parts = PurePosixPath(member.filename).parts
                    if not parts or parts[0] != entry['root'] or '..' in parts or '\\' in member.filename:
                        raise ValueError('Unsafe Blender archive member')
                compressed.extractall(extraction)
        else:
            with tarfile.open(archive, mode='r:xz') as compressed:
                for member in compressed.getmembers():
                    parts = PurePosixPath(member.name).parts
                    if not parts or parts[0] != entry['root'] or '..' in parts:
                        raise ValueError('Unsafe Blender archive member')
                compressed.extractall(extraction, filter='data')
        root = extraction / entry['root']
        for child in root.iterdir():
            destination = output / child.name
            if not child.resolve().is_relative_to(extraction.resolve()) or not destination.resolve().is_relative_to(output):
                raise ValueError('Runtime move escapes its verified extraction or output directory')
            shutil.move(str(child), destination)
    executable = output / ('blender.exe' if args.platform == 'windows' else 'blender')
    if not executable.is_file() or not (output / 'license/license.md').is_file():
        raise ValueError('Incomplete Blender distribution')
    files = []
    for path in sorted(output.rglob('*')):
        if path.is_file():
            files.append({'path': path.relative_to(output).as_posix(), 'bytes': path.stat().st_size,
                          'sha256': digest(path)})
    manifest = {'version': configuration['version'], 'platform': args.platform, 'archive': entry,
                'source': configuration['source'], 'files': files}
    (output / 'libremax-runtime.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    print(f'BLENDER_RUNTIME_PASS: {configuration["version"]}; {len(files)} files; {output}', flush=True)


if __name__ == '__main__':
    main()
