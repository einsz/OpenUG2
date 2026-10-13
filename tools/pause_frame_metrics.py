#!/usr/bin/env python3
"""Measurement and acceptance policy from approved OPACITY/AUTHORED/MOTION specs.
No assets or reference pixels are embedded. CLI consumes verifier captures only.
"""
import json
import hashlib
import math
import sys
from pathlib import Path

BANDS={'outer':(.06,.14,0,3),'rail':(0,.035,60,66),'selector':(.03,.09,48,56),'caption':(0,.035,40,46)}
ROIS={'outer_title':[110,171,220,179],'outer_footer':[240,292,390,310],
      'rail':[100,187,220,190],'selector':[100,211,220,246],'caption':[100,265,220,278]}

def band_pass(surface,t,f):
    lo,hi,flo,fhi=BANDS[surface]
    return all(lo<=x<=hi for x in t) and all(flo<=x<=fhi for x in f) and max(t)-min(t)<=.015 and max(f)-min(f)<=2

def motion_parity(i):
    if not i.get('actual_timed_matched_render_frames_available',False): return 'unverified'
    keys=('reference_peak_phase_glow_value','reference_second_phase_glow_value','production_peak_phase_glow_value','production_second_phase_glow_value')
    if not all(k in i for k in keys):return 'unverified'
    a,b,c,d=(i[k] for k in keys)
    if abs(a-b)<=2 or abs(c-d)<=2:return 'fail'
    if max(abs(a-c),abs(b-d))>2:return 'fail'
    if i.get('matched_phase_subset_fraction_within_2_levels',0)<.95:return 'fail'
    if i.get('marker_center_error_virtual',math.inf)>1:return 'fail'
    return 'pass' if i.get('all_other_motion_checks_pass',False) else 'unverified'

def vector_policy(case,i):
    if 'T_rgb' in i:
        surface=next((k for k in BANDS if k in case),'outer')
        result={'target_band_pass':band_pass(surface,i['T_rgb'],i['F_rgb'])}
        if 'scene_channels' in i:
            result={'unrounded_output_channels':[i['T_rgb'][0]*v+i['F_rgb'][0] for v in i['scene_channels']],**result}
        return result
    if case=='outside_global_dimming':return {'actual_opacity_acceptance':'fail' if i['outside_panel_pair_agreement_fraction_within_1_level']<.99 else 'pass'}
    if case=='dark_scene_fit':return {'actual_opacity_acceptance':'unresolved_insufficient_variation' if min(i['B_p95_minus_p5_channels'])<64 else 'unverified'}
    if case in ('single_unpaired_screenshot','state_only_alpha'):return {'actual_opacity_acceptance':'unverified'}
    if case=='authored_pair_requirement':return {'authored_opacity_parity':'unverified'}
    if case=='authored_parity_positive':return {'matched_composite_parity':all(abs(x)<=.015 for x in i['production_minus_reference_T_rgb']) and all(abs(x)<=3 for x in i['production_minus_reference_F_rgb']) and i['all_other_pair_checks_pass']}
    if case=='manual_override_negative':return {'authored_integration_acceptance':not i['supported_authored_property_overridden']}
    if case.startswith('authored_motion_'):return {'authored_motion_parity':motion_parity(i)}
    if case=='rendered_animation_missing':return {'animation_acceptance':'fail' if i['actual_frames_identical_across_glow_phases'] else 'unverified'}
    raise ValueError('Unsupported case '+case)

def capture_fit(manifest):
    import numpy as np
    from PIL import Image
    data=json.loads(Path(manifest).read_text())
    pairs=data['pairs'];results=[]
    if not pairs:raise ValueError('No actual frame pairs')
    root=Path(manifest).resolve().parent
    def read(name):
        return np.asarray(Image.open(root/name).convert('RGB'),dtype=np.float64)
    # Phase and viewport are never pooled together across animation phases.
    groups={}
    for pair in pairs:groups.setdefault((tuple(pair['viewport']),pair['ui_ms']),[]).append(pair)
    for (viewport,phase),group in groups.items():
        w,h=viewport;s=min(w/640,h/480);ox=(w-640*s)/2;oy=(h-480*s)/2
        images=[];outside=[];refs=[]
        for pair in group:
            off,on=read(pair['off']),read(pair['on'])
            if on.shape!=off.shape or on.shape[:2]!=(h,w):raise ValueError('Unmatched frame dimensions')
            images.append((off,on));refs.append(read(pair['reference']) if pair.get('reference') else None)
            y,x=np.ogrid[:h,:w]
            # The approved panel bound plus a documented two-virtual-unit native feather band.
            mask=~((x>=ox+22.4*s)&(x<ox+624.7*s)&(y>=oy+150*s)&(y<oy+324.2*s))
            outside.append(float(np.mean(np.max(np.abs(on-off),axis=2)[mask]<=1)))
        result={'viewport':viewport,'ui_ms':phase,'scene_count':len(set(p['scene'] for p in group)),
                'outside_agreement_fraction':outside,'outside_exclusion':'SPEC outer panel bounds plus 2 virtual units',
                'authored_opacity_parity':'unverified','surfaces':{}}
        all_pass=min(outside)>=.99 and result['scene_count']>=2
        for name,bounds in ROIS.items():
            x0,y0,x1,y1=bounds
            rect=(slice(math.ceil(oy+y0*s),math.floor(oy+y1*s)),slice(math.ceil(ox+x0*s),math.floor(ox+x1*s)))
            b=np.concatenate([im[0][rect].reshape(-1,3) for im in images]);m=np.concatenate([im[1][rect].reshape(-1,3) for im in images])
            span=np.percentile(b,95,axis=0)-np.percentile(b,5,axis=0)
            def fit(target):
                ts=[];fs=[];med=[];p95=[]
                for c in range(3):
                    a=np.column_stack([b[:,c],np.ones(len(b))]);t,f=np.linalg.lstsq(a,target[:,c],rcond=None)[0]
                    residual=np.abs(target[:,c]-(t*b[:,c]+f))
                    ts.append(float(t));fs.append(float(f));med.append(float(np.median(residual)));p95.append(float(np.percentile(residual,95)))
                return {'T':ts,'F':fs,'median_abs_residual':med,'p95_abs_residual':p95}
            prod=fit(m);surface=name.split('_')[0]
            row={'roi_virtual':bounds,'B_p95_minus_p5':span.tolist(),'production':prod,'diagnostic_absolute_band_pass':band_pass(surface,prod['T'],prod['F'])}
            residual_ok=max(prod['median_abs_residual'])<=2 and max(prod['p95_abs_residual'])<=5
            variation_ok=float(min(span))>=64
            row['status']='unresolved_insufficient_variation' if not variation_ok else 'unverified'
            if all(r is not None for r in refs):
                ref=fit(np.concatenate([r[rect].reshape(-1,3) for r in refs]));row['reference']=ref
                match=all(abs(a-b)<=.015 for a,b in zip(prod['T'],ref['T'])) and all(abs(a-b)<=3 for a,b in zip(prod['F'],ref['F']))
                ref_ok=max(ref['median_abs_residual'])<=2 and max(ref['p95_abs_residual'])<=5
                ok=variation_ok and residual_ok and ref_ok and match
                row['status']='pass' if ok else ('unresolved_insufficient_variation' if not variation_ok else 'fail')
                all_pass=all_pass and ok
            else:all_pass=False
            result['surfaces'][name]=row
        if all(r is not None for r in refs):result['authored_opacity_parity']='pass' if all_pass else 'fail_or_unresolved'
        results.append(result)
    hashes={}
    for pair in pairs:
        for key in ('on','off','reference','reference_off'):
            if pair.get(key):
                filename=root/pair[key]
                hashes[pair[key]]=hashlib.sha256(filename.read_bytes()).hexdigest()
    return {'source_manifest':str(Path(manifest).resolve()),'frame_sha256':hashes,'frame_color_encoding':data.get('frame_color_encoding','unspecified; verifier must record identical saved framebuffer encoding'),'measurements':results}

if __name__=='__main__':
    if len(sys.argv)!=2:raise SystemExit('usage: pause_frame_metrics.py CAPTURE_MANIFEST.json')
    print(json.dumps(capture_fit(sys.argv[1]),indent=2))
