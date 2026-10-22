"""Exercise the actual C++ video surface on Mesa/EGL (no Switch emulation).
Build tests/video_surface_harness.cpp + app/video_surface.cpp + vendored glad.c
as /tmp/nx-video-surface.so, then run this script. No pip dependencies needed.
"""
import ctypes as c
import sys
p = c.c_void_p
I = c.c_int
U = c.c_uint
F = c.c_float
def fn(lib, name, result, *args):
    f = getattr(lib, name); f.restype = result; f.argtypes = args; return f
egl = c.CDLL('libEGL.so.1')
getproc = fn(egl, 'eglGetProcAddress', p, c.c_char_p)
platform_display = c.CFUNCTYPE(p, U, p, c.POINTER(I))(getproc(b'eglGetPlatformDisplayEXT'))
display = platform_display(0x31DD, None, None)  # EGL_PLATFORM_SURFACELESS_MESA
assert fn(egl, 'eglInitialize', U, p, p, p)(display, None, None)
assert fn(egl, 'eglBindAPI', U, U)(0x30A2)  # OpenGL
attrs = (I*13)(0x3033,1,0x3040,8,0x3024,8,0x3023,8,0x3022,8,0x3021,8,0x3038)
config = p(); count = I()
assert fn(egl,'eglChooseConfig',U,p,c.POINTER(I),c.POINTER(p),I,c.POINTER(I))(display,attrs,c.byref(config),1,c.byref(count)) and count.value
pattrs=(I*5)(0x3057,64,0x3056,64,0x3038)
surface=fn(egl,'eglCreatePbufferSurface',p,p,p,c.POINTER(I))(display,config,pattrs)
ctxattrs=(I*7)(0x3098,3,0x30FB,3,0x30FD,1,0x3038)
context=fn(egl,'eglCreateContext',p,p,p,p,c.POINTER(I))(display,config,None,ctxattrs)
assert context and surface
assert fn(egl,'eglMakeCurrent',U,p,p,p,p)(display,surface,surface,context)
lib=c.CDLL(sys.argv[1] if len(sys.argv)>1 else '/tmp/nx-video-surface.so')
loader=c.CFUNCTYPE(p,c.c_char_p)(getproc)
assert fn(lib,'gladLoadGLLoader',I,p)(c.cast(loader,p))
def gl(name,result,*args): return c.CFUNCTYPE(result,*args)(getproc(name.encode()))
enable=gl('glEnable',None,U); disable=gl('glDisable',None,U)
mask=gl('glColorMask',None,U,U,U,U); clearcolor=gl('glClearColor',None,F,F,F,F)
clear=gl('glClear',None,U); scissor=gl('glScissor',None,I,I,I,I)
read=gl('glReadPixels',None,I,I,I,I,U,U,p)
bind=gl('glBindFramebuffer',None,U,U)
begin=fn(lib,'surface_begin',I,p,I,I); present=fn(lib,'surface_present',I,p,I,I,I)
owned=fn(lib,'surface_create',p)()
assert not begin(owned,0,32)
pixel=(c.c_ubyte*4)()
for index in range(60000):
    w,h=(32,32) if index%83 else (48,48)
    # Leave the previous pass in the same kinds of state as a UI renderer.
    enable(0x0BE2); enable(0x0C11); enable(0x0B90); enable(0x0B71)
    scissor(0,0,1,1); mask(0,1,0,0)
    assert begin(owned,w,h), index
    color=(1.,0.,0.) if index%2 else (0.,1.,0.)
    # Paint only part of a frame: untouched pixels must be freshly black.
    enable(0x0C11); scissor(0,0,w//2,h)
    clearcolor(*color,0.); clear(0x4000)
    mask(0,0,0,0)
    ok=index%29!=0
    assert bool(present(owned,w,h,ok)) == ok
    read(1,1,1,1,0x1908,0x1401,pixel)
    expected=tuple(int(v*255) for v in color) if ok else (0,0,0)
    assert tuple(pixel[:3])==expected,(index,list(pixel),expected)
    assert pixel[3]==255,(index,'transparent output',list(pixel))
    read(w-2,1,1,1,0x1908,0x1401,pixel)
    assert tuple(pixel[:3])==(0,0,0),(index,'stale letterbox',list(pixel))
fn(lib,'surface_destroy',None,p)(owned)
assert gl('glGetError',U)()==0
fn(egl,'eglMakeCurrent',U,p,p,p,p)(display,None,None,None)
fn(egl,'eglDestroyContext',U,p,p)(display,context)
fn(egl,'eglDestroySurface',U,p,p)(display,surface)
fn(egl,'eglTerminate',U,p)(display)
print('PASS: 60,000 frames, changing sizes, polluted GL state, partial draws and failed renders; no stale pixels')
