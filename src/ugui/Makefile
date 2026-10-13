# src/ugui/Makefile -- standalone builds for the ugui widget library
# (clean-room GUI library, see DESIGN.md). Does NOT replace the root Makefile.
#
#   make test      build + run the GL-free tests (spec vectors, widgets, nav)
#   make demo      build the SDL2+OpenGL demo window  ->  build/ugui/ugui_demo
#   make run       build + launch the demo
#   make check     launch the demo headlessly, check GL errors, write a screenshot
#                  of every page (and of every original screen when the game
#                  data is given: make check DATA=/path/to/game) into build/ugui/shots
#   make clean
#
# The spec test vectors are read from VECTORS (default: the clean-room spec
# directory, relative to this Makefile).

CC       ?= cc
CFLAGS   ?= -O2 -std=c99 -Wall -Wextra
CFLAGS   += -Wno-unused-function
# strtok_r / usleep style POSIX bits used by the test tools
override CFLAGS += -D_DEFAULT_SOURCE

SDL_CFLAGS := $(shell sdl2-config --cflags 2>/dev/null)
SDL_LIBS   := $(shell sdl2-config --libs   2>/dev/null)

UNAME := $(shell uname)
ifeq ($(UNAME),Darwin)
  GL_LIBS  := -framework OpenGL
  CFLAGS   += -DGL_SILENCE_DEPRECATION
else
  GL_LIBS  := -lGL
endif

BUILD   := ../../build/ugui
VECTORS ?= ../../cleanroom/spec/test-vectors

# GL-free core + widgets
CORE := ugui_util.c ugui_anim.c ugui_strtab.c ugui_fng.c ugui_canvas.c \
        ugui_controller.c ugui_codec.c ugui_bundle.c ugui_tex.c ugui_ffont.c ugui_assets.c \
        ugui_font.c ugui_text.c ugui_glyph.c ugui_kit.c ugui_ui.c \
        ugui_w_basic.c ugui_w_list.c ugui_w_entry.c ugui_w_dialog.c \
        ugui_hintbar.c ugui_scene.c
HDRS := $(wildcard *.h) ../../tools/ugui_demo_app.h
APP  := ../../tools/ugui_demo_app.c ../../tools/ugui_demo_orig.c

.PHONY: all test demo run check clean

all: test demo

$(BUILD):
	@mkdir -p $(BUILD)

# ---- GL-free tests ----
test: $(BUILD)/ugui_test
	python3 ../../tools/ugui_vector_check.py $(BUILD)/ugui_test $(VECTORS)

$(BUILD)/ugui_test: ../../tools/ugui_test.c ../../tools/ugui_test_more.inc ../../tools/ugui_test_assets.inc ../../tools/ugui_test_amend3.inc ../../tools/ugui_test_amend4.inc $(CORE) $(APP) $(HDRS) | $(BUILD)
	$(CC) $(CFLAGS) -DUG_VECTOR_DIR='"$(VECTORS)"' -I. -I../../tools ../../tools/ugui_test.c $(CORE) $(APP) -o $@ -lm

# ---- SDL2 + OpenGL demo ----
demo: $(BUILD)/ugui_demo
	@echo "Run:  $(BUILD)/ugui_demo [--data GAME_DIR] [--size W H]   (mouse only: hover, click, wheel, drag, right-click = Back)"

$(BUILD)/ugui_demo: ../../tools/ugui_demo.c $(CORE) ugui_gl.c ../gl_state.c $(APP) $(HDRS) | $(BUILD)
	$(CC) $(CFLAGS) -I. -I../../tools $(SDL_CFLAGS) ../../tools/ugui_demo.c $(CORE) ugui_gl.c ../gl_state.c $(APP) -o $@ $(SDL_LIBS) $(GL_LIBS) -lm

run: demo
	$(BUILD)/ugui_demo $(if $(DATA),--data "$(DATA)")

# DATA = the game install (default: $$UGUI_DATA or ./); original data is required for faithful visual checks
DATA ?=
check: demo
	@rm -rf $(BUILD)/shots
	@mkdir -p $(BUILD)/shots
	timeout 900 $(BUILD)/ugui_demo --vectors $(VECTORS) --size 640 480 --check --shots $(BUILD)/shots $(if $(DATA),--data "$(DATA)")
	timeout 120 $(BUILD)/ugui_demo --vectors $(VECTORS) --size 1280 720 --pillbox $(if $(DATA),--data "$(DATA)")

clean:
	rm -rf $(BUILD)
