#include "video_surface.hpp"

namespace
{
void prepare(GLuint target, int w, int h)
{
    glBindFramebuffer(GL_FRAMEBUFFER, target);
    glViewport(0, 0, w, h);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_RASTERIZER_DISCARD);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}
}

void VideoSurface::reset()
{
    if (fbo) glDeleteFramebuffers(1, &fbo);
    if (texture) glDeleteTextures(1, &texture);
    fbo = texture = 0;
    w = h = 0;
}

bool VideoSurface::begin(int width, int height)
{
    if (width <= 0 || height <= 0) return false;
    if (!fbo || width != w || height != h)
    {
        reset();
        glActiveTexture(GL_TEXTURE0);
        // mpv may have used a pixel upload buffer. nullptr below must mean
        // "allocate storage", never "read from offset zero in that buffer".
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, texture, 0);
        if (!fbo || !texture || glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE ||
            glGetError() != GL_NO_ERROR)
        {
            reset();
            return false;
        }
        w = width;
        h = height;
    }
    prepare(fbo, w, h);
    // No previous frame may contribute, including letterboxes or a failed draw.
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    return true;
}

bool VideoSurface::present(GLuint target, int width, int height, bool renderOk)
{
    if (width <= 0 || height <= 0) return false;
    prepare(target, width, height);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    bool ok = renderOk && fbo && width == w && height == h;
    if (ok)
    {
        // A replacement copy, with no alpha blending against earlier frames.
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target);
        glBlitFramebuffer(0, 0, w, h, 0, 0, width, height,
                           GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }
    // The Switch compositor requires opaque alpha even when mpv outputs 0.
    prepare(target, width, height);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
    glClear(GL_COLOR_BUFFER_BIT);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    if (glGetError() != GL_NO_ERROR)
    {
        prepare(target, width, height);
        glClear(GL_COLOR_BUFFER_BIT);
        return false;
    }
    return ok;
}
