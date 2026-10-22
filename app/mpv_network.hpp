#pragma once
#include "media_source.hpp"
struct mpv_handle;

// Shared by the Switch player and the native HTTP playback integration test.
bool configureMpvNetwork(mpv_handle* mpv, const media::Headers& headers,
                         const char* caFile = "romfs:/cacert.pem");
