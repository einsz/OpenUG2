#!/usr/bin/env bash
# Ubuntu 22.04+ x86_64 test bundle; runtime libraries remain system packages.
set -euo pipefail
[[ $(uname -s) == Linux && $(uname -m) == x86_64 ]] || {
    echo 'Package this target on Linux x86_64.' >&2; exit 1;
}
root=build/OpenUG2-linux-x86_64
mkdir -p build
mkdir "$root"
cp nfsu2 "$root/openug2"
[[ ! -f nfsu2-vk ]] || cp nfsu2-vk "$root/openug2-vk"
[[ ! -f build/rhi_vk_smoke ]] || cp build/rhi_vk_smoke "$root/"
mkdir "$root/licenses"
cp LICENSE "$root/licenses/OpenUG2.txt"
cp third_party/imgui/LICENSE.txt "$root/licenses/ImGui.txt"
cp docs/TEST_BUILDS.md "$root/README.md"
printf 'revision=%s\nvariant=OpenGL DebugUI\narchitecture=x86_64\n%s\n' \
    "$(git rev-parse HEAD)" "$(getconf GNU_LIBC_VERSION)" > "$root/BUILD.txt"
ldd nfsu2 > "$root/RUNTIME-LIBRARIES.txt"
if grep -q 'not found' "$root/RUNTIME-LIBRARIES.txt"; then
    echo 'A required runtime library is missing.' >&2; exit 1
fi
tar -czf build/OpenUG2-linux-x86_64.tar.gz -C build OpenUG2-linux-x86_64
echo 'Linux package: build/OpenUG2-linux-x86_64.tar.gz'
