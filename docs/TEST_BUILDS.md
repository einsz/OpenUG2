# Desktop test builds

Download development packages from a successful GitHub Actions run on
`development`. The **Windows build** and **macOS and Linux builds** workflows
produce separate artifacts for Windows x86_64, Linux x86_64, macOS Apple Silicon
(arm64), and macOS Intel (x86_64). Artifacts expire after seven days. Manual
preview releases, when published, provide public downloads under Releases.
Maintainers can run **Publish desktop preview** on `development` or `main`:
it rebuilds all four packages, requires every build/test job to succeed, then
publishes a prerelease named `preview-<commit>` with SHA256 checksums. It never
marks the preview as the latest stable release or replaces an existing tag.

These are **test builds**, not a claim of stable gameplay or GPU compatibility.
All primary binaries include **DebugUI**. No original game data is included;
you supply your own Underground 2 directory containing `CARS/`, `TRACKS/`, and
`GLOBAL/` through `--path`. Preserve the complete extracted package layout.

## Windows

See [WINDOWS.md](WINDOWS.md) for installation, runtime dependencies, Vulkan,
and PowerShell commands. Runtime DLLs and their licenses are included. From the
extracted directory:

```powershell
.\openug2.exe --path "YOUR_GAME_DATA_DIRECTORY" --hud --devui
```

## macOS 15 or newer

Choose **arm64** for Apple Silicon, **x86_64** for Intel. Extract the `.tar.gz`
inside the downloaded artifact ZIP. In Terminal, enter the extracted directory:

```sh
./openug2 --path "/path/to/your/game/data" --hud --devui
```

SDL2, libpng and libcrypto are bundled under `lib/`, with relative dependency
paths and licenses. Homebrew is not needed to run the package. This initial
package is a command-line executable, not a `.app` installer; it is ad-hoc
signed, not Apple Developer ID signed or notarized. Follow macOS's normal
approval flow only after verifying the download came from this repository;
do not disable Gatekeeper globally. macOS Vulkan is not included in this build.

## Linux x86_64

The binary is built on Ubuntu 22.04 (glibc 2.35) and is intended for compatible
x86_64 distributions with working OpenGL drivers. It is not an AppImage and
does not bundle glibc or system graphics libraries. Install the runtime libraries
before launching. On Ubuntu 22.04:

```sh
sudo apt-get install libsdl2-2.0-0 libpng16-16 libssl3 zlib1g libgl1
```

On Ubuntu 24.04, the PNG/OpenSSL package names are `libpng16-16t64` and
`libssl3t64`. On Arch Linux, install `sdl2-compat`, `libpng`, `openssl`, `zlib`,
`libglvnd`, and the graphics driver appropriate to your GPU. Other distributions
need equivalent SDL2, PNG16, crypto3, zlib and OpenGL runtime libraries.
Extract the `.tar.gz` inside the artifact ZIP, enter its directory, and run:

```sh
./openug2 --path "/path/to/your/game/data" --hud --devui
```

Use `ldd ./openug2` to identify a missing runtime library. `BUILD.txt` records the
source revision; `RUNTIME-LIBRARIES.txt` records the build host's linkage.
The optional `openug2-vk` and `rhi_vk_smoke` binaries are experimental, offscreen
only; they also need a Vulkan loader/driver. Vulkan does not provide interactive
presentation or DebugUI. Linux CI uses Mesa software rendering, not your GPU.

## What to test and report

1. Start the OpenGL binary with your data directory. Check the world, car, HUD,
   and **1** to open/close DebugUI.
2. Take a repeatable capture:
   `./openug2 --path "DATA" --resolution 1280x720 --frames 1 --shot opengl.png`.
3. When Vulkan is included, run the asset-free smoke test first, then repeat
   the capture with `openug2-vk`. Expect `SMOKE OK`; clean validation requires that
   the validation layer was actually loaded.
4. Include `BUILD.txt`, OS/CPU architecture, GPU/driver version, console output,
   and screenshots with your report. Never upload the original game assets.

CI proves compilation and the listed automated tests. Hardware rendering,
controller behavior, and real gameplay still require user testing. A stable
release should follow those reports, rather than treating a successful build
as proof that every supported machine works.
