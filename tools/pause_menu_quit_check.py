#!/usr/bin/env python3
"""QUIT_DIALOG vectors (quit_dialog.json) against the real pause adapter, on synthetic fixtures.

The adapter binary is the pause_menu_test harness. Every case of the vector file is encoded: the
expected values come from the vector file, the scenario inputs are synthetic (no retail data). Facts
that only the original package can show (package hashes, authored fade-in of the right button,
rendered look) are reported as unverified, never as passed."""
import hashlib
import json
import re
import subprocess
import sys
from pathlib import Path

binary, spec = sys.argv[1], Path(sys.argv[2])
APPROVED_VECTOR_SHA256 = "a76b8c978a8c541ad0e4e1e7570fd344255918d8d83b75ec96cfd096d1f6777b"
raw = (spec / "quit_dialog.json").read_bytes()
assert hashlib.sha256(raw).hexdigest() == APPROVED_VECTOR_SHA256, "quit_dialog.json differs from the approved vector"
vec = json.loads(raw)
CASES = {c["id"]: c for c in vec["cases"]}
used = set()
checks = 0


def run(*args):
    out = subprocess.run([binary, *map(str, args)], check=True, capture_output=True, text=True).stdout
    return json.loads(out.strip().splitlines()[-1])


def scenario(events, mode=1):
    return run("quit-scenario", mode, ",".join(events) if events else "-")


def present(events):
    return run("quit-presentation", ",".join(events))


def tl(cx, cy):
    """Centred virtual units to the top-left units of the adapter."""
    return cx + 320.0, cy + 240.0


def at(name, cx, cy):
    x, y = tl(cx, cy)
    return f"{name}:{x:.4f}:{y:.4f}"


def case(cid):
    used.add(cid)
    return CASES[cid]["expect"] if "expect" in CASES[cid] else {}


def ok(cond, *info):
    global checks
    checks += 1
    assert cond, info


def rgba_eq(got, want, tol=1):
    return len(got) == 4 and all(abs(a - b) <= tol for a, b in zip(got, want))


# ---- identities and constants ----------------------------------------------------------------
by_guid = {o["guid"].lower(): o for o in vec["objects"]}
ident = run("quit-identities")
ok(len(ident) == 14, len(ident))
for i in ident:
    o = by_guid[i["guid"].lower()]
    ok(o["object_hash"].lower() == i["hash"].lower() and o["type"] == i["type"] and
       o["resource_index"] == i["resource"] and o["parent_guid"].lower() == i["parent"].lower() or
       (o["guid"].lower() in ("0x000def80", "0x000def81") and i["parent"].lower() == "0x000def6e"), i, o)
const = run("quit-constants")
ok(const["package_key"].lower() == vec["primary_package"]["key"].lower(), const)
ok(const["package_name"] == vec["primary_package"]["lookup_name"], const)
S = vec["strings"]
ok(const["message"].lower() == S["CONFIRM_QUIT_GAME"]["key"].lower(), const)
ok(const["yes"].lower() == S["YES"]["key"].lower() and const["no"].lower() == S["NO"]["key"].lower(), const)
ok(const["yes_alt"].lower() == S["UI_COMMON_YES"]["key"].lower(), const)
ok(const["no_alt"].lower() == S["UI_COMMON_NO"]["key"].lower(), const)
ok(const["caption"].lower() == S["MU_PAUSE_QUIT_TO_FE"]["key"].lower(), const)
subprocess.run([binary, "quit-static"], check=True, capture_output=True)

LEFT = (-160.0, 74.0)     # centres of the visible button extents
RIGHT = (160.0, 74.0)


def focus_of(s):
    return s["focus"]


def assert_result(s, result, quit_invoked):
    ok(s["result"] == result and s["quit_invoked"] == quit_invoked, s, result, quit_invoked)


def assert_pause_alive(s):
    ok(s["pause_still_open"] and s["game_paused"] and not s["pause_resumed"] and s["selected"] == 1, s)


# ---- QD-open-rest / entry samples --------------------------------------------------------------
e = case("QD-open-rest")
s = present(["open", "update:180"])
ok(s["focus_node"] == s["right_node"], s)
ok(s["hidden"] == [0] * 9, s)                       # title, icon, single-button set, off-screen objects
ok(rgba_eq(s["left_label_rgba"], e["left_label_rgba"], 0), s)
ok(rgba_eq(s["right_label_rgba"], e["right_label_rgba_pulse_phase_180ms"], 1), s)
ok(s["left_button_alpha"] == e["left_button_alpha"], s)
ok(e["title_visible"] is False and e["icon_visible"] is False and e["centre_button_visible"] is False)

e = case("QD-entry-samples")
fade = "40:0,180:255"
for t, v in e["fade_group_alpha"].items():
    ok(abs(run("quit-track", fade, t)[0] - v) <= 1, (t, v))
for t, v in e["message_rgba"].items():
    ok(abs(run("quit-track", fade, t)[0] - v[3]) <= 1, (t, v))
for t, v in e["right_label_init_if_unfocused"].items():
    ok(abs(run("quit-track", fade, t)[0] - v[3]) <= 1, (t, v))
root = by_guid["0x000e0b39"]
ok(root["stored_rgba"][3] == e["always_on_dark_panel_alpha_at_0ms"])
ev = vec["evaluated_colour_samples"]
for name, keys in (("fade_group_FADEIN", "40:0,180:255"), ("button_left_FADEIN", "40:0,180:80"),
                   ("button_right_FADEIN", "40:0,180:255"), ("label_left_FADEIN", "40:0,180:255"),
                   ("fade_group_FADEOUT", "0:255,100:0"), ("label_FADEOUT", "0:255,100:0")):
    for t, v in ev[name].items():
        ok(abs(run("quit-track", keys, t)[0] - v[3]) <= 1, (name, t, v))

# ---- pulse and focus changes -----------------------------------------------------------------------
e = case("QD-pulse")
for t, v in e["right_label_rgba"].items():
    s = present(["open", f"update:{t}"])
    ok(rgba_eq(s["right_label_rgba"], v, 1), (t, v, s["right_label_rgba"]))

e = case("QD-left")
for how in ("key_left", "pad_left"):
    s = present(["open", "update:180", how])
    ok(s["focus_node"] == s["left_node"], s)
    ok(s["left_button_alpha"] == e["left_button_alpha"] and s["right_button_alpha"] == e["right_button_alpha"], s)
    ok(rgba_eq(s["left_label_rgba"], e["left_label_rgba_at_change"], 0), s)
    ok(rgba_eq(s["right_label_rgba"], e["right_label_rgba"], 0), s)
    r = scenario(["open", "update:180", how])
    ok(r["result"] is None and r["quit_invoked"] == 0 and r["dialog_open"] and r["focus"] == "left_button_positive", r)

case("QD-left-at-left")
a = present(["open", "update:180", "key_left", "update:50"])
b = present(["open", "update:180", "key_left", "update:50", "key_left", "key_up", "key_down"])
ok(a == b, a, b)                                       # no wrap, Up/Down inert, nothing changes
r = scenario(["open", "key_left", "key_left", "repeat_left"])
ok(r["focus"] == "left_button_positive" and r["result"] is None and r["quit_invoked"] == 0, r)

e = case("QD-right-back")
s = present(["open", "update:180", "key_left", "update:50", "key_right"])
ok(s["focus_node"] == s["right_node"], s)
ok(s["right_button_alpha"] == e["right_button_alpha"] and s["left_button_alpha"] == e["left_button_alpha"], s)
ok(rgba_eq(s["right_label_rgba"], e["right_label_rgba_at_change"], 0), s)
ok(rgba_eq(s["left_label_rgba"], e["left_label_rgba"], 0), s)

case("QD-right-at-right")
a = present(["open", "update:180"])
b = present(["open", "update:180", "key_right", "pad_right", "key_up", "pad_up", "wheel"])
ok(a == b, a, b)

# ---- accept / back ----------------------------------------------------------------------------------------
e = case("QD-accept-default")
for accept in ("key_enter", "key_kpenter", "pad_a"):
    for lead in ([], ["update:0"], ["update:90"], ["update:500"]):     # including the very first frame
        s = scenario(["open", *lead, accept])
        assert_result(s, e["result"], e["quit_invoked"])
        ok(s["pause_still_open"] == e["pause_still_open"] and s["game_paused"] == e["game_paused"], s)
        ok(s["pause_resumed"] == e["pause_resumed"] and not s["dialog_open"] == e["dialog_exits"], s)
        ok(s["selected"] == 1, s)
s = scenario(["open", "key_enter"], mode=0)                # logic-only fixture behaves the same
assert_result(s, "cancel", 0)

e = case("QD-accept-left")
for left, accept in (("key_left", "key_enter"), ("pad_left", "pad_a"), ("key_left", "key_kpenter")):
    s = scenario(["open", left, accept, "update"])
    assert_result(s, e["result"], e["quit_invoked"])
    ok(s["quit_frame"] == 0 and not s["dialog_open"], s)

e = case("QD-back")
for pre in ([], ["key_left"]):
    for back in ("key_escape", "pad_b", "pad_back"):
        s = scenario(["open", *pre, back, "update"])
        assert_result(s, e["result"], e["quit_invoked"])
        assert_pause_alive(s)
        ok(e["trigger_delivered_to_pause"] is False and s["resume_actions"] == 0 and s["pills_drawn"], s)
s = scenario(["open", "key_escape", "repeat_escape", "update"])        # its auto-repeat is not a new Back
ok(s["resume_actions"] == 0 and s["pause_still_open"], s)

# ---- pointer ---------------------------------------------------------------------------------------------------
# visible extents of Q5.5, centred, inclusive
vec_extent = {"left": (-285.7, 53.0, -16.3, 94.4), "right": (16.3, 53.0, 285.7, 94.4)}
def inside_points(side):
    x0, y0, x1, y1 = vec_extent[side]
    return [((x0 + x1) / 2, (y0 + y1) / 2), (x0, y0), (x1, y1), (x0, y1), (x1, y0)]

e = case("QD-click-left")
for px, py in inside_points("left"):
    s = scenario(["open", at("press", px, py), "update"])
    ok(s["result"] is None and s["quit_invoked"] == 0 and s["dialog_open"], s)
    s = scenario(["open", at("press", px, py), "update", at("release", px, py), "update"])
    assert_result(s, e["result"], e["quit_invoked"])
    ok(s["quit_frame"] == 1, s)                                       # the release frame
e = case("QD-click-right")
for px, py in inside_points("right"):
    s = scenario(["open", at("press", px, py), at("release", px, py), "update"])
    assert_result(s, e["result"], e["quit_invoked"])
    assert_pause_alive(s)
ok(CASES["QD-click-right"]["expect"]["result_on"] == "release frame")
e = case("QD-press-no-result")
for side in (LEFT, RIGHT):
    s = scenario(["open", at("press", *side), "update:100"])
    ok(s["result"] is None and s["quit_invoked"] == 0 and s["dialog_open"] and s["press"] in (0, 1), s)
case("QD-release-elsewhere")
for events in ([at("press", *LEFT), at("release", 0, 0)], [at("press", *LEFT), at("release", *RIGHT)],
               [at("press", *RIGHT), at("release", *LEFT)], [at("press", *LEFT), at("release", -300, 74)],
               [at("press", *LEFT), at("release", -285.8, 74)], [at("press", *LEFT), at("release", -160, 94.5)]):
    s = scenario(["open", *events, "update"])
    ok(s["result"] is None and s["quit_invoked"] == 0 and s["dialog_open"], (events, s))
case("QD-press-outside-release-inside")
for press in ((0, 0), (0, 1), (-310, -230), (-160, 52.9), (-700, 74)):
    for target in (LEFT, RIGHT):
        s = scenario(["open", at("press", *press), at("release", *target), "update"])
        ok(s["result"] is None and s["quit_invoked"] == 0 and s["dialog_open"], (press, target, s))
case("QD-click-outside")
# panel, message, darkened screen, letterbox, the pause arrows and the centred item underneath
for p in ((0, 0), (0, 1), (-310, -230), (300, 200), (-700, 0), (700, 0), (-250.0, -12.0), (3.0, -13.0)):
    s = scenario(["open", at("press", *p), at("release", *p), "update"])
    ok(s["result"] is None and s["dialog_open"] and s["selected"] == 1 and s["pending"] == 0 and s["marker"] == 596.0, (p, s))
    ok(s["resume_actions"] == 0 and s["pause_still_open"], s)

e = case("QD-hover-left")
s = present(["open", "update:180", at("move", *LEFT)])
ok(s["focus_node"] == s["left_node"], s)
ok(s["left_button_alpha"] == e["left_button_alpha"] and s["right_button_alpha"] == e["right_button_alpha"], s)
ok(rgba_eq(s["left_label_rgba"], e["left_label_rgba_at_change"], 0) and rgba_eq(s["right_label_rgba"], e["right_label_rgba"], 0), s)
r = scenario(["open", at("move", *LEFT)])
ok(r["result"] is None and r["quit_invoked"] == 0 and r["pill_hover"] == -1, r)
case("QD-hover-leave")
a = present(["open", "update:180", at("move", *LEFT), "update:50"])
for away in ((0, 0), (0, -150), (-700, 74), (-285.8, 74)):       # leaving keeps the focus, nothing changes
    b = present(["open", "update:180", at("move", *LEFT), "update:50", at("move", *away)])
    ok(a == b, away, a, b)
# hovering the same button again keeps the pulse running (no restart)
b = present(["open", "update:180", at("move", *LEFT), "update:50", at("move", *LEFT)])
ok(a == b, a, b)
case("QD-hover-stationary-open")
s = scenario(["open", "key_enter"])
ok(s["focus"] == "right_button_negative" and s["result"] == "cancel" and s["quit_invoked"] == 0, s)
s = scenario([at("move", *LEFT), "open", "key_enter"])             # pointer rested there before opening
ok(s["focus"] == "right_button_negative" and s["result"] == "cancel" and s["quit_invoked"] == 0, s)
s = scenario([at("move", *LEFT), "open", "update:100", "key_enter"])
ok(s["result"] == "cancel", s)

e = case("QD-right-mouse")
for pre in ([], ["key_left"]):
    s = scenario(["open", *pre, "mouse_right_down"])               # on the press frame, before any update
    assert_result(s, e["result"], e["quit_invoked"])
    assert_pause_alive(s)
    s = scenario(["open", *pre, "mouse_right_down", "mouse_right_up", "update"])
    ok(s["resume_actions"] == 0 and s["pause_still_open"] and not s["dialog_open"], s)
    ok(e["paired_release_delivered_to_pause"] is False)
case("QD-start-ignored")
s = scenario(["open", "update:50", "pad_start", "update"])
ok(s["dialog_open"] and s["result"] is None and not s["pause_resumed"] and s["resume_actions"] == 0, s)

# ---- exit, latency, composition --------------------------------------------------------------------------
e = case("QD-exit")
for t, v in e["fade_group_alpha"].items():
    ok(abs(run("quit-track", "0:255,100:0", t)[0] - v) <= 1, (t, v))
for t, v in e["label_alpha"].items():
    ok(abs(run("quit-track", "0:255,100:0", t)[0] - v) <= 1, (t, v))
for t, v in e["message_alpha_authored"].items():
    ok(abs(run("quit-track", "0:0,100:255", t)[0] - v) <= 1, (t, v))
ok(e["always_on_dark_panel_alpha_until_removed"] == root["stored_rgba"][3])
# the dialog is gone on the input frame (<= 100 ms) and accepts nothing after the activation
for events in (["open", "key_left", "key_enter"], ["open", at("press", *LEFT), at("release", *LEFT)],
               ["open", at("press", *RIGHT), at("release", *RIGHT)], ["open", "key_escape"]):
    s = scenario(events)
    ok(not s["dialog_open"] and e["dialog_removed_by_ms"] >= 0, s)
    s2 = scenario([*events, "key_left", "key_enter", "key_enter", "mouse_right_down", at("click", *LEFT)])
    ok(s2["confirms"] == s["confirms"] and s2["cancels"] == s["cancels"], s, s2)
ok(e["dialog_accepts_input_after_activation"] is False and e["exit_presentation_intercepts_input"] is False)

e = case("QD-cancel-latency")
paths = [["key_enter"], ["pad_a"], ["key_escape"], ["pad_b"], ["mouse_right_down"],
         [at("press", *RIGHT), at("release", *RIGHT)]]
for path in paths:
    s = scenario(["open", *path, "key_left"])     # same frame: the probe key moves the pause selection
    ok(s["selected"] == 0 and s["pills_drawn"] and not s["dialog_open"], (path, s))
    s = scenario(["open", *path, "update", "key_left", "update"])   # and on the next frame
    ok(s["selected"] == 0 and s["pause_still_open"] and s["resume_actions"] == 0 and s["quit_invoked"] == 0, (path, s))
    ok(e["game_paused"] is True and e["pause_selection_unchanged"] is True)
    s = scenario(["open", *path])
    ok(s["selected"] == 1 and s["marker"] == 596.0 and s["pills_drawn"] and s["game_paused"], (path, s))

e = case("QD-confirm-latency")
for path in (["key_left", "key_enter"], ["pad_left", "pad_a"], [at("press", *LEFT), at("release", *LEFT)]):
    for lead in ([], ["update", "update", "update:40"]):
        s = scenario(["open", *lead, *path, "update"])
        ok(s["quit_invoked"] == e["quit_invoked"] and s["quit_frame"] == len(lead) and not s["dialog_open"], (path, s))
        s = scenario(["open", *lead, *path])
        ok(s["quit_invoked"] == 1 and s["pending"] == 3, s)         # requested on the input frame, before any update
ok(e["waits_for_exit_fade"] is False)

e = case("QD-compose")
s = scenario(["open", "update:180"])
ok(not s["pills_drawn"] and s["selected"] == 1 and s["game_paused"] and s["dialog_open"], s)
ok(e["pause_dim_scripts_started"] is False and e["pause_drawn_beneath"] is True)
# pills are not hoverable or clickable while open; the pause screen and arrows get nothing
s = scenario(["open", at("move", 104 - 320 + 0.0, 302 - 240 + 0.0)])
ok(s["pill_hover"] == -1, s)
s = scenario(["open", "key_up", "key_down", "wheel", "pad_up", "update:50"])
ok(s["selected"] == 1 and s["dialog_open"] and s["result"] is None and s["pending"] == 0, s)
# the pause dim scripts (DIM/UNDIM requests) are never sent by the adapter
src = "".join(p.read_text() for p in (Path(binary).resolve().parent.parent / "src").glob("pause_menu*"))
ok(not re.search(r"UG_MSG_(REQ_)?(UN)?DIM", src), "pause dim scripts must stay unused")
ok(e["beneath_brightness_factor"] == 0.4 and e["tolerance"] == 0.03)
s = scenario(["open_pill", at("release", 0, 0), "update"])             # opened by a pill press
ok(s["dialog_open"] and s["result"] is None and s["selected"] == 1, s)
s = scenario(["open_pill", at("release", 540 - 320, 302 - 240), "update"])   # its release must not answer
ok(s["dialog_open"] and s["result"] is None, s)

e = case("QD-cancel-return")
s = scenario(["open", "key_left", "update:300", "key_escape", "update"])
ok(s["selected"] == 1 and s["marker"] == 596.0 and s["pills_drawn"] and s["game_paused"], s)
ok(e["pause_has_input_after_ms"] == 0 and e["pause_caption_unchanged"] and e["pause_marker_unchanged"])
t = run("quit-text", 1)
ok(t["caption"] == vec["pause_quit_item_caption"]["english"], t)

case("QD-reopen")
a = present(["open", "update:100"])
b = present(["open", "key_left", "update:300", "key_escape", "open", "update:100"])
ok(a == b, a, b)                                                       # fresh focus, fresh pulse phase
s = scenario(["open", "key_left", "key_escape", "open", "key_enter"])
ok(s["opens"] == 2 and s["cancels"] == 2 and s["result"] == "cancel" and s["quit_invoked"] == 0, s)

case("QD-single-dispatch")
s = scenario(["open", "key_left", "key_enter", "repeat_enter", "key_enter", at("click", *LEFT), "key_escape",
              "update", "update"])
ok(s["quit_invoked"] == 1 and s["confirms"] == 1 and s["resume_actions"] == 0, s)
s = scenario(["open", "key_left", at("press", *LEFT), at("release", *LEFT), at("press", *LEFT), at("release", *LEFT), "update"])
ok(s["quit_invoked"] == 1 and s["confirms"] == 1, s)

# ---- text -----------------------------------------------------------------------------------------------------------
e = case("QD-text")
t = run("quit-text", 1)
ok(t["message"] == e["message"] and t["left"] == e["left_label"] and t["right"] == e["right_label"], t)
ok("^" not in t["message"] and e["message_lines"] == 1 and t["message"] != S["CONFIRM_QUIT_TO_MAIN_LOST_PROGRESS"]["english"], t)
ok(t["picture"] is True, t)
s = present(["open"])
ok(s["message"] == e["message"] and s["left_label"] == e["left_label"] and s["right_label"] == e["right_label"], s)
ok("PLACEHOLDER" not in json.dumps(s) and e["placeholder_text_visible"] is False)
ok(e["lost_progress_variant_0xE15B558A_used"] is False)
t = run("quit-text", 3)                                                # only the equivalent alternates present
ok(t["left"] == "Yes*" and t["right"] == "No*" and t["picture"] is True, t)
t = run("quit-text", 2)                                                # no string table entries at all
ok(t["dialog_open"] and not t["picture"] and t["message"] is None and "string-table text missing" in t["status"], t)
t = run("quit-text", 0)                                                # no package: logic only, status reported
ok(t["dialog_open"] and not t["picture"] and "unavailable" in t["status"], t)
r = scenario(["open", "key_left", "key_enter", "update"], mode=0)
ok(r["quit_invoked"] == 1 and r["quit_frame"] == 0 and r["picture"] is False, r)
r = scenario(["open", "key_escape"], mode=2)
ok(r["result"] == "cancel", r)

e = case("QD-pause-caption")
t = run("quit-text", 1)
ok(t["caption"] == e["caption"] and t["caption_status"] == "", t)
t = run("quit-text", 4)                                                # table present, caption key absent
ok(t["caption"] == "Quit Game" and "unavailable" in t["caption_status"], t)    # host label stays, reported
t = run("quit-text", 2)                                                # no table entries at all: host label, nothing invented
ok(t["caption"] == "Quit Game" and t["caption_status"] == "", t)
ok(e["activation_opens"].startswith("quit dialog"))
run("quit-draw")

missing = set(CASES) - used
ok(not missing, missing)
print(f"quit dialog vectors: PASS ({len(CASES)} cases, {checks} checks) on synthetic fixtures")
print("Not verified without the retail inputs: package/art hashes and presence, the authored enter fade "
      "(right button alpha 255 at rest), real glyph rendering, darkening factor 0.40, 1920x1080 / 1280x960 captures.")
