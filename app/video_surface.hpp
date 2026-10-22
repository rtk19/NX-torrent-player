#pragma once
#include <glad/glad.h>

// Owns a complete video image independently of the UI's rotating back buffers.
// All methods, including destruction, run on the current GL context's thread.
class VideoSurface
{
  public:
    ~VideoSurface() { reset(); }
    VideoSurface() = default;
    VideoSurface(const VideoSurface&) = delete;
    VideoSurface& operator=(const VideoSurface&) = delete;
    bool begin(int width, int height);
    bool present(GLuint target, int width, int height, bool renderOk);
    void reset();
    GLuint framebuffer() const { return fbo; }
  private:
    GLuint fbo = 0, texture = 0;
    int w = 0, h = 0;
};
