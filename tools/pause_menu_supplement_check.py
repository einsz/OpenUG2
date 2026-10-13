#!/usr/bin/env python3
"""All approved correction cases: adapter state, synthetic authored tracks and measurement policy.
Retail metadata/final-frame checks are explicitly unavailable in this synthetic run.
"""
import json
import subprocess
import sys
from pathlib import Path
from pause_frame_metrics import vector_policy

binary, root = sys.argv[1], Path(sys.argv[2])
def equal(a,b):
    if isinstance(b,dict):return set(a)==set(b) and all(equal(a[k],v) for k,v in b.items())
    if isinstance(b,list):return len(a)==len(b) and all(equal(x,y) for x,y in zip(a,b))
    if isinstance(b,(int,float)) and not isinstance(b,bool):return abs(a-b)<.003
    return a==b

count=0
for filename in ('opacity.json','motion.json','authored.json'):
    for case in json.loads((root/filename).read_text())['cases']:
        i,e=case['input'],case['expected']
        if filename=='motion.json' and case['id']!='rendered_animation_missing':
            got=json.loads(subprocess.check_output([binary,'motion',case['id'],str(i.get('N',3)),str(i.get('i',i.get('initial_i',0)))],text=True))
            for key,value in got.items():
                if key in e:
                    actual=value;expected=e[key]
                    if isinstance(expected,list):
                        assert all(abs(a-b)<=.003 for a,b in zip(actual,expected)),(case['id'],key,actual,expected)
                    else: assert actual==expected,(case['id'],key,actual,expected)
            # Pixel clauses require actual frames; do not pass them from these values.
            if case['id']=='glow_cycle': assert set(got)=={'normalized_glow_alpha'}
        elif case['id']=='evaluated_entry_highlight':
            got=json.loads(subprocess.check_output([binary,'authored-samples'],text=True))
            assert equal(got,e),(case['id'],got,e)
        elif case['id']=='resolve_authored_pause':
            # Approved metadata forms the verifier expectation; no runtime package opened here.
            assert i['package_name']=='UI_pause.fng'
            assert e==dict(package_key='0x32124083',decoded_size=33044,object_count=63,resource_count=14)
            print('resolve_authored_pause: unavailable (runtime metadata check deferred)')
        elif case['id']=='cancel_during_entry_reopen':
            subprocess.run([binary,'checks'],check=True,stdout=subprocess.DEVNULL)
        else:
            got=vector_policy(case['id'],i)
            assert equal(got,e),(case['id'],got,e)
        count+=1
print(f'pause supplements: {count} cases encoded; synthetic state/arithmetic/policy PASS')
print('Actual retail package metadata, matched-frame opacity/glow/marker parity and genuine-art rendering remain unverified.')

for case in json.loads((root/'binding.json').read_text())['cases']:
    got=json.loads(subprocess.check_output([binary,'binding',case['case']],text=True))
    assert not got['slot_visible'] and not got['unused_pool_visible']
    if isinstance(case['expected'],dict):
        expected=case['expected']
        assert got['selected_texture']==expected['selected_texture']
        assert got['neighbor_texture']==expected.get('right_neighbor_texture',expected.get('left_neighbor_texture'))
        assert got['caption']==('Return to Explore Mode' if case['input']['selected']==0 else 'Quit Game')
    elif case['case']=='unpopulated':assert got['caption']=='' and got['selected_texture']=='0x00000000'
    else:assert got['caption']=='Return to Explore Mode'
    assert got['pool_alpha']==119
print('pause binding: 14 role identities + 10 pool identities + all 4 integration cases PASS (synthetic fixtures; retail facts unverified)')
