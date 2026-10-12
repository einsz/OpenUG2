# Contributing to OpenUG2

Use **`development`** for current builds, bug reproduction and contributions.
`main` receives reviewed integrations and can lag behind active work. Coding
agents must check their branch and commit before investigating or editing.

```sh
git fetch origin
git switch development
git pull --ff-only origin development
git branch --show-current
git rev-parse --short HEAD
```

Preserve uncommitted work. If switching or updating is blocked, resolve it safely
instead of resetting files. Base feature branches on current `development` and
target `development` in pull requests; do not push directly to `main`.

Read the [README](README.md) and [developer and agent guide](docs/DEVELOPER_GUIDE.md),
then the relevant subsystem and [format notes](docs/FORMATS.md). Coordinate the
scope in the [contribution roadmap #5](https://github.com/whoismept/OpenUG2/issues/5)
or an existing focused issue before a large change.

For interactive development, build with `make debug` so the ImGui diagnostics
are available. Include the branch, exact commit, build target, OS/architecture,
GPU/driver, data edition and reproduction command in reports. A decoded course
or a screenshot is not proof that a race completes or the engine matches retail.

Keep changes small and measured. Run the relevant deterministic tests and
provide same-pose before/after captures for visual changes. Record platform or
asset checks you could not run. Production behavior must stay unchanged during
an audit unless the task explicitly includes a fix.

This is a clean-room project. Use your own legal NFSU2 installation for local
checks; never commit EA assets, executables, retail data dumps or recovered game
code. Original parsers, synthetic fixtures, derived format facts and research
links are welcome. See the guide for the full boundary.
