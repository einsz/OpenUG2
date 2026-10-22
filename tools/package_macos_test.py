#!/usr/bin/env python3
"""Asset-free tests for the macOS packaging dependency graph."""
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

import package_macos as packaging


class DependencyTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name).resolve()
        self.binary = self.make_file('nfsu2')

    def make_file(self, name):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.touch()
        return path

    def graph(self, deps, paths=None):
        with patch.object(packaging, 'dependencies', side_effect=lambda path: deps[path]), \
                patch.object(packaging, 'install_id', side_effect=str), \
                patch.object(packaging, 'rpaths', side_effect=lambda path: (paths or {}).get(path, [])):
            return packaging.dependency_graph(self.binary)

    def test_recursive_dependencies_ignore_system_and_self_ids(self):
        first = self.make_file('lib/libFirst.dylib')
        second = self.make_file('lib/libSecond.dylib')
        graph = self.graph({self.binary: [str(first), '/usr/lib/libSystem.B.dylib'],
                            first: [str(first), '@loader_path/libSecond.dylib'],
                            second: [str(second)]})
        self.assertEqual(graph[self.binary], {str(first): first})
        self.assertEqual(graph[first], {'@loader_path/libSecond.dylib': second})
        self.assertEqual(graph[second], {})

    def test_rpath_can_be_inherited_from_executable(self):
        first = self.make_file('lib/libFirst.dylib')
        second = self.make_file('lib/libSecond.dylib')
        graph = self.graph({self.binary: ['@rpath/libFirst.dylib'],
                            first: ['@rpath/libSecond.dylib'], second: []},
                           {self.binary: ['@executable_path/lib']})
        self.assertEqual(graph[first], {'@rpath/libSecond.dylib': second})

    def test_missing_dependency_fails(self):
        with self.assertRaisesRegex(ValueError, 'Unresolved'):
            self.graph({self.binary: ['@rpath/missing.dylib']})

    def test_ambiguous_rpath_fails(self):
        self.make_file('first/libSame.dylib')
        self.make_file('second/libSame.dylib')
        with self.assertRaisesRegex(ValueError, 'ambiguous'):
            self.graph({self.binary: ['@rpath/libSame.dylib']},
                       {self.binary: ['@loader_path/first', '@loader_path/second']})

    def test_basename_collision_fails(self):
        first = self.make_file('first/libSame.dylib')
        second = self.make_file('second/libSame.dylib')
        with self.assertRaisesRegex(ValueError, 'collision'):
            self.graph({self.binary: [str(first), str(second)]})

    def test_symlink_alias_is_one_library(self):
        library = self.make_file('lib/libReal.dylib')
        alias = self.root / 'lib/libAlias.dylib'
        alias.symlink_to(library)
        graph = self.graph({self.binary: [str(library), str(alias)], library: []})
        self.assertEqual(len(graph), 2)
        self.assertEqual(graph[self.binary][str(alias)], library)

    def test_cycles_terminate(self):
        first = self.make_file('lib/libFirst.dylib')
        second = self.make_file('lib/libSecond.dylib')
        graph = self.graph({self.binary: [str(first)], first: [str(second)], second: [str(first)]})
        self.assertEqual(len(graph), 3)

    def test_formula_license_found_above_lib(self):
        library = self.make_file('formula/lib/libOne.dylib')
        license_path = self.make_file('formula/LICENSE.txt')
        self.assertEqual(packaging.dependency_license(library), license_path)

    def test_missing_license_fails(self):
        with self.assertRaisesRegex(ValueError, 'license'):
            packaging.dependency_license(self.make_file('lib/libOne.dylib'))

    def test_existing_package_is_preserved(self):
        output = self.root / 'output'
        destination = output / 'macos-arm64'
        destination.mkdir(parents=True)
        sentinel = destination / 'keep.txt'
        sentinel.write_text('preserve')
        readme = self.make_file('README.md')
        with patch.object(packaging, 'run', return_value='arm64'), \
                self.assertRaisesRegex(ValueError, 'overwrite'):
            packaging.package(self.binary, output, readme)
        self.assertEqual(sentinel.read_text(), 'preserve')


if __name__ == '__main__':
    unittest.main()
