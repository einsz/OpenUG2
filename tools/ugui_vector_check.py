#!/usr/bin/env python3
"""Run C vector tests, then independently check the decoded synthetic RGBA hash."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

binary, vectors = map(lambda s: Path(s).resolve(), sys.argv[1:3])
with tempfile.TemporaryDirectory(prefix="ugui_vector_") as tmp:
    output = Path(tmp) / "rgba.bin"
    env = dict(os.environ, UGUI_VECTOR_RGBA_FILE=str(output))
    result = subprocess.run([str(binary), str(vectors)], env=env)
    if result.returncode:
        sys.exit(result.returncode)
    expected = json.loads((vectors / "ps2_icon_codec_vectors.json").read_text())["expected_rgba_sha256"]
    actual = hashlib.sha256(output.read_bytes()).hexdigest()
    if actual != expected:
        raise SystemExit(f"PS2 decoded RGBA SHA256 mismatch: {actual}")
    print(f"PS2 decoded RGBA SHA256: {actual} (matches approved vector)")
