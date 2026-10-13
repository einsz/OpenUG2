#!/usr/bin/env python3
"""Run every case of pause_menu_vectors/behavior.json (BEHAVIOR.md: item-row slide, snapping marker,
arrow rules, pointer rules, immediate resume) against the real adapter via build/pause_menu_test.
The vector file is a verbatim copy of the approved vectors; this script supplies the drivers."""
import json
import subprocess
import sys
from pathlib import Path

binary, root = sys.argv[1], Path(sys.argv[2])
vec = json.loads((root / "behavior.json").read_text())
POS = vec["position_tolerance_virtual"]
SCROLL = vec["scroll_position_tolerance_items"]
ALPHA_BYTES = vec["alpha_tolerance_bytes"]
CATALOGUE = "resume,options,quit"


def run(*args):
    return json.loads(subprocess.check_output([binary, *map(str, args)], text=True))


def near(a, b, tol, what):
    assert abs(a - b) <= tol, (what, a, b, tol)


def near_list(a, b, tol, what):
    assert len(a) == len(b), (what, a, b)
    for x, y in zip(a, b):
        near(x, y, tol, what)


def events(spec):
    """[[name, t_ms], ...] or [name, ...] -> 't:name' event string."""
    out = []
    for k, item in enumerate(spec):
        name, t = item if isinstance(item, list) else (item, 10 * k)
        out.append(f"{t}:{name}")
    return ",".join(out)


def timeline(catalogue, start, ev, samples):
    return run("behavior-timeline", catalogue, start, ev or "-", *samples)


def sample_times(keys):
    return sorted(int(k) for k in keys)


def at(res, t):
    return res["samples"][str(t)]


count = 0

for case in vec["cases"]:
    cid = case["id"]

    if cid in ("authored_right_arrow_blink", "authored_left_arrow_blink"):
        side = "right" if "right" in cid else "left"
        got = run("behavior-blink", side)
        for exp, g in zip(case["samples"], got["samples"]):
            assert exp["t_ms"] == g["t_ms"], cid
            if exp["t_ms"] < 400:  # at 400 ms the script has ended and chained to the idle state
                near_list(g["rgba"][:3], exp["rgba"][:3], 1, cid + " rgb")
            near(g["rgba"][3], exp["rgba"][3], ALPHA_BYTES, cid + " alpha")
            near_list(g["size"], exp["size"], 0.01, cid + " size")
        # the 400 ms sample may be the end of the blink or the chained idle state
        static = case["after_blink_static"]
        near_list(got["after_blink_static"]["rgba"], static["rgba"], ALPHA_BYTES, cid + " idle")
        near_list(got["after_blink_static"]["size"], static["size"], 0.01, cid + " idle size")
        assert got["hidden_arrow_blinks"] == 0, "hidden wins over blink (R3.6)"
        # centre/quaternion placement is package data that the adapter must not override
        assert case["hidden_state"]["alpha"] == 0 and case["hidden_state"]["duration_ms"] == 0

    elif cid == "authored_exit_fade":
        fade = case["fade"]
        # Package data: every authored fade is fully transparent by 100 ms (the root group by 80 ms)
        # and the exit-complete event (320 ms) is presentation only; resume never waits for either.
        for name, f in fade.items():
            end = 80 if name == "package_root_group" else 100
            assert f["alpha"][str(end)] == 0 and f["alpha"]["140"] == 0, name
        assert case["item_activation_sequence"]["exit_complete_event_at_ms"] == 320
        res = run("behavior-resume", "Resume_item_Enter")
        assert not res["closing_after_input"] and res["overlay_alpha_zero_by_ms_after_input"] <= 100

    elif cid in ("marker_snap_2_items", "marker_snap_5_items_rapid"):
        n, start = case["catalogue_N"], case["start_i"]
        ev = events(case["inputs"] if "inputs" in case else [[case["input"].split()[0], 0]])
        exp = case["expected_marker_center"]
        got = timeline(n, start, ev, sample_times(exp))
        for t, xy in exp.items():
            near_list(at(got, t)["marker"], xy, POS, f"{cid}@{t}")

    elif cid == "arrow_visibility_table":
        for row in case["rows"]:
            got = at(timeline(row["N"], row["i"], None, [-1]), -1)
            assert got["left"] == row["left"] and got["right"] == row["right"], row

    elif cid == "arrow_hides_on_reaching_end":
        e = case["expected"]
        got = timeline(case["catalogue_N"], case["start_i"], events(case["inputs"]), sample_times(e["right_arrow_alpha_at_ms"]))
        for t, alpha in e["right_arrow_alpha_at_ms"].items():
            visible = at(got, t)["right"]
            assert (150 if visible else 0) == alpha, (t, alpha, visible)
        assert at(got, 0)["i"] == e["i"] and at(got, 0)["left"] == e["left_arrow_visible"]

    elif cid in ("boundary_no_wrap_right", "boundary_no_wrap_left", "singleton"):
        e = case["expected"]
        inputs = case.get("inputs") or [case["input"]]
        n, start = case["catalogue_N"], case["start_i"]
        before = at(timeline(n, start, None, [-1]), -1)
        got = timeline(n, start, events(inputs), [-1, 0, 50, 300])
        after = at(got, 300)
        assert after["i"] == e["i"]
        assert after["s"] == float(start), "no slide may start"
        near_list(after["marker"], e.get("marker_center", [50.0, 188.4]), POS, cid)
        assert after["left"] == e["left_arrow_visible"] and after["right"] == e["right_arrow_visible"]
        assert got["blinks"] == 0 and after["caption"] == before["caption"] and got["invoked"] == []
        assert at(got, 0)["s"] == float(start)

    elif cid == "slide_one_step_table":
        rows = case["rows"]
        got = timeline(2, 0, "0:Right", [r["t_ms"] for r in rows])
        for r in rows:
            s = at(got, r["t_ms"])
            near(s["s"], r["fraction_covered"], SCROLL, f"s@{r['t_ms']}")
            near(s["items"][0]["x"] - 320.388, -r["offset_virtual"], POS, f"offset@{r['t_ms']}")
            near_list([s["items"][0]["w"], s["items"][0]["h"]], r["outgoing_icon_size"], 22 * SCROLL, "outgoing")
            near_list([s["items"][1]["w"], s["items"][1]["h"]], r["incoming_icon_size"], 22 * SCROLL, "incoming")
        # The slide ends (snaps) at about 234 ms and not before.
        end = timeline(2, 0, "0:Right", [233, 234])
        assert at(end, 233)["s"] < 1.0 and at(end, 234)["s"] == 1.0
        near(case["snap_time_ms_one_step"], 233.9, 0.1, "snap time")

    elif cid == "icon_alpha_vs_distance":
        for row in case["rows"]:
            got = run("behavior-place", row["distance_items"])
            near(got["alpha"], row["alpha"], 0.001, f"alpha d={row['distance_items']}")
            near_list(got["size"], row["size"], 0.001, f"size d={row['distance_items']}")

    elif cid == "slide_right_one_step_items":
        n = case["catalogue_N"]
        times = sample_times(case["expected_scroll_position_s"])
        got = timeline(n, case["start_i"], events(case["inputs"]), times)
        for t in times:
            s = at(got, t)
            near(s["s"], case["expected_scroll_position_s"][str(t)], SCROLL, f"s@{t}")
            for k in range(n):
                it = s["items"][k]
                near(it["x"], case["expected_item_center_x"][str(t)][f"item{k}"], POS, f"x item{k}@{t}")
                near(it["y"], case["item_center_y"], POS, "y")
                near_list([it["w"], it["h"]], case["expected_item_size"][str(t)][f"item{k}"], 22 * SCROLL, f"size item{k}@{t}")
                near(it["alpha"], case["expected_item_alpha"][str(t)][f"item{k}"], 0.004, f"alpha item{k}@{t}")
            # caption, marker and arrows are logically current from the input frame (R1.8)
            assert s["i"] == 1 and s["caption"] == "item1"
        assert case["caption_marker_arrows_change_at_ms"] == 0
        near_list(at(got, 0)["marker"], [50 + 546 / (n - 1), 188.4], POS, "marker at input frame")

    elif cid == "visible_at_rest":
        for row in case["rows"]:
            s = at(timeline(row["N"], row["i"], None, [-1]), -1)
            visible = [k for k, it in enumerate(s["items"]) if it["alpha"] > 0]
            assert visible == row["visible_items"], (row, visible)

    elif cid in ("slide_retarget_mid_animation", "slide_reversal_mid_animation"):
        times = sample_times(case["expected_scroll_position_s"])
        got = timeline(case["catalogue_N"], case["start_i"], events(case["inputs"]), times)
        for t in times:
            near(at(got, t)["s"], case["expected_scroll_position_s"][str(t)], SCROLL, f"{cid}@{t}")
        final = at(got, times[-1])
        assert final["i"] == case["final_i"]
        if cid == "slide_retarget_mid_animation":
            # no jump: s is continuous across the second input at 50 ms
            near(at(got, 49)["s"], at(got, 50)["s"], 0.02, "continuity")

    elif cid == "slide_frame_rate_independence":
        got = run("behavior-substeps")
        near(got["s_one_step"], case["s_at_100ms"], SCROLL, "one step")
        near(got["s_six_steps"], case["s_by_6_substeps"], SCROLL, "six steps")
        near(got["s_one_step"], got["s_six_steps"], 1e-4, "composition")

    elif cid.startswith("pointer_"):
        e = case["expected"]
        start = case.get("start_i", 1)
        g = case.get("gesture", {})
        evs = []
        if "press" in g:
            (x, y), t = g["press"]; evs.append(f"{t}:press:{x}:{y}")
            (x, y), t = g["release"]; evs.append(f"{t}:release:{x}:{y}")
        elif "move_over" in g:
            evs = [f"{10 * k}:hover:{x}:{y}" for k, (x, y) in enumerate(g["move_over"])]
        elif "secondary_click" in g:
            evs = ["0:right_click"]
        pre = events(case["inputs"]) if "inputs" in case else None
        spec = ",".join(x for x in [pre] + evs if x)
        end = max(int(s.split(":")[0]) for s in spec.split(",")) + 5
        got = timeline(CATALOGUE, start, spec, [-1, end])
        before, after = at(got, -1), at(got, end)
        if "invoked_at_press" in e:
            assert got["invoked"] == e["invoked_at_release"] and got["invoked_event_indexes"] == [1], "activation on release only"
            assert e["invoked_at_press"] == []
            assert e["invocations"] == len(got["invoked"]) and e["selection_changed"] == (after["i"] != start)
        elif "invoked" in e:
            assert got["invoked"] == e["invoked"], (cid, got["invoked"])
        if "i" in e:
            assert after["i"] == e["i"], (cid, after["i"])
        if "slide_started" in e:
            assert (after["s"] != float(after["i"])) == e["slide_started"], (cid, after["s"])
        if "marker_center" in e:
            near_list(after["marker"], e["marker_center"], POS, cid)
        if "caption" in e:
            assert after["caption"] == e["caption"]
        if "paused" in e:
            assert after["paused"] == e["paused"]
        if "arrow_blink_started" in e:
            assert (got["blinks"] > 0) == e["arrow_blink_started"]
        if "visual_change" in e:
            assert got["signature_changed"] == e["visual_change"]
        if "paused_after_input_frame" in e:  # secondary button
            assert after["paused"] == e["paused_after_input_frame"] and got["invoked"] == ["back/resume"]
            assert e["selected_item_invoked"] is False and after["screen"] == 3, "never activates the selected item"
        if cid == "pointer_center_during_slide":
            assert got["invoked"] == e["invoked"] and len(got["invoked"]) == e["invocations"]

    elif cid.startswith("resume_immediate_"):
        trigger = case["trigger"]
        e = case["expected"]
        got = run("behavior-resume", trigger)
        assert got["paused_after_processing_frame"] == e["paused_after_processing_frame"], (cid, got)
        for key in ("simulation_advances_from_frame", "gameplay_clock_advances_from_frame",
                    "gameplay_input_accepted_from_frame", "audio_resumed_from_frame",
                    "triggering_input_forwarded_to_gameplay", "menu_accepts_input_after_resume"):
            assert got[key] == e[key], (cid, key, got[key], e[key])
        assert got["max_resume_latency_frames"] <= e["max_resume_latency_frames"]
        assert got["overlay_alpha_zero_by_ms_after_input"] <= e["overlay_alpha_zero_by_ms_after_input"]
        assert not got["closing_after_input"], "no fade/timer may gate resume"

    else:
        raise AssertionError(f"unhandled behavior vector {cid}")
    count += 1

print(f"pause behavior vectors: PASS ({count} cases from behavior.json against the real adapter)")
print("Host-loop vectors run through a model of src/main.c's frame order; audio/clock follow the same RESUME action.")
