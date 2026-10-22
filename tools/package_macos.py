#!/usr/bin/env python3
"""Bundle a single-architecture macOS test binary and its private dylibs."""
import argparse
from collections import deque
from pathlib import Path
import shutil
import subprocess
import tarfile
import tempfile


SYSTEM_PREFIXES = ('/System/Library/', '/usr/lib/')


def run(*args):
    return subprocess.check_output(args, text=True).strip()


def dependencies(path):
    return [line.strip().split(' (compatibility version', 1)[0]
            for line in run('otool', '-L', str(path)).splitlines()[1:] if line.strip()]


def install_id(path):
    lines = run('otool', '-D', str(path)).splitlines()
    return lines[1].strip() if len(lines) > 1 else None


def rpaths(path):
    lines = run('otool', '-l', str(path)).splitlines()
    result = []
    for index, line in enumerate(lines):
        if line.strip() == 'cmd LC_RPATH':
            result.append(lines[index + 2].strip().removeprefix('path ').rsplit(' (offset ', 1)[0])
    return result


def expand_path(value, loader, executable):
    for prefix, base in (('@loader_path', loader.parent),
                         ('@executable_path', executable.parent)):
        if value == prefix or value.startswith(prefix + '/'):
            return base / value[len(prefix):].lstrip('/')
    return Path(value) if value.startswith('/') else None


def resolve_dependency(value, loader, executable, search_paths):
    if value.startswith(SYSTEM_PREFIXES):
        return None
    if value.startswith('@rpath/'):
        candidates = []
        for owner, item in search_paths:
            base = expand_path(item, owner, executable)
            if base is not None:
                candidates.append(base / value[len('@rpath/'):])
    else:
        candidate = expand_path(value, loader, executable)
        candidates = [candidate] if candidate is not None else []
    matches = {candidate.resolve() for candidate in candidates if candidate.is_file()}
    if len(matches) != 1:
        raise ValueError(f'Unresolved or ambiguous dependency {value!r} in {loader}')
    return matches.pop()


def dependency_graph(executable):
    graph = {}
    names = {}
    root_rpaths = [(executable, item) for item in rpaths(executable)]
    pending = deque([(executable, root_rpaths)])
    while pending:
        source, inherited = pending.popleft()
        if source in graph:
            continue
        own_paths = [(source, item) for item in rpaths(source)]
        search_paths = own_paths + inherited
        identifier = install_id(source) if source != executable else None
        edges = {}
        for value in dependencies(source):
            if value == identifier:
                continue
            target = resolve_dependency(value, source, executable, search_paths)
            if target is None:
                continue
            if target.name in names and names[target.name] != target:
                raise ValueError(f'Dylib basename collision: {names[target.name]} and {target}')
            names[target.name] = target
            edges[value] = target
            pending.append((target, search_paths))
        graph[source] = edges
    return graph


def dependency_license(source):
    # Homebrew stores formula licenses beside lib/, including in Cellar paths.
    for parent in source.parents:
        for filename in ('LICENSE', 'LICENSE.txt', 'COPYING', 'COPYING.txt'):
            candidate = parent / filename
            if candidate.is_file():
                return candidate
    raise ValueError(f'No dependency license found for {source}')


def package(binary, output, readme, revision=None):
    binary = binary.resolve(strict=True)
    readme = readme.resolve(strict=True)
    architectures = run('lipo', '-archs', str(binary)).split()
    if len(architectures) != 1 or architectures[0] not in ('arm64', 'x86_64'):
        raise ValueError(f'Expected arm64 or x86_64 binary, got {architectures}')
    arch = architectures[0]
    destination = output / f'macos-{arch}'
    archive = output / f'OpenUG2-macos-{arch}.tar.gz'
    if destination.exists() or archive.exists():
        raise ValueError(f'Refusing to overwrite existing package: {destination} or {archive}')
    graph = dependency_graph(binary)
    for source in graph:
        if source != binary and arch not in run('lipo', '-archs', str(source)).split():
            raise ValueError(f'Dependency does not support {arch}: {source}')
    revision = revision or run('git', 'rev-parse', 'HEAD')
    project = Path(__file__).resolve().parent.parent
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='.macos-package-', dir=output) as temporary:
        bundle = Path(temporary) / destination.name
        (bundle / 'lib').mkdir(parents=True)
        (bundle / 'licenses').mkdir()
        copies = {source: bundle / ('openug2' if source == binary else 'lib/' + source.name)
                  for source in graph}
        for source, target in copies.items():
            shutil.copy2(source, target)
            target.chmod(target.stat().st_mode | 0o200)
            if source != binary:
                license_path = dependency_license(source)
                shutil.copy2(license_path, bundle / 'licenses' / (source.name + '.LICENSE.txt'))
        shutil.copy2(project / 'LICENSE', bundle / 'LICENSE')
        shutil.copy2(project / 'third_party/imgui/LICENSE.txt', bundle / 'licenses/imgui.LICENSE.txt')
        shutil.copy2(readme, bundle / 'README.md')
        (bundle / 'BUILD.txt').write_text(f'OpenUG2 macOS test build\nRevision: {revision}\nArchitecture: {arch}\n', encoding='utf-8')
        for source, target in copies.items():
            for old, dependency in graph[source].items():
                new = '@loader_path/' + ('lib/' if source == binary else '') + dependency.name
                run('install_name_tool', '-change', old, new, str(target))
            if source != binary:
                run('install_name_tool', '-id', '@loader_path/' + source.name, str(target))
            for item in dict.fromkeys(rpaths(target)):
                run('install_name_tool', '-delete_rpath', item, str(target))
            for item in dependencies(target):
                if not item.startswith(SYSTEM_PREFIXES + ('@loader_path/',)):
                    raise ValueError(f'External dependency remains in {target}: {item}')
        for source, target in copies.items():
            if source != binary:
                run('codesign', '--force', '--sign', '-', str(target))
        run('codesign', '--force', '--sign', '-', str(copies[binary]))
        for target in copies.values():
            run('codesign', '--verify', '--strict', str(target))
        temporary_archive = Path(temporary) / archive.name
        with tarfile.open(temporary_archive, 'w:gz') as tar:
            tar.add(bundle, arcname=bundle.name)
        bundle.rename(destination)
        temporary_archive.rename(archive)
    return archive


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, default=Path('nfsu2'))
    parser.add_argument('--output', type=Path, default=Path('build'))
    parser.add_argument('--readme', type=Path, default=Path('docs/TEST_BUILDS.md'))
    parser.add_argument('--revision')
    args = parser.parse_args()
    try:
        archive = package(args.binary, args.output, args.readme, args.revision)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        parser.exit(1, f'Packaging failed: {error}\n')
    print(archive)


if __name__ == '__main__':
    main()
