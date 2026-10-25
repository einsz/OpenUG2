#!/usr/bin/env bash
# Package built UCRT64 executables and their imported runtime DLLs; no assets.
set -euo pipefail
if [[ ${MSYSTEM:-} != UCRT64 || -z ${MINGW_PREFIX:-} ]]; then
    echo 'Run this script inside MSYS2 UCRT64.' >&2; exit 1
fi
stage=${1:-build/windows-ucrt64}
# Refuse to mix a previous package with this build.
mkdir "$stage"
queue=(nfsu2.exe nfsu2-vk.exe build/rhi_vk_smoke.exe)
for file in "${queue[@]}"; do cp "$file" "$stage/"; done
system=$(cygpath -u "${WINDIR:?}/System32")
for ((i=0; i<${#queue[@]}; i++)); do
    imports=$(objdump -p "${queue[i]}" | awk '$1 == "DLL" && $2 == "Name:" { sub(/\r$/, "", $3); print $3 }')
    [[ -n $imports ]] || continue
    while IFS= read -r dll; do
        dll=${dll%$'\r'}
        [[ $dll =~ ^[a-zA-Z0-9_+.-]+\.[dD][lL][lL]$ ]] || {
            printf 'Unexpected DLL import name from %s: %q\n' "${queue[i]}" "$dll" >&2; exit 1;
        }
        [[ -f "$stage/$dll" ]] && continue
        source=$(find "$MINGW_PREFIX/bin" -maxdepth 1 -iname "$dll" -type f -print -quit)
        if [[ -n $source ]]; then
            cp "$source" "$stage/$dll"
            queue+=("$stage/$dll")
        elif [[ ! -f "$system/$dll" && $dll != api-ms-win-* && $dll != ext-ms-win-* ]]; then
            echo "Unresolved runtime DLL: $dll" >&2; exit 1
        fi
    done <<< "$imports"
done
mv "$stage/nfsu2.exe" "$stage/openug2.exe"
mv "$stage/nfsu2-vk.exe" "$stage/openug2-vk.exe"
mkdir "$stage/licenses"
cp -R "$MINGW_PREFIX/share/licenses/." "$stage/licenses/"
cp third_party/imgui/LICENSE.txt "$stage/licenses/ImGui.txt"
cp LICENSE "$stage/licenses/OpenUG2.txt"
cp docs/WINDOWS.md "$stage/BUILD-AND-RUN.md"
printf 'revision=%s\nvariant=DebugUI (OpenGL); experimental Vulkan offscreen\n' \
    "$(git rev-parse HEAD)" > "$stage/BUILD.txt"
echo "Windows package: $stage"
