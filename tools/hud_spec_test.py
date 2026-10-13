#!/usr/bin/env python3
"""Executable tests authored solely from the approved synthetic vectors."""
import ctypes as C,json,math,pathlib,struct,subprocess,sys,tempfile,hashlib,re
root=pathlib.Path(__file__).resolve().parents[1]
spec=pathlib.Path(sys.argv[1]) if len(sys.argv)>1 else root/'docs/cleanroom-hud'
# Renewed approval gate (widescreen policy fold-in). Pinned values must equal the
# latest approval recorded in REVIEW.md and the shipped bytes.
APPROVED_SPEC='56009554e2a4f6a3673180a98d5c5875ce3c38acd6a0fd9c5ece382024703aca'
APPROVED_VECTORS={
 # test-vectors/README.md is documentation, not a vector; its current hash is not recorded in REVIEW.md, so it is not pinned.
 'test-vectors/behavior.json':'a1ac1035ec2184af7e7738629f2c3b149fa19d0fb453cb8cfa4800f978880946',
 'test-vectors/format.json':'b0d7c11dfb105e33fe5cd8ae152a60cf7cc8594e27557cbf736da3ff00a7b53a',
 'test-vectors/synthetic-object.bin':'48284337c925fc7632109ae3c73601627f5d43e4583e1c04b37ac661640f5042',
 'test-vectors/texture.json':'7773d4ff395abd9bafea9bd498e3b784550f8fa9e1a424aea1612e789094b768',
 'test-vectors/capture1.json':'cbca785849a3e59c6c7978a4474b6ce1f13f6630b17821c2bd07aad7a1615f28',
 'test-vectors/capture2.json':'e2724ac3352a1f4a74c597965aaf98898f35e9e08f3abb4fc4c0d87cacf11505',
 'test-vectors/capture3.json':'83224533b04ab28b5aa4cb11ed3fe9d5c39e3579711721b730dc4754c1798bd7',
 'test-vectors/capture4.json':'f16c95f08d0bbf1c54ef75fefa27e8ff451e8631271b6148d89923e053166498',
 'test-vectors/capture5.json':'747e49aad23e5fa4e0441ea2cf84c491ddf933071feb651bc6a4dd60caf715f8',
 'test-vectors/capture6.json':'61b93ac77f79108114cf831c6557b625f21ae42c7e9c4a5a472e15576c1d816c',
 'test-vectors/capture7.json':'a2c313a79ce303c87443d5b3007eebb1cbd7098e6547760bf97dcfa68e8cbd67',
 'test-vectors/capture8.json':'92509edc220ec046ed6b33cb21b61edaf82982d8fd8b5b032e0fe8ddd33755ea',
 'test-vectors/wide-policy.json':'9bb2b42d1c8be319e2e335740497d8c6c92232b8ddcc8a735481435ee7ee175a',
}
review=(spec/'REVIEW.md').read_text()
recorded=re.findall(r'(?:(?:New approved|Approved) SHA-256\(SPEC\.md\): |SHA-256\(SPEC\.md\) = )`([0-9a-f]{64})`',review)
assert recorded and recorded[-1]==APPROVED_SPEC,'REVIEW.md latest approval differs from the pinned gate'
assert hashlib.sha256((spec/'SPEC.md').read_bytes()).hexdigest()==APPROVED_SPEC,'Specification changed without a renewed approval gate'
inputs={}
for relative,wanted in APPROVED_VECTORS.items():
 assert wanted in review,f'Pinned vector hash not recorded in REVIEW.md: {relative}'
 data=(spec/relative).read_bytes();assert hashlib.sha256(data).hexdigest()==wanted,f'Unapproved vector edit: {relative}';inputs[relative]=data

libpath=root/'build/libhud_spec.so';libpath.parent.mkdir(exist_ok=True)
subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-fPIC','-shared',str(root/'src/hud_asset.c'),str(root/'src/hud_texture.c'),'-lm','-o',str(libpath)],check=True)
lib=C.CDLL(str(libpath));F3=C.c_float*3
class Node(C.Structure):
 _fields_=[('kind',C.c_uint32),('name',C.c_uint32),('id',C.c_uint32),('parent_id',C.c_uint32),('parent',C.c_int),('has_parent',C.c_int),('appearance',C.c_int),('valid',C.c_int),('resource',C.c_uint16),('justification',C.c_uint16),('color',C.c_ubyte*4),('pivot',F3),('position',F3),('quaternion',C.c_float*4),('size',F3),('uv',C.c_float*4),('sample',C.c_void_p)]
class Resource(C.Structure):_fields_=[('id',C.c_uint32),('kind',C.c_uint32),('name',C.c_void_p)]
class Package(C.Structure):_fields_=[('nodes',C.POINTER(Node)),('count',C.c_size_t),('resources',C.POINTER(Resource)),('resource_count',C.c_size_t),('diagnostics',C.c_uint)]
class Glyph(C.Structure):_fields_=[('code',C.c_uint16),('x',C.c_uint16),('y',C.c_uint16),('advance',C.c_uint16),('width',C.c_ubyte),('height',C.c_ubyte),('top',C.c_ubyte),('bearing',C.c_int8)]
class Font(C.Structure):_fields_=[('name',C.c_char*256),('texture',C.c_char*256),('glyphs',C.POINTER(Glyph)),('count',C.c_size_t),('inv_w',C.c_float),('inv_h',C.c_float),('line',C.c_uint32)]
lib.ha_speed.argtypes=[C.c_double,C.c_int,C.c_char_p];lib.ha_cash.argtypes=[C.c_int,C.c_int,C.c_int,C.c_int64,C.c_char,C.c_char_p];lib.ha_rpm_angle.argtypes=[C.c_double]*3;lib.ha_rpm_angle.restype=C.c_double;lib.ha_scale.argtypes=[C.c_double];lib.ha_fit.argtypes=[C.c_int,C.c_int,C.c_float,C.c_float,C.POINTER(C.c_float)];lib.ha_map.argtypes=[C.c_double]*7+[C.POINTER(C.c_double)];lib.ha_text_start.argtypes=[C.c_float,C.c_float,C.c_uint,C.c_float];lib.ha_text_start.restype=C.c_float;lib.ha_hash.argtypes=[C.c_char_p];lib.ha_hash.restype=C.c_uint32;lib.ha_texture_hash.argtypes=[C.c_char_p];lib.ha_texture_hash.restype=C.c_uint32
lib.ha_point.argtypes=[C.POINTER(Package),C.c_size_t,C.POINTER(C.c_float),C.POINTER(C.c_float),C.c_float]
def near(a,b):
 if isinstance(a,(list,tuple)): assert len(a)==len(b);[near(x,y) for x,y in zip(a,b)]
 else:assert abs(a-b)<1e-5,(a,b)
class MapFrame(C.Structure):_fields_=[('center',C.c_double*2),('basis',C.c_double*4),('size',C.c_double*2),('world',C.c_double*2),('heading',C.c_double),('window',C.c_double),('width',C.c_int),('height',C.c_int),('enabled',C.c_int)]
lib.ha_map_pick.argtypes=[C.POINTER(MapFrame),C.c_double,C.c_double,C.c_double,C.POINTER(C.c_double)]
checks=0;gl_policies=[]
for case in json.loads(inputs['test-vectors/behavior.json'])['cases']:
 i,e=case['input'],case['expected'];name=case['name'];buf=C.create_string_buffer(64)
 if name.startswith('map-pick'):
  frame=MapFrame();frame.width,frame.height=i['viewport'];frame.enabled=i['map_enabled'];frame.center[:]=i.get('logical_map_center',[0,0]);frame.basis[:]=[1,0,0,1];frame.size[:]=i.get('logical_map_size',[100,100]);frame.world[:]=i.get('world_center',[0,0]);frame.heading=i.get('heading_radians',0);frame.window=i.get('window_world',500);picked=(C.c_double*2)();accepted=lib.ha_map_pick(C.byref(frame),*i['screen_click'],i.get('mask_coverage',1),picked);assert bool(accepted)==e['accepted'];assert not accepted or near(list(picked),e['world']) is None
 elif 'speed_mps'in i:assert lib.ha_speed(i['speed_mps'],i['metric'],buf);assert buf.value.decode()==e['text']
 elif 'gear'in i:g={'reverse':-1,'neutral':0}.get(i['gear'],i['gear']);assert lib.ha_gear(g,buf);assert buf.value.decode()==e['text']
 elif 'viewport'in i:
  out=F3();v=lib.ha_fit(*i['viewport'],*i.get('logical',[0,0]),out)
  if 'scale'in e:near(out[0],e['scale']);near(list(out)[1:],e['screen'])
  else:assert bool(v)==e['hud_visible']
 elif 'maximum_rpm'in i:assert lib.ha_scale(i['maximum_rpm'])==e['scale_rpm'];near(lib.ha_rpm_angle(i['rpm'],i['maximum_rpm'],i['authored_angle_degrees']),e['angle_degrees'])
 elif 'career'in i and 'balance'in i or name=='cash-missing':assert bool(lib.ha_cash(i['career'],'balance'in i,i['suppressed'],i.get('balance',0),b',',buf))==e['visible'];assert 'text'not in e or buf.value.decode()==e['text']
 elif 'origin'in i:out=(C.c_double*3)();assert lib.ha_map(*i['origin'],i['extent'],*i['position'],0,i['window_world'],out);near(list(out)[:2],e['uv_center']);near([out[2]]*2,e['uv_window'])
 elif 'advances'in i:near(sum(i['advances']),e['unscaled_width']);start=lib.ha_text_start(i['anchor_x'],i['scale'],i['justification'],sum(i['advances']));near(start,e['start_x']);positions=[]
 elif 'names'in i:assert [lib.ha_hash(s.encode()) for s in i['names']]==e['hashes']
 elif name.startswith('skin-') and 'requested_resource_present'not in i:assert lib.ha_skin(i['skin'])==e['skin']
 elif name=='pivot-orbit':
  n=Node(kind=1,valid=1,parent=-1);n.quaternion[3]=1;n.size[:]=[1,1,1];n.position[:]=[*i['position'],0];n.pivot[:]=[*i['pivot'],0];p=Package(C.pointer(n),1,None,0,0);out=F3();lib.ha_point(C.byref(p),0,F3(*i['vertex'],0),out,math.radians(i['angle_degrees']));near(list(out)[:2],e['point'])
 elif name=='independent-feature-visibility':gl_policies.append(name);continue
 elif name=='skin-resource-fallback':gl_policies.append(name);continue
 else:raise AssertionError(name)
 if 'advances'in i:
  for a in i['advances']:positions.append(start);start+=a*i['scale']
  near(positions,e['next_positions'])
 checks+=1
for v in [math.nan,math.inf,-math.inf,1e100]:
 buf=C.create_string_buffer(32);assert not lib.ha_speed(v,1,buf);assert not buf.value
for v in [math.nan,math.inf,-math.inf]:near(lib.ha_rpm_angle(v,6000,-20),0)
for coord in [math.nan,math.inf,-math.inf]:
 for j in [3,4,5]:args=[0,0,100,0,0,0,500];args[j]=coord;assert not lib.ha_map(*args,(C.c_double*3)())
assert not lib.ha_gear(-2,C.create_string_buffer(32))
f=json.loads(inputs['test-vectors/format.json']);raw=inputs['test-vectors/synthetic-object.bin'];n=Node();assert lib.ha_object(raw,len(raw),C.byref(n));e=f['synthetic_object']['expected'];assert (n.kind,n.name,n.id)==(e['type'],e['name_id'],e['identity']);assert list(n.color)==e['color_rgba'];near(list(n.position),e['position']);assert not n.has_parent
nodes=(Node*2)()
for j,r in enumerate(f['hierarchy']['nodes']):
 n=nodes[j];n.id=r['identity'];n.name=r['name_id'];n.kind=5;n.valid=1;n.parent=-1;n.has_parent=r['parent']is not None;n.parent_id=r['parent']or 0;n.position[:]=[*r['position'],0];n.size[:]=[*r.get('scale',[1,1]),1];n.quaternion[3]=1
p=Package(nodes,2,None,0,0);assert lib.ha_hierarchy(C.byref(p));out=F3();lib.ha_point(C.byref(p),1,F3(),out,0);near(list(out)[:2],f['hierarchy']['expected_child_position'])
nodes[0].has_parent=1;nodes[0].parent_id=8;assert not lib.ha_hierarchy(C.byref(p));assert not nodes[0].valid and not nodes[1].valid
assert not lib.ha_object(b'FOb\xea'+struct.pack('<I',100)+b'xxxx',12,C.byref(Node()))
def block(tag,b):return tag+struct.pack('<I',len(b))+b
hdr=f['package_selection'];h=bytearray(hdr['payload_size']);struct.pack_into('<II',h,16,hdr['name_count'],hdr['path_count']);h[24:34]=hdr['name_bytes_ascii'].encode();h[34:44]=hdr['path_bytes_ascii'].encode();assert lib.ha_header_name(bytes(h),len(h),hdr['expected_name'].encode())==1
internal=block(b'PkHd',h);package=block(struct.pack('<I',0x30203),b'FEn\xe7'+struct.pack('<I',len(internal))+internal);p=Package();assert lib.ha_package_select(package,len(package),b'SYNTH.fng',C.byref(p));lib.ha_free(C.byref(p));assert not lib.ha_package_select(package*2,len(package)*2,b'SYNTH.fng',C.byref(p));assert not lib.ha_package_select(package[:-1],len(package)-1,b'SYNTH.fng',C.byref(p))
requests=struct.pack('<I',1)+struct.pack('<IIIIII',1,99,1,0,0,0);bad=internal+block(b'Res\xcc',block(b'RsNm',b'x\0')+block(b'RsRq',requests));package=block(struct.pack('<I',0x30203),b'FEn\xe7'+struct.pack('<I',len(bad))+bad);assert lib.ha_package_select(package,len(package),b'SYNTH.fng',C.byref(p));assert p.diagnostics and p.resources[0].kind==2**32-1;lib.ha_free(C.byref(p))
for r in f['image_resolution']['cases']:assert lib.ha_texture_hash(r['alias'].encode())==r['expected_texture_hash']
g=f['synthetic_glyph'];header=f['synthetic_font_header'];data=bytearray(512+header['enclosing_fntf_extent']);data[:6]=b'font\0\0';data[256:262]=b'atlas\0';data[512:516]=b'FNTF';struct.pack_into('<H',data,522,header['glyph_count']);struct.pack_into('<II',data,536,header['header24_byte_offset'],header['header28_byte_extent']);struct.pack_into('<ff',data,576,1/g['atlas_size'][0],1/g['atlas_size'][1]);offset=640;struct.pack_into('<HBBHHBbB3xH',data,offset,g['char_code'],g['width'],g['height'],*g['atlas_xy'],0,g['bearing_x'],g['top_offset'],g['advance']);font=Font();assert lib.ha_font(bytes(data),len(data),C.byref(font));q=font.glyphs[0];near([q.x*font.inv_w,q.y*font.inv_h,(q.x+q.width)*font.inv_w,(q.y+q.height)*font.inv_h],g['expected_uv_before_inset']);C.CDLL(None).free(font.glyphs);data[offset+9]=0xfe;assert lib.ha_font(bytes(data),len(data),C.byref(font));assert font.glyphs[0].bearing==-2;C.CDLL(None).free(font.glyphs)
struct.pack_into('<I',data,536,1);assert not lib.ha_font(bytes(data),len(data),C.byref(Font()))
print(f'{checks} direct approved behavior vectors plus format/malformed/nonfinite tests passed; {len(gl_policies)} rendering policies require the GL probe')

# Deterministic malformed-input exploration derived from the synthetic fragment.
import random
rng=random.Random(123)
for _ in range(2000):
 fragment=bytearray(raw)
 for j in range(rng.randrange(1,5)):
  index=rng.randrange(len(fragment));fragment[index]=rng.randrange(256)
 if rng.randrange(2):fragment=fragment[:rng.randrange(len(fragment)+1)]
 node=Node();lib.ha_object(bytes(fragment),len(fragment),C.byref(node))
 if node.sample:C.CDLL(None).free(C.c_void_p(node.sample))
print('2000 deterministic malformed descriptor variants completed safely')

# Exercise the actual bounded catalog walker with fresh synthetic slot records
# in separate texture packages; parsed font-table presence is independent.
catalog=f['multi_package_catalog']
def catalog_fixture(packages):
 return b''.join(block(struct.pack('<I',0xb3300000),block(struct.pack('<I',0xb3310000),block(struct.pack('<I',0x33310003),b''.join(struct.pack('<IIIIII',t['texture_id'],0,0,0,0,0) for t in p['textures'])))) for p in packages)
probe=root/'build/hud_asset_probe'
if probe.exists():
 for packages,expected in [(catalog['packages'],[1,1]),(catalog['missing_atlas_variant']['packages'],[1,0])]:
  with tempfile.NamedTemporaryFile(suffix='.bin') as fixture:
   fixture.write(catalog_fixture(packages));fixture.flush()
   result=subprocess.check_output([str(probe),'--catalog',fixture.name,*map(str,catalog['required_texture_ids'])],text=True)
   assert list(map(int,result.split()))==expected
 print('later-package catalog and independently missing font-atlas cases passed')
else:raise AssertionError('Build hud-asset-probe before running complete catalog tests')

# Newly approved texture contracts use no original pixel or catalog samples.
texture=json.loads(inputs['test-vectors/texture.json'])
class Pixels(C.Structure):_fields_=[('rgba',C.POINTER(C.c_ubyte)),('width',C.c_uint),('height',C.c_uint),('format',C.c_uint32)]
lib.ha_texture_decode.argtypes=[C.c_uint32,C.c_uint,C.c_uint,C.c_char_p,C.c_size_t,C.c_char_p,C.c_size_t,C.POINTER(Pixels)]
lib.ha_texture_find.argtypes=[C.c_char_p,C.c_size_t,C.c_uint32,C.POINTER(Pixels)]
def rgba(out):return [list(out.rgba[i*4:i*4+4]) for i in range(out.width*out.height)]
def release(out):C.CDLL(None).free(out.rgba)
palette=bytearray(1024)
indexed=texture['indexed'];indices=bytes(indexed['indices'])
for i,pixel in enumerate(indexed['palette_entries_bgra']):palette[i*4:i*4+4]=bytes(pixel)
out=Pixels();assert lib.ha_texture_decode(41,2,2,indices,4,bytes(palette),1024,C.byref(out))==1;assert rgba(out)==indexed['expected_rgba'];release(out)
dxt3=int.from_bytes(b'DXT3','little');compressed=bytes.fromhex(texture['dxt3']['block_hex']);assert lib.ha_texture_decode(dxt3,4,4,compressed,len(compressed),None,0,C.byref(out))==1;assert rgba(out)==[texture['dxt3']['expected_rgb_each_pixel']+[a] for a in texture['dxt3']['expected_alpha_row_major']];release(out)
low=bytes.fromhex(texture['dxt3_low_first_endpoint']['block_hex']);assert lib.ha_texture_decode(dxt3,4,4,low,16,None,0,C.byref(out))==1;assert rgba(out)==[texture['dxt3_low_first_endpoint']['expected_rgba_each_pixel']]*16;release(out)
assert lib.ha_texture_decode(41,2,2,indices,4,bytes(palette),1023,C.byref(out))==-1
assert lib.ha_texture_decode(dxt3,4,4,compressed,15,None,0,C.byref(out))==-1
assert lib.ha_texture_decode(dxt3,0,4,compressed,16,None,0,C.byref(out))==-1
alignment=texture['aligned_data'];assert (alignment['payload_offset']+127)//128*128==alignment['expected_aligned_data_base'];assert alignment['expected_aligned_data_base']+alignment['image_offset']==alignment['expected_image_file_offset']
def collection_fixture(patch=None,format_count=2):
 catalogs=bytearray(124*2);formats=bytearray(32*format_count)
 # Preserve intentionally unsorted IDs and pair formats by ordinal position.
 for i,(key,fmt,io,po,stored,pal,top,w,h) in enumerate([(91,41,0,4,4,1024,4,2,2),(12,dxt3,1028,0,16,0,16,4,4)]):
  struct.pack_into('<I',catalogs,i*124+36,key);struct.pack_into('<IIIIIHH',catalogs,i*124+48,io,po,stored,pal,top,w,h)
  if i<format_count:struct.pack_into('<I',formats,i*32+20,fmt)
 if patch:offset,value=patch;struct.pack_into('<I',catalogs,offset,value)
 metadata=block(struct.pack('<I',0xb3310000),block(struct.pack('<I',0x33310004),catalogs)+block(struct.pack('<I',0x33310005),formats))
 payload_offset=8+len(metadata)+8+8
 padding=(128-payload_offset%128)%128
 pixels=bytes(padding)+indices+bytes(palette)+compressed
 return block(struct.pack('<I',0xb3300000),metadata+block(struct.pack('<I',0xb3320000),block(struct.pack('<I',0x33320002),pixels)))
fixture=collection_fixture()
for key,expected in [(91,indexed['expected_rgba']),(12,[texture['dxt3']['expected_rgb_each_pixel']+[a] for a in texture['dxt3']['expected_alpha_row_major']])]:
 assert lib.ha_texture_find(fixture,len(fixture),key,C.byref(out))==1;assert rgba(out)==expected;release(out)
for patch in [(60,1023),(64,5),(48,2**32-1),(68,0)]:
 fixture=collection_fixture(patch);assert lib.ha_texture_find(fixture,len(fixture),91,C.byref(out))==-1
fixture=collection_fixture((124+64,15));assert lib.ha_texture_find(fixture,len(fixture),12,C.byref(out))==-1
fixture=collection_fixture(format_count=1);assert lib.ha_texture_find(fixture,len(fixture),91,C.byref(out))==-1
# A supported texture in a later complete collection must still be reached.
fixture=collection_fixture()+collection_fixture();assert lib.ha_texture_find(fixture,len(fixture),12,C.byref(out))==1;release(out)
print('approved indexed palette, DXT3 alpha/low-endpoint, alignment, ordinal pairing and six bounds cases passed')

properties=json.loads(inputs['test-vectors/behavior.json'])['map_pick_properties']
combinations=0
for width,height in properties['viewports']:
 for heading in properties['headings_radians']:
  for window in properties['window_world']:
   frame=MapFrame();frame.width=width;frame.height=height;frame.enabled=1;frame.center[:]=properties['logical_map_center'];frame.basis[:]=[1,0,0,1];frame.size[:]=properties['logical_map_size'];frame.world[:]=properties['world_center'];frame.heading=heading;frame.window=window;scale=min(width/640,height/480)
   for dx,dy in properties['logical_panel_offsets']:
    x=width/2+(frame.center[0]+dx)*scale;y=height/2+(frame.center[1]+dy)*scale;out=(C.c_double*2)();assert lib.ha_map_pick(C.byref(frame),x,y,1,out)
    expected=[frame.world[0]+(math.cos(heading)*dx/frame.size[0]-math.sin(heading)*dy/frame.size[1])*window,frame.world[1]-(math.sin(heading)*dx/frame.size[0]+math.cos(heading)*dy/frame.size[1])*window];near(list(out),expected);assert not lib.ha_map_pick(C.byref(frame),x,y,0,out);combinations+=1
   dx,dy=properties['outside_logical_panel_offset'];assert not lib.ha_map_pick(C.byref(frame),width/2+(frame.center[0]+dx)*scale,height/2+(frame.center[1]+dy)*scale,1,(C.c_double*2)())
print(f'{combinations} authored-map fitment/heading/window picking combinations and coverage/outside rejection passed')

# Calibration changes only the needle orientation, preserving its authored
# anchor and supported independent x/y reflection factors.
for quaternion in [[0,0,math.sin(.7),math.cos(.7)],[1,0,0,0],[0,1,0,0]]:
 node=Node(kind=1,parent=-1,valid=1);node.quaternion[:]=quaternion;node.pivot[:]=[3,4,0];node.position[:]=[10,20,0];assert lib.ha_needle_calibrate(C.byref(node));assert list(node.pivot)==[3,4,0] and list(node.position)==[10,20,0]
 frame=Package(C.pointer(node),1,None,0,0);out=F3();lib.ha_point(C.byref(frame),0,F3(3,4,0),out,1.2);near(list(out),[13,24,0])
 tilted=Node();tilted.quaternion[:]=[.3,.3,.3,.7];assert not lib.ha_needle_calibrate(C.byref(tilted))
print('calibrated needle zero/anchor and supported mirror preservation tests passed')

# ---------------------------------------------------------------------------
# Runtime capture amendments, capture sets 1-3 (capture1/2/3.json).
U3=C.c_ubyte*3;U4=C.c_ubyte*4
lib.ha_text_tint.argtypes=[C.c_int,C.c_int,C.c_int,C.c_double,C.c_double,U4,U3,U4]
STANDSTILL=0.5
lib.ha_dial_angle.argtypes=[C.c_double,C.c_double];lib.ha_dial_angle.restype=C.c_double
lib.ha_indicated_rpm.argtypes=[C.c_double,C.c_double];lib.ha_indicated_rpm.restype=C.c_double
lib.ha_redline.argtypes=[C.c_int,C.c_int,C.c_double,C.c_int,C.POINTER(C.c_double)]
lib.ha_shift.argtypes=[C.c_int,C.c_int,C.c_double,C.c_int,C.c_double,C.c_double]
lib.ha_nos_level.argtypes=[C.c_int,C.c_int,C.c_int,C.c_double,C.c_double,C.c_double,C.POINTER(C.c_double)]
lib.ha_bank.argtypes=[C.c_int,C.c_int,C.c_int,C.c_int64,C.c_char,C.c_int,C.c_char_p]
class Widget(C.Structure):_fields_=[(k,C.c_int) for k in ('visible','backing','dpad','centre','mail','highlight_right','diagnostic')]
lib.ha_widget.argtypes=[C.c_int,C.c_int,C.c_int,C.c_int,C.POINTER(Widget)]
class Key(C.Structure):_fields_=[('name',C.c_uint32),('kind',C.c_uint32),('parent_root',C.c_int),('parent_name',C.c_uint32),('alias',C.c_char_p)]
lib.ha_lookup.argtypes=[C.POINTER(Package),C.POINTER(Key)]
TEXT_SPEED,TEXT_GEAR=0,1
GEARS={'reverse':-1,'neutral':0}
def change_of(v):return 1 if v is True else 0  # absent, false, non-boolean: not in progress
def tint(role,gear,authored,setting,change=0,speed=None,alpha=200,standstill=STANDSTILL):
 # Earlier vectors carry no speed: gear 1 implies standstill, gear 2 moving (vectors README).
 if speed is None:speed=0 if gear==1 else 27
 out=U4();lib.ha_text_tint(role,gear,change,speed,standstill,U4(*authored,alpha),U3(*setting),out);assert out[3]==alpha,'alpha stays authored';return list(out)[:3]
lib.ha_setting_tint.argtypes=[U4,U3,U4]
def setting_tint(authored,setting):
 out=U4();lib.ha_setting_tint(U4(*authored,40),U3(*setting),out);assert out[3]==40,'alpha stays authored';return list(out)[:3]
def text_pens(anchor,size,bits,advances):
 start=lib.ha_text_start(anchor,size,bits,sum(advances));pens=[]
 for a in advances:pens.append(start);start+=a*size
 return pens,start
def bank(i,resolved=1):
 buf=C.create_string_buffer(64);r=lib.ha_bank(i['career'],'balance'in i,i['suppressed'],i.get('balance',0),b',',resolved,buf);return r,buf.value.decode()
def widget(career,pending,resolved=1,centre=1):
 w=Widget();lib.ha_widget(career,pending,resolved,centre,C.byref(w));return w
def check_widget(w,e):
 if 'widget_visible'in e:assert bool(w.visible)==e['widget_visible']
 for field,attr in [('backing_visible','backing'),('dpad_visible','dpad'),('mail_visible','mail'),('centre_figure_visible','centre')]:
  if field in e:assert bool(getattr(w,attr))==e[field],(field,e)
 if 'highlights_visible'in e:assert (['right'] if w.highlight_right else [])==e['highlights_visible']
 if 'diagnostic'in e:assert bool(w.diagnostic)==e['diagnostic']
 if 'referenced_image_drawn'in e:assert e['referenced_image_drawn'] is False  # no API path draws the referenced image
 if 'banner_text_visible'in e:assert e['banner_text_visible'] is False      # the widget contract has no text element
def north(i,e):
 # Existing provisional convention: orbit about the map centre by -heading.
 n=Node(kind=1,valid=1,parent=-1);n.quaternion[3]=1;n.size[:]=[1,1,1];n.position[:]=[i['map_center'][0],i['map_center'][1]-i['north_pivot_offset'],0];n.pivot[:]=[0,i['north_pivot_offset'],0]
 p=Package(C.pointer(n),1,None,0,0);out=F3();lib.ha_point(C.byref(p),0,F3(0,0,0),out,-i['heading_radians']);near(list(out)[:2],e['north_center'])
 d=math.degrees(-i['heading_radians']);diff=(d-e['planar_rotation_degrees']+180)%360-180;near(diff,0)
def lookup_fixture(descriptors):
 # Recipe encoding: one root group carries each distinct parent name; each
 # descriptor becomes a child node whose resource names its alias.
 parents=sorted({int(d['parent_name_id'],16) for d in descriptors});kinds={'image':1,'text':2,'group':5}
 nodes=(Node*(len(parents)+len(descriptors)))();resources=(Resource*len(descriptors))();keep=[]
 for j,name in enumerate(parents):n=nodes[j];n.kind=5;n.name=name;n.id=1000+j;n.valid=1;n.parent=-1;n.appearance=1
 for j,d in enumerate(descriptors):
  n=nodes[len(parents)+j];n.kind=kinds[d['kind']];n.name=int(d['name_id'],16);n.id=2000+j;n.valid=1;n.has_parent=1;k=parents.index(int(d['parent_name_id'],16));n.parent=k;n.parent_id=1000+k;n.resource=j;n.appearance=1
  b=C.create_string_buffer(d['alias'].encode());keep.append(b);resources[j].kind=1;resources[j].name=C.cast(b,C.c_void_p)
 return Package(nodes,len(nodes),resources,len(descriptors),0),len(parents),keep
def key_of(k):return Key(int(k['name_id'],16),{'image':1,'text':2,'group':5}[k['kind']],0,int(k['parent_name_id'],16),k.get('alias','').encode() or None)
capture_checks=0
for vector in ['capture1.json','capture2.json','capture3.json']:
 for case in json.loads(inputs['test-vectors/'+vector])['cases']:
  name=case['name'];i=case.get('input',{});e=case.get('expected',{})
  if name=='fit-1920x1080':
   for logical,screen in zip(i['logical_points'],e['screen_points']):out=F3();assert lib.ha_fit(*i['viewport'],*logical,out);near(out[0],e['scale']);near(list(out)[1:],screen)
  elif name=='glyph-right-justify':
   g=i['glyphs'];pens,_=text_pens(i['anchor'][0],i['size'],i['justification'],[q['advance'] for q in g])
   for pen,q,box in zip(pens,g,e['quads']):near([pen+q['bearing_x']*i['size'],pen+(q['bearing_x']+q['width'])*i['size'],i['anchor'][1]+q['top_offset']*i['size'],i['anchor'][1]+(q['top_offset']+q['height'])*i['size']],[box['x0'],box['x1'],box['y0'],box['y1']])
  elif name in('gear-colour-setting','gear1-colour','gear2-authored','gear-reverse-authored'):
   g=GEARS.get(i['gear'],i['gear']);buf=C.create_string_buffer(32);assert lib.ha_gear(g,buf)
   if 'gear_text'in e:assert buf.value.decode()==e['gear_text']
   if 'text'in e:assert buf.value.decode()==e['text']
   assert tint(TEXT_GEAR,g,i['authored_gear_rgb'],i['gear_colour_setting_rgb'])==e['gear_rgb']
   if 'speed_rgb'in e:assert tint(TEXT_SPEED,g,i['authored_speed_rgb'],i['gear_colour_setting_rgb'])==e['speed_rgb']
  elif name.startswith('north-heading'):north(i,e)
  elif name in('bank-zero','bank-off','bank-suppressed','bank-100','bank-25'):
   r,text=bank(i);shown=r==1
   for field in ('label_visible','backing_visible','value_visible'):
    if field in e:assert shown==e[field]
   if 'value_text'in e:assert text==e['value_text']
   if 'label_text'in e:assert (C.c_char*6).in_dll(lib,'ha_bank_label').value.decode()==e['label_text']
   if 'sibling_texts_visible'in e:assert e['sibling_texts_visible'] is False  # no sibling text is a bank target
   if 'pen_positions'in e:pens,end=text_pens(i['anchor_x'],i['size'],i['justification'],i['advances']);near(pens,e['pen_positions']);near(end,e['pen_end'])
  elif name=='redline-inferred-optin':
   for sub in case['cases']:
    si,se=sub['input'],sub['expected'];start=C.c_double();r=lib.ha_redline(si['setting_enabled'],'redline_rpm'in si,si.get('redline_rpm',0),si['scale_rpm'],C.byref(start))
    assert (r==1)==se['visible']
    if se['visible']:near(start.value,se['start_dial_angle_degrees'])
  elif name=='gauge-placement-independent':
   # Placement inputs carry no NOS/boost state; the opt-in NOS display is off
   # by default and there is no boost path at all.
   for v in i['variants']:assert lib.ha_nos_level(0,v['nos_installed'],1,1,91,239,C.byref(C.c_double()))==0
   assert e['nos_draws_in_standard_path']==0 and e['boost_draws_in_standard_path']==0 and not hasattr(lib,'ha_boost')
  elif name.startswith('widget-'):
   w=widget(i['career'],i['message_pending'],i.get('widget_targets_resolved',True),i.get('centre_texture_present',True));check_widget(w,e)
   if 'highlight_rgb'in e:assert setting_tint([255,255,255],i['highlight_colour_setting_rgb'])==e['highlight_rgb']
   if name=='widget-idle':
    assert w.centre and not w.diagnostic;missing=widget(1,0,1,0);assert not missing.centre and missing.diagnostic and missing.backing
  elif name.startswith('shift-'):
   g=GEARS.get(i['gear'],i['gear']);assert (lib.ha_shift(i['setting_enabled'],'shift_rpm'in i,i.get('shift_rpm',0),g,i['rpm'],0)==1)==e['visible']
   if e['visible']:assert setting_tint([255,255,255],i['shift_colour_setting_rgb'])==e['rgb']
  elif name=='nos-level':
   for sub in case['cases']:
    si,se=sub['input'],sub['expected'];limit=C.c_double();r=lib.ha_nos_level(si['setting_enabled'],si['nos_installed'],'level'in si,si.get('level',0),si.get('a0',91),si.get('a1',239),C.byref(limit))
    assert (r>=1)==se['backing_visible']
    if 'coloured_layer_visible'in se:assert (r==2)==se['coloured_layer_visible']
    if 'coloured_layer_max_dial_angle'in se:assert r==2;near(limit.value,se['coloured_layer_max_dial_angle'])
  elif name.startswith('lookup-'):
   p,offset,keep=lookup_fixture(i['descriptors']);k=key_of(i['key']);r=lib.ha_lookup(C.byref(p),C.byref(k))
   assert (r>=0)==e['resolved']
   if e['resolved']:assert r-offset==e['index']
  else:raise AssertionError('unhandled capture vector '+name)
  capture_checks+=1
# Defaults and diagnostics stated in the amendments but not encoded as vectors.
for v in [math.nan,math.inf,-math.inf]:
 assert lib.ha_nos_level(1,1,1,v,91,239,C.byref(C.c_double()))==-1
 assert lib.ha_redline(1,1,v,9500,C.byref(C.c_double()))==-1
 assert lib.ha_shift(1,1,v,2,7600,0)==-1
assert lib.ha_nos_level(1,1,0,0,91,239,C.byref(C.c_double()))==0,'no level supplied hides NOS'
assert lib.ha_redline(1,1,9600,9500,C.byref(C.c_double()))==0
assert bank({'career':True,'balance':-5,'suppressed':False})[0]==-1 and bank({'career':True,'balance':5,'suppressed':False},0)[0]==-1
assert bank({'career':True,'balance':1234567,'suppressed':False})[1]=='1,234,567'
assert not widget(0,0).visible and not widget(1,0,0).visible and widget(1,0,0).diagnostic
for dx,dy,deg in [(0,1,0),(-1,0,90),(0,-1,180),(1,0,270),(-1,1,45)]:near(lib.ha_dial_angle(dx,dy),deg)
near(lib.ha_indicated_rpm(math.nan,0),0);near(lib.ha_indicated_rpm(12000,8000),8000);near(lib.ha_indicated_rpm(-5,0),0)
print(f'{capture_checks} capture set 1-3 vectors (capture1/2/3.json) plus amendment default/diagnostic checks passed')

# ---------------------------------------------------------------------------
# Set 4 (implementer questions): capture4.json.
lib.ha_setting_rgba.argtypes=[U4,U4]
lib.ha_blend_over.argtypes=[U4,U4,U3,C.POINTER(C.c_double)]
lib.ha_inherited_message_visuals.argtypes=[C.c_int,C.c_int,C.c_int]
def over(texel,tint,bg):
 out=(C.c_double*3)();lib.ha_blend_over(U4(*texel),U4(*tint),U3(*bg),out);return list(out)
set4=0
for case in json.loads(inputs['test-vectors/capture4.json'])['cases']:
 name=case['name'];i=case['input'];e=case['expected']
 if name=='highlight-rgba-replaces-alpha':
  t=U4();lib.ha_setting_rgba(U4(*i['highlight_setting_rgba']),t);assert list(t)==i['highlight_setting_rgba']
  near(over(i['texel_rgba'],list(t),i['background_rgb']),e['rgb'])
 elif name=='highlight-rgb-only-contrast':
  t=U4();lib.ha_setting_tint(U4(*i['authored_tint_rgba']),U3(*i['highlight_setting_rgb_only']),t)
  got=over(i['texel_rgba'],list(t),i['background_rgb']);assert all(abs(a-b)<0.01 for a,b in zip(got,e['rgb_if_alpha_kept']))
  assert (max(abs(a-b) for a,b in zip(got,[255,190,50]))<=12)==e['conforms']
 elif name.startswith('banner-'):
  resolved=i.get('widget_targets_resolved',True);w=widget(i['career'],i['message_pending'],resolved)
  shown=lib.ha_inherited_message_visuals(0,w.visible,i['message_pending'])
  if 'inherited_message_visuals'in e:assert bool(shown)==e['inherited_message_visuals']
  if 'inherited_message_visuals_permitted'in e:assert bool(shown)==e['inherited_message_visuals_permitted']
  if 'widget_visible'in e:assert bool(w.visible)==e['widget_visible']
  if 'mail_icon'in e:assert bool(w.mail)==e['mail_icon'] and bool(w.highlight_right)==e['right_highlight']
  if 'diagnostic'in e:assert bool(w.diagnostic)==e['diagnostic']
 elif name.startswith('shift-'):
  r=lib.ha_shift(i['setting_enabled'],1,i['shift_rpm'],i['gear'],i['rpm'],i.get('maximum_rpm',0))
  assert (r==1)==e['visible']
  if 'diagnostic'in e:assert (r<0)==e['diagnostic']
  if 'indicated_rpm'in e:near(lib.ha_indicated_rpm(i['rpm'],i['maximum_rpm']),e['indicated_rpm'])
 else:raise AssertionError('unhandled set 4 vector '+name)
 set4+=1
# Baseline race path keeps its gate; nonfinite thresholds diagnose (D5).
assert lib.ha_inherited_message_visuals(1,1,1)==1 and lib.ha_inherited_message_visuals(1,0,0)==0
assert lib.ha_shift(1,1,-1,3,5000,0)==-1 and lib.ha_shift(0,1,-1,3,5000,0)==0
print(f'{set4} set 4 vectors (capture4.json) plus race-path and threshold checks passed')

# ---------------------------------------------------------------------------
# Capture set 5: capture5.json (E1 gear colour, E2/E3 N2O glow, E4 NOS level).
class Glow(C.Structure):_fields_=[('visible',C.c_int),('alpha_byte',C.c_ubyte),('alpha',C.c_double),('width',C.c_double),('height',C.c_double),('rotation_ccw_deg',C.c_double)]
lib.ha_glow.argtypes=[C.c_double,C.POINTER(Glow)]
lib.ha_glow_units.argtypes=[C.c_double,C.c_double];lib.ha_glow_units.restype=C.c_double
lib.ha_glow_trigger.argtypes=[C.c_int,C.c_double,C.c_int,C.c_double,C.c_double]
def glow_at(t):
 g=Glow();lib.ha_glow(t,C.byref(g));return g
def check_glow(g,e):
 if e.get('hidden'):assert not g.visible,e;return
 assert g.visible,e
 assert abs(g.alpha-e['alpha'])<1e-5,(g.alpha,e['alpha']);assert g.alpha_byte==round(e['alpha']*255)
 assert abs(g.width-e['size_v'][0])<1e-6 and abs(g.height-e['size_v'][1])<1e-6,(g.width,g.height,e)
 assert abs(g.rotation_ccw_deg-e['rotation_deg_ccw'])<1e-6,(g.rotation_ccw_deg,e)
set5=0;cases5=json.loads(inputs['test-vectors/capture5.json'])['cases'];by_name={c['name']:c for c in cases5}
for case in cases5:
 name=case['name'];i=case['input'];e=case['expected']
 if name.startswith('gear-'):
  change=change_of(i['gear_change_in_progress'])
  g=GEARS.get(i['gear'],i['gear'])
  assert tint(TEXT_GEAR,g,i['authored_gear_rgb'],i['gear_colour_setting_rgb'],change,i['speed_mps'],i['authored_alpha'])==e['gear_rgb'],name
  assert tint(TEXT_SPEED,g,i['authored_gear_rgb'],i['gear_colour_setting_rgb'],change,i['speed_mps'],i['authored_alpha'])==i['authored_gear_rgb'],'non-gear text keeps authored tint'
 elif name=='glow-timescale':
  t=lib.ha_glow_units(i['ms_since_event'],i['ms_per_unit']);assert abs(t-e['equivalent_t_units'])<0.01,t  # vector value is rounded to 2 decimals
  check_glow(glow_at(t),by_name[e['same_as']]['expected'])
 elif name.startswith('glow-'):check_glow(glow_at(i['t_units']),e)
 elif name=='nos-level-three-quarters':
  limit=C.c_double();r=lib.ha_nos_level(i['setting_enabled'],i['nos_installed'],1,i['level'],i['a0'],i['a1'],C.byref(limit))
  assert r==2;near(limit.value,e['coloured_layer_max_dial_angle'])
 else:raise AssertionError('unhandled set 5 vector '+name)
 set5+=1
# F1 extras: the change input only adds orange for forward gears; N/R authored whatever the input.
A=[192,192,192];O=[255,128,64]
assert tint(TEXT_GEAR,4,A,O,1,0)==O and tint(TEXT_GEAR,4,A,O,1,50)==O and tint(TEXT_GEAR,4,A,O,0,0)==A
assert tint(TEXT_GEAR,1,A,O,0,0)==O and tint(TEXT_GEAR,1,A,O,1,50)==O and tint(TEXT_GEAR,1,A,O,0,50)==A
for c in (0,1):
 for g in (-1,0):assert tint(TEXT_GEAR,g,A,O,c,0)==A
assert tint(TEXT_GEAR,1,A,O,0,-0.4)==O and tint(TEXT_GEAR,1,A,O,0,-0.6)==A,'speed magnitude'
# E3 extras: curve keys, bounds and wrap points.
for t in [-5,99.9,math.nan,800,1e9,700,621]:assert not glow_at(t).visible,t
g=glow_at(330);assert g.visible and g.alpha_byte==(int(255*230/240))%256 and (g.width,g.height,g.rotation_ccw_deg)==(64,32,0)
g=glow_at(348);assert g.alpha_byte==int(255+245*8/20)%256 and 0<g.alpha_byte<255
assert glow_at(460).width==74 and glow_at(480).width==64 and glow_at(360).rotation_ccw_deg==2 and glow_at(450).rotation_ccw_deg==2
for bad in (0,-1,math.nan,math.inf):assert math.isnan(lib.ha_glow_units(100,bad))
assert math.isnan(lib.ha_glow_units(math.nan,0.82))
assert not glow_at(lib.ha_glow_units(100,0)).visible
# Event handling: opt-in; a new event while the sequence runs is ignored (guessed); allowed again once it ended.
assert lib.ha_glow_trigger(0,0,0,0,0.82)==0,'disabled ignores the event'
assert lib.ha_glow_trigger(1,100,0,0,0.82)==1,'no sequence yet'
assert lib.ha_glow_trigger(1,100,1,50,0.82)==0,'running: ignored'
assert lib.ha_glow_trigger(1,50+800*0.82+1,1,50,0.82)==1,'finished: restart'
assert lib.ha_glow_trigger(1,10,1,50,0.82)==1,'clock restarted: new sequence'
assert lib.ha_glow_trigger(1,100,0,0,math.nan)==0,'invalid time scale: no sequence'
print(f'{set5} set 5 vectors (capture5.json) plus gear-rule and glow-event checks passed')

# ---------------------------------------------------------------------------
# Capture set 6: capture6.json (F1 gear colour: gear change in progress + gear-1 standstill).
def num(v):return math.nan if v=='NaN' else v
set6=0
for case in json.loads(inputs['test-vectors/capture6.json'])['cases']:
 name=case['name'];i=case['input'];e=case['expected'];assert name.startswith('gear-'),name
 g=GEARS.get(i['gear'],i['gear']);th=num(i['standstill_threshold_mps'])
 if 'gear_change_in_progress_by_frame' in i:
  frames=[(change_of(c),num(i['speed_mps'])) for c in i['gear_change_in_progress_by_frame']];want=e['gear_rgb_by_frame']
 else:frames=[(change_of(i['gear_change_in_progress']),num(i['speed_mps']))];want=[e['gear_rgb']]
 assert len(frames)==len(want),name
 for (c,sp),w in zip(frames,want):
  assert tint(TEXT_GEAR,g,i['authored_gear_rgb'],i['gear_colour_setting_rgb'],c,sp,i['authored_alpha'],th)==w,name
  assert tint(TEXT_SPEED,g,i['authored_gear_rgb'],i['gear_colour_setting_rgb'],c,sp,i['authored_alpha'],th)==i['authored_gear_rgb'],'non-gear text keeps authored tint'
 set6+=1
assert set6==15
assert tint(TEXT_GEAR,1,A,O,0,0,standstill=math.nan)==A,'nonfinite threshold: no standstill claim'
assert tint(TEXT_GEAR,1,A,O,0,0,standstill=-1)==A and tint(TEXT_GEAR,3,A,O,1,0,standstill=0)==O
print(f'{set6} set 6 vectors (capture6.json) plus gear-rule checks passed')

# ---------------------------------------------------------------------------
# Capture set 7: capture7.json (G glyph drawn extent; H shift icon).
class GlyphQuad(C.Structure):_fields_=[(k,C.c_double) for k in ('x0','y0','x1','y1','u0','v0','u1','v1')]
lib.ha_glyph_quad.argtypes=[C.POINTER(Glyph),C.c_float,C.c_float,C.c_double,C.c_double,C.c_double,C.c_double,C.c_int,C.POINTER(GlyphQuad)]
lib.ha_shift_mode.argtypes=[C.c_int,C.c_int,C.c_double,C.c_int,C.c_double,C.c_double,C.c_int,C.c_int]
lib.ha_shift_flash.argtypes=[C.c_int,C.c_double,C.c_double,C.POINTER(C.c_int)];lib.ha_shift_flash.restype=C.c_double
class ShiftPhase(C.Structure):_fields_=[('shown',C.c_int),('start_ms',C.c_double)]
lib.ha_shift_elapsed.argtypes=[C.POINTER(ShiftPhase),C.c_int,C.c_double];lib.ha_shift_elapsed.restype=C.c_double
lib.ha_shift_default_colour.argtypes=[U3]
TRANSMISSION={None:0,'manual':1,'automatic':2}
def near_tol(a,b,t):assert abs(a-b)<=t,(a,b)
set7=0;shift7=0
for case in json.loads(inputs['test-vectors/capture7.json'])['cases']:
 i,e,name=case['input'],case['expected'],case['name']
 if name.startswith('glyph-'):
  size=i['size'];glyphs=i['glyphs'];start=lib.ha_text_start(i['anchor'][0],size,i['justification'],float(sum(g['advance'] for g in glyphs)))
  if 'pen_start' in e:near(start,e['pen_start'])
  pen=start;quads=[]
  for g in glyphs:
   atlas=g.get('atlas_size',[64,64]);ax,ay=g.get('atlas_xy',[0,0])
   rec=Glyph(code=65,x=ax,y=ay,advance=g['advance'],width=g.get('width',0),height=g.get('height',0),top=g.get('top_offset',0),bearing=g.get('bearing_x',0))
   q=GlyphQuad();drew=lib.ha_glyph_quad(C.byref(rec),1/atlas[0],1/atlas[1],pen,i['anchor'][1],size,size,1 if i.get('half_texel_inset') else 0,C.byref(q))
   if drew:
    quads.append((q,rec,atlas))
    if 'uv' in e.get('quads',[{}])[0] and not i.get('half_texel_inset'):near([q.u0,q.v0,q.u1,q.v1],e['quads'][0]['uv'])
   pen+=g['advance']*size
  assert len(quads)==e.get('quad_count',len(e['quads'])),name
  for (q,rec,atlas),w in zip(quads,e['quads']):near([q.x0,q.x1,q.y0,q.y1],[w['x0'],w['x1'],w['y0'],w['y1']])
  if 'next_pen' in e:near(pen,e['next_pen'])
  if 'justification_width' in e:near(sum(g['advance'] for g in glyphs)*size,e['justification_width'])
  if 'uv_within' in e:
   # G3 item 2: each coordinate lies between the whole cell and the half-texel-inset cell.
   q,rec,atlas=quads[0];lo=e['uv_within'];cell=[rec.x/atlas[0],rec.y/atlas[1],(rec.x+rec.width)/atlas[0],(rec.y+rec.height)/atlas[1]];assert all(c<=u+1e-9 for c,u in zip(cell[:2],[q.u0,q.v0])) and all(u<=c+1e-9 for c,u in zip(cell[2:],[q.u1,q.v1]));assert q.u0<=lo[0]+1e-9 and q.v0<=lo[1]+1e-9 and q.u1>=lo[2]-1e-9 and q.v1>=lo[3]-1e-9
  if 'ink_past_anchor' in e:near(quads[-1][0].x1-i['anchor'][0],e['ink_past_anchor'])
  assert not e.get('clipped') and not e.get('scissor') and not e.get('clipped_at_advance'),name
  set7+=1
 elif name.startswith('shift-'):
  ts=i.get('time_scale_ms_per_unit',0.82);g=GEARS.get(i.get('gear',2),i.get('gear',2))
  shown=lib.ha_shift_mode(1 if i.get('setting_enabled',True) else 0,1,i['shift_rpm'],g,i.get('rpm',7400),0,TRANSMISSION[i.get('transmission')],1 if i.get('shift_icon_automatic',True) else 0)==1
  if name=='shift-reappear-phase':
   # H3 item 5 (provisional): shown at 0, hidden at 300 units, shown again at
   # 900 units, where a free-running phase would be dark; the phase restarts.
   assert shown,'reappear case is shown at the reference RPM'
   ph=ShiftPhase();t0=10000.0;d=C.c_int()
   assert lib.ha_shift_elapsed(C.byref(ph),1,t0)==0;near(lib.ha_shift_elapsed(C.byref(ph),1,t0+299*ts),299*ts)
   assert lib.ha_shift_elapsed(C.byref(ph),0,t0+300*ts)==0
   since=lib.ha_shift_elapsed(C.byref(ph),1,t0+900*ts);near(since,i['elapsed_since_reappear_units']*ts)
   near_tol(lib.ha_shift_flash(1,since,ts,C.byref(d)),e['alpha'],0.05);assert not d.value
   assert lib.ha_shift_flash(1,900*ts,ts,C.byref(d))==0,'without the restart this frame would be dark'
   dflt=U3();lib.ha_shift_default_colour(dflt);assert setting_tint([255,255,255],list(dflt))==e['rgb']
   shift7+=1;set7+=1;continue
  assert shown==e['visible'],name
  if e['visible']:
   ms=i['elapsed_units']*ts if 'elapsed_units' in i else i.get('elapsed_ms',0);d=C.c_int()
   env=lib.ha_shift_flash(1 if i.get('flash_setting',True) else 0,ms,ts,C.byref(d))
   if 'alpha' in e:near_tol(env,e['alpha'],0.05)
   if e.get('diagnostic'):assert d.value==1
   if 'rgb' in e:
    rgb=i.get('shift_colour_setting_rgb')
    if rgb is None:dflt=U3();lib.ha_shift_default_colour(dflt);rgb=list(dflt)
    assert setting_tint([255,255,255],rgb)==e['rgb'],name
  shift7+=1;set7+=1
 else:raise AssertionError(name)
assert set7==len(json.loads(inputs['test-vectors/capture7.json'])['cases']) and shift7==len([c for c in json.loads(inputs['test-vectors/capture7.json'])['cases'] if c['name'].startswith('shift-')]),(set7,shift7)
# Envelope shape, repeat, steady fallbacks and phase restart.
d=C.c_int()
for u,want in [(0,1),(579,1),(580,1),(590,.5),(600,0),(1199,0),(1200,1),(1780,1),(1790,.5),(2380,0),(2400,1)]:near_tol(lib.ha_shift_flash(1,u*0.82,0.82,C.byref(d)),want,1e-6);assert not d.value
for bad_scale in [0,-1,math.nan,math.inf]:assert lib.ha_shift_flash(1,900,bad_scale,C.byref(d))==1 and d.value==1
assert lib.ha_shift_flash(1,math.nan,0.82,C.byref(d))==1 and d.value==1
assert lib.ha_shift_flash(0,900,0.82,C.byref(d))==1 and d.value==0
assert lib.ha_shift_mode(1,1,7000,2,7400,0,2,0)==0 and lib.ha_shift_mode(1,1,7000,2,7400,0,1,0)==1 and lib.ha_shift_mode(1,1,7000,2,7400,0,0,0)==1
assert lib.ha_shift_mode(1,1,math.nan,2,7400,0,1,1)==-1 and lib.ha_shift_mode(1,0,7000,2,7400,0,1,1)==0
assert lib.ha_shift_mode(1,1,7000,2,7400,0,7,0)==1,'unknown transmission values apply H3'
# Phase: invalid clock gives NaN (steady lit with diagnostic), then restarts on a valid clock; a backward clock restarts.
ph=ShiftPhase();assert math.isnan(lib.ha_shift_elapsed(C.byref(ph),1,math.nan));assert lib.ha_shift_flash(1,math.nan,0.82,C.byref(d))==1 and d.value==1
assert lib.ha_shift_elapsed(C.byref(ph),1,500)==0 and lib.ha_shift_elapsed(C.byref(ph),1,700)==200 and lib.ha_shift_elapsed(C.byref(ph),1,100)==0
assert lib.ha_shift_flash(1,-1,0.82,C.byref(d))==1 and d.value==1
# G3: a glyph is never clipped to its advance or pen, and 1-texel extents take no inset.
q=GlyphQuad();assert lib.ha_glyph_quad(C.byref(Glyph(code=48,x=10,y=0,advance=17,width=18,height=30,top=0,bearing=2)),1/256,1/64,0,0,0.8,0.8,1,C.byref(q))
near([q.x0,q.x1],[1.6,16]);assert q.x1>17*0.8
assert lib.ha_glyph_quad(C.byref(Glyph(code=124,x=4,y=4,advance=3,width=1,height=1,top=0,bearing=0)),1/8,1/8,0,0,1,1,1,C.byref(q));near([q.u0,q.v0,q.u1,q.v1],[.5,.5,.625,.625])
print(f'{set7} set 7 vectors (capture7.json: glyph extent and shift icon) plus envelope/fallback/phase checks passed')

# ---------------------------------------------------------------------------
# Set 8: capture8.json (G5-G7 glyph sampling). The rendered pixel is modelled
# from the same pieces the GL path uses: the glyph atlas repack (transparent
# one-texel gutter, G6 item 2), ha_glyph_quad (G3 rectangle, inset per the
# setting), linear varying interpolation across the quad at pixel centres,
# ha_sample_linear (the GL_LINEAR + GL_CLAMP_TO_EDGE sampler state set on the
# font atlas) and ha_glyph_over (shader multiply plus SRC_ALPHA,
# ONE_MINUS_SRC_ALPHA blend).
class GlyphCell(C.Structure):_fields_=[('x',C.c_uint16),('y',C.c_uint16),('ok',C.c_ubyte)]
class GlyphAtlas(C.Structure):_fields_=[('rgba',C.POINTER(C.c_ubyte)),('width',C.c_uint),('height',C.c_uint),('cells',C.POINTER(GlyphCell)),('count',C.c_size_t)]
lib.ha_glyph_atlas.argtypes=[C.c_char_p,C.c_uint,C.c_uint,C.POINTER(Glyph),C.c_size_t,C.POINTER(GlyphAtlas)]
lib.ha_glyph_atlas_free.argtypes=[C.POINTER(GlyphAtlas)]
lib.ha_sample_linear.argtypes=[C.POINTER(C.c_ubyte),C.c_uint,C.c_uint,C.c_double,C.c_double,C.c_double*4]
lib.ha_glyph_coverage.argtypes=[C.c_double,C.c_double];lib.ha_glyph_coverage.restype=C.c_double
lib.ha_glyph_over.argtypes=[C.c_double]*5;lib.ha_glyph_over.restype=C.c_double
def parse_cell(text):
 m=re.match(r'(\d+)\s*(?:wide\s*)?x\s*(\d+)(?:\s*tall)?,\s*every texel (\d+)',text);assert m,text;return int(m[1]),int(m[2]),int(m[3])
def source_atlas(cw,ch,alpha,rgb,at,size):
 # Adversarial neighbours: every texel outside the cell is opaque, so only the
 # repack's gutter can make the surround read alpha 0 (G6 item 2).
 W,H=size;px=bytearray([17,34,51,255]*(W*H))
 for y in range(ch):
  for x in range(cw):o=((at[1]+y)*W+at[0]+x)*4;px[o:o+4]=bytes([*rgb,alpha])
 return bytes(px)
def render_case(i,at,size,repack=True):
 cw,ch,alpha=parse_cell(i['cell_alpha']);k=i['pixels_per_texel'];qx0,qy0,qx1,qy1=i['quad_screen_px'];tint=i['tint_rgba'];bg=i['background_rgb'][0]
 src=source_atlas(cw,ch,alpha,i['texel_rgb'],at,size);rec=Glyph(code=48,x=at[0],y=at[1],advance=cw,width=cw,height=ch,top=0,bearing=0)
 if repack:
  pa=GlyphAtlas();assert lib.ha_glyph_atlas(src,size[0],size[1],C.byref(rec),1,C.byref(pa))==1
  assert pa.count==1 and pa.cells[0].ok;rec.x,rec.y=pa.cells[0].x,pa.cells[0].y;tex,W,H=pa.rgba,pa.width,pa.height
 else:
  buf=(C.c_ubyte*len(src)).from_buffer_copy(src);tex,W,H=buf,size[0],size[1]
 q=GlyphQuad();assert lib.ha_glyph_quad(C.byref(rec),1/W,1/H,qx0,qy0,k,k,1 if i.get('half_texel_inset') else 0,C.byref(q))
 near([q.x0,q.y0,q.x1,q.y1],[qx0,qy0,qx1,qy1])  # G6 item 1: quad edges on the cell's outer texel edges
 cov=[];out=[];s=(C.c_double*4)()
 for r in range(int(qy1-qy0)):
  crow=[];orow=[]
  for c in range(int(qx1-qx0)):
   tx=(c+.5)/(q.x1-q.x0);ty=(r+.5)/(q.y1-q.y0);lib.ha_sample_linear(tex,W,H,q.u0+tx*(q.u1-q.u0),q.v0+ty*(q.v1-q.v0),s)
   crow.append(lib.ha_glyph_coverage(s[3],tint[3]));orow.append(lib.ha_glyph_over(s[0],s[3],tint[0],tint[3],bg))
  cov.append(crow);out.append(orow)
 if repack:lib.ha_glyph_atlas_free(C.byref(pa))
 return cov,out
set8=0;cases8=json.loads(inputs['test-vectors/capture8.json'])['cases'];byname={c['name']:c for c in cases8}
for case in cases8:
 name,e=case['name'],case['expected'];i=dict(case['input'])
 if 'same_as' in i:base=dict(byname[i.pop('same_as')]['input']);base.update(i);i=base
 cw,ch,_=parse_cell(i['cell_alpha'])
 # Cell at the atlas origin (edge clamping) and in the middle of opaque texels.
 for at,size in [((0,0),(cw+3,ch+2)),((2,1),(cw+5,ch+4))]:
  cov,out=render_case(i,at,size);tol=e.get('tolerance',0.01)
  if 'coverage_row_5_columns_0_to_15' in e:[near_tol(a,b,tol) for a,b in zip(cov[5],e['coverage_row_5_columns_0_to_15'])];assert len(cov[5])==16
  if 'coverage_column_5_rows_0_to_11' in e:[near_tol(row[5],b,tol) for row,b in zip(cov,e['coverage_column_5_rows_0_to_11'])];assert len(cov)==12
  if 'coverage_pixel_0_0' in e:near_tol(cov[0][0],e['coverage_pixel_0_0'],tol)
  if 'coverage_all_pixels' in e:[near_tol(v,e['coverage_all_pixels'],tol) for row in cov for v in row]
  if 'coverage_rows_0_to_3' in e:
   assert len(cov)==4;[near_tol(a,b,tol) for row,want in zip(cov,e['coverage_rows_0_to_3']) for a,b in zip(row,want)]
   near_tol(max(v for row in cov for v in row),e['max_coverage'],tol)
  if 'edge_pixel_full_coverage' in case.get('must_not',{}):assert cov[5][0]<0.99 and cov[5][15]<0.99,name
  if 'output_value' in e:
   probe=[out[r][c] for r in range(4,12) for c in range(4,12)];t8=e['tolerance_8bit']
   for v in probe:near_tol(v,e['output_value'],t8)
   if 'coverage' in e:near_tol(cov[6][6],e['coverage'],0.01)
   for v in case.get('must_not',{}).values():assert all(abs(p-v)>t8 for p in probe),(name,v)
 set8+=1
assert set8==len(cases8)==7,set8
# The repack is what makes the surround transparent: sampling the source atlas
# directly, with opaque neighbours, would put full coverage on the quad edge.
cov,_=render_case(byname['glyph-map-edge-fade']['input'],(2,1),(9,7),repack=False);assert cov[5][0]>0.99
# Gutter invariants over several glyphs: every texel of the one-texel ring
# (diagonal corners included) has alpha 0, cells are copied exactly, empty and
# out-of-atlas cells take no slot, and slots never overlap.
W,H=16,8;src=bytes((x*7+y*13)%256 if c<3 else 200+(x+y)%56 for y in range(H) for x in range(W) for c in range(4))
gs=(Glyph*4)(Glyph(code=48,x=0,y=0,width=5,height=8,advance=5),Glyph(code=49,x=5,y=0,width=11,height=3,advance=11),Glyph(code=32,x=0,y=0,width=0,height=0,advance=4),Glyph(code=50,x=12,y=6,width=8,height=4,advance=8))
pa=GlyphAtlas();assert lib.ha_glyph_atlas(src,W,H,gs,4,C.byref(pa))==1
px=lambda x,y:[pa.rgba[(y*pa.width+x)*4+c] for c in range(4)]
assert pa.cells[0].ok and pa.cells[1].ok and pa.cells[2].ok and not pa.cells[3].ok,'cell outside the source atlas is rejected'
spans=[]
for j in range(2):
 g,cell=gs[j],pa.cells[j];assert cell.x>=1 and cell.y>=1 and cell.x+g.width+1<=pa.width and cell.y+g.height+1<=pa.height
 for y in range(-1,g.height+1):
  for x in range(-1,g.width+1):
   p=px(cell.x+x,cell.y+y)
   if 0<=x<g.width and 0<=y<g.height:o=((g.y+y)*W+g.x+x)*4;assert p==list(src[o:o+4])
   else:assert p[3]==0,(j,x,y)
 spans.append((cell.x-1,cell.y-1,cell.x+g.width+1,cell.y+g.height+1))
a,b=spans;assert a[2]<=b[0] or b[2]<=a[0] or a[3]<=b[1] or b[3]<=a[1],'slots overlap'
lib.ha_glyph_atlas_free(C.byref(pa));assert not pa.rgba and not pa.cells
assert lib.ha_glyph_atlas(src,W,H,gs,0,C.byref(pa))==1 and pa.width>=1 and pa.height>=1;lib.ha_glyph_atlas_free(C.byref(pa))
assert lib.ha_glyph_atlas(None,W,H,gs,1,C.byref(pa))==0 and lib.ha_glyph_atlas(src,0,H,gs,1,C.byref(pa))==0
# Sampler: exact texel centres, half-way blend, clamp to edge.
t=(C.c_ubyte*16)(*[0,0,0,0, 255,255,255,255, 0,0,0,0, 0,0,0,0]);s=(C.c_double*4)()
lib.ha_sample_linear(t,2,2,.75,.25,s);near(list(s),[1,1,1,1]);lib.ha_sample_linear(t,2,2,.5,.25,s);near(list(s),[.5]*4);lib.ha_sample_linear(t,2,2,1.5,.25,s);near(list(s),[1]*4)
lib.ha_sample_linear(t,2,2,-3,-3,s);near(list(s),[0]*4)
print(f'{set8} set 8 vectors (capture8.json: glyph mapping, bilinear fade, linear alpha) plus gutter/sampler checks passed')

# Project policy: widescreen HUD fitment (wide-policy.json; not original behaviour).
wide=json.loads(inputs['test-vectors/wide-policy.json'])
FIT={'wide':0,'4:3':1}
lib.ha_fit_scale.argtypes=[C.c_int,C.c_int,C.c_int,C.POINTER(C.c_double),C.POINTER(C.c_double)]
lib.ha_fit_anchor.argtypes=[C.c_double]
lib.ha_fit_translation.argtypes=[C.c_int,C.c_int,C.c_int,C.c_double];lib.ha_fit_translation.restype=C.c_double
lib.ha_fit_point.argtypes=[C.c_int,C.c_int,C.c_int,C.c_double,C.c_double,C.c_double,C.POINTER(C.c_double)]
def near_px(a,b,t=1e-2):assert abs(a-b)<=t,(a,b)
wide_checks=0
for case in wide['cases']:
 i,e,name=case['input'],case['expected'],case['name'];policy=FIT[i['policy']];w,h=i['viewport']
 if 'click' in i:
  # Draw/pick round trip: a map group on the left (centre -224) and one on the right; the click is at the
  # rendered map centre, i.e. the final transform of the group's logical centre including its translation.
  for gx in (-224.0,0.0,216.0):
   out=(C.c_double*3)();assert lib.ha_fit_point(policy,w,h,gx,gx,17.0,out)
   frame=MapFrame();frame.width,frame.height=w,h;frame.enabled=1;frame.size[:]=[100,100];frame.basis[:]=[1,0,0,1]
   frame.center[:]=[gx+lib.ha_fit_translation(policy,w,h,gx),17.0];frame.world[:]=[1234.5,-678.25];frame.heading=0.7;frame.window=500
   res=(C.c_double*2)();assert lib.ha_map_pick(C.byref(frame),out[1],out[2],1,res);near_px(res[0],1234.5,1e-6);near_px(res[1],-678.25,1e-6)
   if gx<-107 and policy==0 and w/h>4/3:assert lib.ha_fit_translation(policy,w,h,gx)<0  # translated, not merely identical
   # a click in the unshifted position lies outside a shifted map and must not pick the world centre
   if abs(lib.ha_fit_translation(policy,w,h,gx))>1:
    old=(C.c_double*3)();assert lib.ha_fit_point(1,w,h,gx,gx,17.0,old)
    frame2=frame;assert not lib.ha_map_pick(C.byref(frame2),old[1],old[2],1,res) or abs(res[0]-1234.5)>1e-3
 elif 'group_centre_logical_x' in i:
  cx=i['group_centre_logical_x'];x=cx+i.get('descendant_local_offset_x',0);out=(C.c_double*3)()
  assert lib.ha_fit_point(policy,w,h,cx,x,0,out)
  near_px(out[1],e.get('output_x_px',e.get('descendant_output_x_px')))
  if 'translation' in e:assert lib.ha_fit_translation(policy,w,h,cx)==e['translation']
  if 'translation_logical' in e:near_px(lib.ha_fit_translation(policy,w,h,cx),e['translation_logical'])
 else:
  s_,d_=C.c_double(),C.c_double();assert lib.ha_fit_scale(policy,w,h,C.byref(s_),C.byref(d_));near_px(s_.value,e['s'],1e-6);near_px(d_.value,e['D'],1e-3)
 wide_checks+=1
assert wide_checks==len(wide['cases'])
# Policy facts stated in the section but not encoded as numeric vectors.
assert [lib.ha_fit_anchor(x) for x in (-640/6-1e-9,-640/6,0,640/6,640/6+1e-9)]==[-1,0,0,0,1]
for w,h in ((640,480),(1440,1080),(1024,768),(800,1000),(300,1080)):  # 4:3 and narrower: wide == 4:3
 for gx in (-300.0,-224.0,0.0,216.0,300.0):
  a,b=(C.c_double*3)(),(C.c_double*3)();assert lib.ha_fit_point(0,w,h,gx,gx,5,a) and lib.ha_fit_point(1,w,h,gx,gx,5,b);assert list(a)==list(b)
for w,h in ((1920,1080),(2560,1080),(1366,768)):  # 4:3 policy never moves anything; wide keeps y and scale
 for gx in (-300.0,0.0,300.0):
  a,b=(C.c_double*3)(),(C.c_double*3)();assert lib.ha_fit_point(1,w,h,gx,gx,5,a);assert lib.ha_fit_translation(1,w,h,gx)==0
  assert lib.ha_fit_point(0,w,h,gx,gx,5,b);near_px(a[0],b[0],1e-12);near_px(a[2],b[2],1e-12)
  s_,d_=C.c_double(),C.c_double();lib.ha_fit_scale(0,w,h,C.byref(s_),C.byref(d_));near_px(s_.value,h/480,1e-12);near_px(d_.value,(w/h*480-640)/2,1e-9)
  near_px(lib.ha_fit_translation(0,w,h,gx),(0 if gx==0 else -d_.value if gx<0 else d_.value),1e-9)
assert not lib.ha_fit_scale(0,0,480,C.byref(C.c_double()),C.byref(C.c_double())) and not lib.ha_fit_point(0,640,-1,0,0,0,(C.c_double*3)())
for bad in (math.nan,math.inf,-math.inf):  # nonfinite group centre: not moved, never a nonfinite translation
 assert lib.ha_fit_anchor(bad)==0 and lib.ha_fit_translation(0,1920,1080,bad)==0
print(f'{wide_checks} widescreen policy vectors (wide-policy.json) plus identity/threshold/validity checks passed')
