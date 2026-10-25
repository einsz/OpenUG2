# Six-platform build and release implementation plan

> **Status: planned, inactive.** Repository Actions are disabled. Workflow
> references are archived under `.github/workflows-disabled/`, outside the
> executable workflow directory. This document does not enable builds or releases.
>
> **For agentic workers:** Use `superpowers:executing-plans` when implementation
> is explicitly requested. Do not restore workflows or enable Actions from this
> plan alone. Preserve unrelated working-tree changes and work on `development`.

**Goal:** Eventually produce downloadable OpenUG2 test packages for six native
OS/CPU targets, then promote verified packages to stable releases.

**Architecture:** Separate native build/test jobs from packaging and release
publication. Each package records its exact source revision, OS and architecture;
publication requires the checks appropriate to every advertised target.

**Tech stack:** Existing C/C++ engine, GNU Make, SDL, OpenGL + DebugUI, platform
package managers and GitHub Actions. ARM64 toolchain and runner choices remain
implementation work, particularly on Windows.

**Spec:** The target matrix, constraints and acceptance criteria below record
the requested first-stage distribution scope.

## Target matrix

| OS | CPU | Intended package | Current evidence / remaining work |
| --- | --- | --- | --- |
| Linux | arm64 | `OpenUG2-linux-arm64.tar.gz` | Planned; native build, dependencies, packaging and device tests not implemented |
| Linux | x86_64 | `OpenUG2-linux-x86_64.tar.gz` | Earlier hosted build/CPU/software-rendering/package checks passed; hardware/gameplay acceptance remains open |
| Windows | arm64 | `OpenUG2-windows-arm64.zip` | Planned; native compiler, dependency ABI, graphics support and packaging must be established |
| Windows | x86_64 | `OpenUG2-windows-x86_64.zip` | Earlier UCRT64 build/CPU/package checks passed; reported authored-start failure on user data remains unresolved |
| macOS | arm64 | `OpenUG2-macos-arm64.tar.gz` | Earlier hosted build/package checks and second-machine CLI check passed; hardware/gameplay acceptance remains open |
| macOS | x86_64 | `OpenUG2-macos-x86_64.tar.gz` | Earlier hosted build/CPU/package checks passed; hardware/gameplay acceptance remains open |

These are six distribution targets, not six currently supported or stable
releases. Earlier passing checks do not establish compatibility with all devices.

## Constraints

- Keep primary packages on OpenGL with DebugUI enabled. Public executable names
  are `openug2` / `openug2.exe`; data is supplied through quoted `--path` values.
- Do not include original game assets, EA executables or disassembly.
- Preserve the distinction between compilation, CPU tests, software-rendered
  tests and physical-device/gameplay validation. Emulated execution is not proof
  of native graphics compatibility.
- The current Vulkan path is experimental and offscreen-only. Do not advertise
  interactive presentation, DebugUI or additional Vulkan platforms without tests.
- Choose minimum OS/glibc versions, native runners, toolchains and dependency
  versions explicitly before implementation. Recheck availability and pricing
  then; this plan does not reserve paid runners or authorize purchases.
- Keep all build inputs read-only for untrusted PR runs. Release write permission
  belongs only to a separate trusted publication job, without executing PR artifacts.
- Main promotion uses a reviewed `development` to `main` PR. Never push directly
  to `main`, force-update release tags or label an untested build stable.

## Review focus

- Missing, unreadable, mismatched or variant game data must produce actionable
  diagnostics; resolve the reported Windows authored-start failure before stable.
- ARM64 packages must contain native ARM64 binaries and dependencies, including
  DLLs; an x86_64 executable running under emulation does not satisfy the target.
- Include dynamically loaded dependencies as well as linker imports: macOS
  SDL2 compatibility builds also require SDL3 and its expected filename alias.
- A package must run outside the build directory without a compiler/package
  manager supplying accidentally omitted private libraries.
- macOS signing/notarization and Windows graphics-driver availability must be
  documented honestly. Do not disable OS security policy to make a check pass.

## Implementation phases — not started

### 1. Establish the six native build contracts

- [ ] Record OS floors, compiler/SDK versions, dependency sources, runner
  availability and cost limits for each matrix row.
- [ ] Reuse existing x86_64/Linux, x86_64/Windows and both macOS recipes as
  references, reviewing them against the chosen environments again.
- [ ] Establish Linux ARM64 compilation and runtime dependencies separately
  from the current x86_64-only Linux packaging script.
- [ ] Evaluate a native Windows ARM64 toolchain and ARM64 SDL/PNG/crypto/GL
  dependencies. Do not reuse UCRT64 x86_64 libraries for ARM64 output.
- [ ] Add failing architecture/ABI checks before adding each new build recipe;
  make them pass with native output. Record unverified GPU capabilities explicitly.

Acceptance: all six binaries identify as their requested architecture, link to
matching dependencies, and pass asset-free CPU and CLI tests on the target OS.

### 2. Package and validate independent downloads

- [ ] Generalize `tools/package_linux.sh` and `tools/package_windows.sh` only
  after the new architecture contracts are proven; retain overwrite protection.
- [ ] Extend `tools/package_macos.py` only where the validated contracts require
  it; retain recursive dependency, alias, architecture, license and signing checks.
- [ ] Include `BUILD.txt`, run instructions and project/dependency licenses;
  record Linux external runtime requirements instead of bundling glibc or drivers.
- [ ] Test unpacked archives on clean target environments, including missing
  data, paths with spaces and executable naming. Add regressions before fixes.
- [ ] Validate OpenGL/DebugUI and real game loading on physical devices; test
  Vulkan only on targets where it is actually shipped.

Acceptance: six self-consistent downloads with reproducible provenance and
documented runtime dependencies; no unsupported target is silently omitted.

### 3. Reintroduce release automation only when requested

- [ ] Obtain an explicit instruction to reactivate Actions; inspect repository
  settings and any required security checks before changing them.
- [ ] Restore the archived workflow files to `.github/workflows/` together,
  update reusable-workflow paths if renamed, and extend the matrix to all six
  proven targets. Archived files currently describe only the earlier four targets.
- [ ] Keep Actions disabled until a review of the final workflow diff passes;
  then enable the intended workflows and run an explicitly requested trial.
- [ ] Require all advertised target jobs and packaged-runtime checks before a
  manual prerelease publication. Publish six archives plus `SHA256SUMS` for one
  exact commit, with immutable target names and tags.
- [ ] Retain short CI artifact lifetimes, bounded jobs and cancellation of
  obsolete builds. Review storage allowances and budgets before reactivation.
- [ ] Update `docs/TEST_BUILDS.md`, `docs/WINDOWS.md` and README with current
  download instructions and verified platform requirements.
- [ ] Promote a prerelease to stable only after the required device reports
  are accepted and blocking startup/rendering issues are resolved.

Acceptance: reviewed six-target automation and a deliberate prerelease-to-stable
process. Until a later instruction activates this phase, no CI or deployment runs.

## Current pause and existing downloads

The existing preview and previous artifacts are historical test outputs; pausing
Actions does not delete releases. Artifacts retain their original expiry dates.
No new build, release or main promotion is part of this planning change.

The archived workflows are recovery/reference material, not active configuration.
Repository-level Actions must also be deliberately re-enabled before future runs.
