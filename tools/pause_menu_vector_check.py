#!/usr/bin/env python3
"""Run every approved pause vector against the actual engine adapter."""
import json
import subprocess
import sys
from pathlib import Path

binary, spec = sys.argv[1], Path(sys.argv[2])
subprocess.run([binary,"checks"],check=True)

def compare(expected, actual, path="", visual_tolerance=None):
    for key, value in expected.items():
        got = actual[key]
        here = path + "/" + key
        if isinstance(value, dict):
            compare(value, got, here, visual_tolerance)
        elif isinstance(value, list):
            assert len(value) == len(got), here
            tolerance = visual_tolerance if visual_tolerance and ("visible_bounds" in key or key == "virtual_ink_bounds") else .001
            assert all(abs(a-b) <= tolerance for a,b in zip(value, got)), (here,value,got)
        elif isinstance(value, bool) or isinstance(value, str):
            assert value == got, (here,value,got)
        else:
            assert abs(value-got) <= .001, (here,value,got)

total=0
superseded=0
# BEHAVIOR 4.4 supersedes interaction.json's neighbour-icon click selection (pointer on a
# neighbouring icon does nothing); behavior.json pointer_click_inert_* cases replace it.
SUPERSEDED_EVENTS={"configuration_icon_click"}
for case in json.loads((spec/"geometry.json").read_text())["cases"]:
    w,h=case["input"]["viewport"]
    out=subprocess.check_output([binary,"geometry",str(w),str(h)],text=True)
    compare(case["expected"],json.loads(out));total+=1
for case in json.loads((spec/"interaction.json").read_text())["cases"]:
    i=case["input"]
    if i["event"] in SUPERSEDED_EVENTS:
        superseded+=1;continue
    out=subprocess.check_output([binary,"interaction",i["event"],i.get("selected","resume")],text=True)
    compare(case["expected"],json.loads(out));total+=1
for index, case in enumerate(json.loads((spec/"corrections.json").read_text())["cases"]):
    out=subprocess.check_output([binary,"correction",str(index)],text=True)
    compare(case["expected"],json.loads(out),visual_tolerance=2);total+=1
print(f"pause vectors: PASS ({total} cases, {superseded} superseded by behavior.json; synthetic configuration follows existing Fe dispatch)")
print("Retail resource metadata/alias equality and rendered fidelity remain unresolved without verifier runtime inputs; correction resources above use synthetic fixtures.")
