#!/bin/sh
# check_gl.sh — renderer-boundary gate (issue #14, P10): no GL API outside the
# GL backend. Fails if any src/ file that is not an allowed GL owner names a GL
# header, a gl* entry point, or a GL* type. Comments and string literals are
# stripped first, so GL mentioned in prose or diagnostics does not count.
#
# Allowed GL owners:
#   rhi_gl.c / rhi_gl.h / rhi_gl_state.c / rhi_gl_state.h  the GL backend itself
#   debugui.cpp                                            Dear ImGui's GL2 backend
#   hud.c / ugui/ugui_gl.c / ugui/ugui_gl.h                self-contained GL
#                                                          renderers (to migrate)
# Everything else reaches the GPU only through rhi.h.
set -eu

root=${1:-src}
allow="rhi_gl.c rhi_gl.h rhi_gl_state.c rhi_gl_state.h debugui.cpp ugui_gl.c ugui_gl.h hud.c"

fail=0
# -- find source files, skip the allowed owners by basename --
for f in $(find "$root" -type f \( -name '*.c' -o -name '*.h' -o -name '*.cpp' \) | sort); do
    base=${f##*/}
    skip=0
    for a in $allow; do [ "$base" = "$a" ] && skip=1 && break; done
    [ "$skip" = 1 ] && continue

    # strip block comments, line comments and string literals, keep line numbers
    stripped=$(perl -0777 -pe 's{/\*.*?\*/}{ }gs; s{//[^\n]*}{}g; s{"(\\.|[^"\\\n])*"}{""}g' "$f")

    hits=$(printf '%s\n' "$stripped" | grep -nE \
        '#[[:space:]]*include[[:space:]]*[<"](GL/|GLES|OpenGL/|SDL_opengl)|\bgl[A-Z][A-Za-z0-9]*[[:space:]]*\(|\b(GLuint|GLenum|GLint|GLfloat|GLboolean|GLsizei|GLvoid|GLchar|GLbitfield|GLclampf|GLubyte|GLushort|GLshort|GLdouble|GLintptr|GLsizeiptr|GLbyte|GLuint64|GLfixed)\b' \
        || true)

    if [ -n "$hits" ]; then
        fail=1
        echo "check-gl: GL API outside the backend in $f:"
        printf '%s\n' "$hits" | sed 's/^/    /'
    fi
done

if [ "$fail" != 0 ]; then
    echo "check-gl: FAILED — route the above through rhi.h, or add a documented owner to tools/check_gl.sh." >&2
    exit 1
fi
echo "check-gl: OK — no GL API outside the backend (src/)."
