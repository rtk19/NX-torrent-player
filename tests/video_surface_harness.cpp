#include "video_surface.hpp"
extern "C" {
VideoSurface* surface_create() { return new VideoSurface; }
void surface_destroy(VideoSurface* s) { delete s; }
int surface_begin(VideoSurface* s, int w, int h) { return s->begin(w, h); }
int surface_present(VideoSurface* s, int w, int h, int ok) { return s->present(0, w, h, ok); }
}
