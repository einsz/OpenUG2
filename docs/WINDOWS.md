# Windows builds and testing

Use **64-bit Windows 10/11**, **MSYS2 UCRT64**, and the `development` branch.
The existing GNU Make build produces native Windows executables; Visual Studio,
WSL, and the plain MSYS shell are not the build path described here.
OpenGL + DebugUI is the interactive reference. Vulkan is an **experimental
offscreen backend**, without presentation, DebugUI, or the GL-only UI renderers.

## Install the build tools

1. Install [MSYS2](https://www.msys2.org/) and open **MSYS2 UCRT64**.
2. Run `pacman -Syu`. If asked to close the terminal, reopen UCRT64 and repeat
   until the update completes.
3. Install the native UCRT64 dependencies:

```sh
pacman -S --needed make git vim \
  mingw-w64-ucrt-x86_64-gcc \
  mingw-w64-ucrt-x86_64-pkgconf \
  mingw-w64-ucrt-x86_64-SDL2 \
  mingw-w64-ucrt-x86_64-glew \
  mingw-w64-ucrt-x86_64-libpng \
  mingw-w64-ucrt-x86_64-openssl \
  mingw-w64-ucrt-x86_64-zlib
```

`vim` supplies the `xxd` shader embedding tool. Keep all native libraries in the
same UCRT64 environment; do not mix MINGW64, CLANG64, or MSYS library packages.
Check that `echo "$MSYSTEM"` prints `UCRT64` and `command -v gcc` is under
`/ucrt64/bin`. The package/environment conventions are documented by
[MSYS2](https://www.msys2.org/docs/environments/).

## Build and run with DebugUI

```sh
git clone --branch development https://github.com/whoismept/OpenUG2.git
cd OpenUG2
make -j2 debug
./nfsu2.exe --path "C:/Games/NFSU2" --hud --devui
```

`--path` accepts your own data directory, including quoted paths with spaces.
The earlier positional data-directory argument is also accepted. A missing or
empty `--path` value exits with a usage error.

Supply your own legally obtained Underground 2 data directory, containing
`CARS/`, `TRACKS/`, and `GLOBAL/`. No original game assets are distributed.
Use a short ASCII directory initially; always quote paths containing spaces.
Press **1** for the developer panel. Build after source updates with
`make -j2 debug` again. In an existing clone, preserve local changes before
switching to `development` and updating with `git pull --ff-only`.

Run from UCRT64 so its runtime DLLs are on PATH. If Windows reports a missing
SDL2/GLEW/PNG/crypto/compiler DLL, return to that shell and check the installed
packages. Do not download DLLs from third-party download sites. A packaged
Actions artifact includes the imported runtime DLLs alongside the executables.
Downloaded packages name them `openug2.exe` and `openug2-vk.exe`; the current
source-build Make targets still produce `nfsu2.exe` and `nfsu2-vk.exe`.
Install a current graphics driver from your GPU vendor: the Microsoft basic
display driver does not establish working OpenGL/Vulkan support.

The optional **TEST-ONLY PNG controller-glyph folder** is currently unsupported
on Windows and reports that status explicitly. The normal game-data controller
texture source is unaffected. Its POSIX no-follow containment checks are not
replaced by an unchecked Windows path loader.

## Test experimental Vulkan

Install the additional build dependencies in the same UCRT64 shell:

```sh
pacman -S --needed \
  mingw-w64-ucrt-x86_64-vulkan-headers \
  mingw-w64-ucrt-x86_64-vulkan-loader \
  mingw-w64-ucrt-x86_64-glslang
make -j2 nfsu2-vk
make rhi-vk-smoke
./nfsu2-vk.exe --path "C:/Games/NFSU2" --shot windows-vulkan.png
```

An RTX 4070 machine with the NVIDIA driver is a useful hardware test host.
The smoke test renders synthetic inputs and checks readback without game data;
expect a successful exit and `SMOKE OK`. The screenshot command exits
after capture: it is not an interactive Vulkan game. Keep resolution, vehicle,
track, and camera identical when comparing with an OpenGL `--shot` capture.
Do not set Linux lavapipe ICD paths or `SDL_VIDEODRIVER=offscreen` on Windows;
the backend uses a plain hidden SDL window and the installed Windows driver.
A black image, crash, validation message, or missing panel is not proof that the
GPU is unsupported; record which executable was used and its complete console
output. DebugUI is available in `nfsu2.exe`, not `nfsu2-vk.exe`.

For a local package after both builds and `make build/rhi_vk_smoke.exe`:

```sh
bash tools/package_windows.sh
```

The fresh `build/windows-ucrt64/` directory contains executables, imported
runtime DLLs, licenses, and the source revision. Rename/remove an older package
first; packaging refuses to mix outputs into an existing directory.

## Actions build artifacts

[Windows build reference](../.github/workflows-disabled/windows-build.yml) is
currently **inactive**: repository Actions are disabled. Follow the
[six-platform plan](BUILD_RELEASE_PLAN.md) for future reactivation. The archived
recipe builds DebugUI and
experimental Vulkan, runs asset-free CPU/portability tests, and uploads a
Windows UCRT64 artifact with runtime DLLs and licenses. Its former triggers were
pull requests, pushes and manual dispatch; they do not run while paused.
For any unexpired earlier artifact, download from its successful run's **Artifacts** section,
extract the whole archive, and pass your data directory to `openug2.exe` from
PowerShell, for example:

```powershell
.\openug2.exe --path "C:/Games/NFSU2" --hud --devui
.\rhi_vk_smoke.exe
```

These are development artifacts, not tagged releases or an installer. Hosted
runner compilation and CPU tests do not validate physical GPU rendering,
interactive DebugUI, or RenderDoc capture/replay. Those need a Windows hardware
run before claiming runtime compatibility or promoting a public release.

## Collect Windows test results

In the extracted package folder, use PowerShell and replace only the value
of `--path`. Start with OpenGL and press **1** to check DebugUI; close the game
before continuing. Vulkan smoke uses no game data.

```powershell
.\openug2.exe --path "C:/Games/NFSU2" --hud --devui 2>&1 | Tee-Object opengl.log
.\rhi_vk_smoke.exe 2>&1 | Tee-Object vulkan-smoke.log
.\openug2.exe --path "C:/Games/NFSU2" --resolution 1280x720 --frames 1 --shot opengl.png 2>&1 | Tee-Object opengl-shot.log
.\openug2-vk.exe --path "C:/Games/NFSU2" --resolution 1280x720 --frames 1 --shot vulkan.png 2>&1 | Tee-Object vulkan-shot.log
```

Use the same data, car, track and options in both screenshots; do not add
`--devui` to the comparison capture. The experimental Vulkan backend does not
render the GL-only UI, so compare vehicle/world rendering. Keep `BUILD.txt`,
the logs and both PNGs when reporting results. A `validation messages: 0` line
is evidence of clean validation only if the validation layer was actually
loaded; a "validation not found" warning means that layer did not run.
