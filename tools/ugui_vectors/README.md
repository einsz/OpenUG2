# Independent GUI test fixtures

These are approved synthetic compatibility vectors, plus a reviewer-sanitized
controller glyph index containing identification metadata only. No PNG artwork,
original binary, disc data or analyst evidence is bundled. The folder case
fixtures generate disposable synthetic PNGs and calculate their own digests.

Run the distributed synthetic checks with `make ugui-test`, or
`make -C src/ugui test`. Build the demo with `make ugui-demo`.
The image test boundary requires libpng and OpenSSL libcrypto development files
and pkg-config, in addition to the existing compiler/SDL2/OpenGL dependencies.

Normal/final console demo checks use the corresponding runtime ISO source:

```
build/ugui_demo --data PC_DATA --ps2-iso PS2_ISO --xbox-iso XBOX_ISO \
  --size 1280 960 --gui-check --shots /tmp/ugui-final-960
build/ugui_demo --data PC_DATA --ps2-iso PS2_ISO --xbox-iso XBOX_ISO \
  --size 1280 720 --gui-check --shots /tmp/ugui-final-720
```

For the PNG loading boundary only, explicitly enable testing:

```
build/ugui_test tools/ugui_vectors --test-glyph-folder EXPORTED_GLYPH_ROOT
build/ugui_demo --data PC_DATA --test-glyph-folder EXPORTED_GLYPH_ROOT \
  --size 1280 960 --gui-check --shots /tmp/ugui-folder-test-960
```

Folder results verify only this test path and never count as final ISO
acceptance. Paths are explicit user configuration; no folder is discovered or
used as a fallback. Both sources and captures remain external runtime inputs or
uncommitted test outputs. Resource availability and decoded counts alone do not
establish complete final-frame acceptance.
