# Manual libass rendering check. Requires Python Pillow and the host libass.
# Run: python3 tests/render_subtitles.py [output-directory]
# Inspect both PNGs for Hebrew reading order, punctuation and mixed Latin/digits.
# Uses no system font provider; matches the Switch font-loading configuration.
import ctypes as c
from pathlib import Path
import sys
repo = Path(__file__).resolve().parents[1]
output = Path(sys.argv[1]) if len(sys.argv) > 1 else Path('/tmp/nx-subtitle-preview')
output.mkdir(parents=True, exist_ok=True)
from PIL import Image
ass = c.CDLL('libass.so.9')
def setup(name, restype, args):
    fn = getattr(ass, name); fn.restype = restype; fn.argtypes = args; return fn
ptr = c.c_void_p
library = setup('ass_library_init', ptr, [])()
setup('ass_set_fonts_dir', None, [ptr,c.c_char_p])(library, str(repo / 'assets/subtitle-fonts').encode())
renderer = setup('ass_renderer_init', ptr, [ptr])(library)
setup('ass_set_frame_size',None,[ptr,c.c_int,c.c_int])(renderer,1280,720)
# No fontconfig/system font provider: match Switch's font discovery.
setup('ass_set_fonts',None,[ptr,c.c_char_p,c.c_char_p,c.c_int,c.c_char_p,c.c_int])(renderer,None,b'DejaVu Sans',0,None,1)
setup('ass_set_shaper',None,[ptr,c.c_int])(renderer,1)
script='''[Script Info]
ScriptType: v4.00+
PlayResX: 1280
PlayResY: 720
[V4+ Styles]
Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding
Style: Default,DejaVu Sans,46,&H00FFFFFF,&H00FFFFFF,&H00000000,&H00000000,-1,0,0,0,100,100,0,0,1,2.6,1.4,2,30,30,50,-1
[Events]
Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text
Dialogue: 0,0:00:00.00,0:00:05.00,Default,,0,0,0,,שלום עולם! בדיקת כתוביות בעברית.
Dialogue: 0,0:00:05.00,0:00:10.00,Default,,0,0,0,,עברית ו-English: מספר 123, שָׁלוֹם!
'''.encode()
track=setup('ass_read_memory',ptr,[ptr,c.c_char_p,c.c_size_t,c.c_char_p])(library,script,len(script),b'UTF-8')
assert track
class ASSImage(c.Structure): pass
ASSImage._fields_=[('w',c.c_int),('h',c.c_int),('stride',c.c_int),('bitmap',c.POINTER(c.c_ubyte)),('color',c.c_uint32),('dst_x',c.c_int),('dst_y',c.c_int),('next',c.POINTER(ASSImage)),('type',c.c_int)]
render=setup('ass_render_frame',c.POINTER(ASSImage),[ptr,ptr,c.c_longlong,c.POINTER(c.c_int)])
for ms in (1000,6000):
    changed=c.c_int(); cur=render(renderer,track,ms,c.byref(changed)); count=0
    out=Image.new('RGBA',(1280,720),(48,64,82,255))
    while cur:
        im=cur.contents
        if im.w and im.h:
            bitmap=b''.join(c.string_at(c.addressof(im.bitmap.contents)+row*im.stride, im.w) for row in range(im.h))
            alpha=Image.frombytes('L',(im.w,im.h),bitmap).point(lambda x: x*(255-(im.color&255))//255)
            ink=Image.new('RGBA',(im.w,im.h),((im.color>>24)&255,(im.color>>16)&255,(im.color>>8)&255,255)); ink.putalpha(alpha)
            out.alpha_composite(ink,(im.dst_x,im.dst_y)); count+=1
        cur=im.next
    assert count > 0
    out.convert('RGB').save(str(output / f'hebrew-{ms}.png'))
    print(f'Bundled fonts with no system provider at {ms}ms: {count} rendered glyph layers, PASS')
setup('ass_free_track',None,[ptr])(track)
setup('ass_renderer_done',None,[ptr])(renderer)
setup('ass_library_done',None,[ptr])(library)
