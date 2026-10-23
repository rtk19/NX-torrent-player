#pragma once
#include <mpv/client.h>

// Keep the demuxer and its existing packets, but stop playback/read-ahead while
// the owning view is in the short-lived cache. Commands never wait on mpv's core
// from the UI thread, where a network read could otherwise freeze navigation.
inline bool setMpvSessionParked(mpv_handle* mpv, bool parked, bool userPaused = false)
{
    const char* pause[] = { "set", "pause", parked || userPaused ? "yes" : "no", nullptr };
    const char* readahead[] = { "set", "demuxer-readahead-secs", parked ? "0" : "30", nullptr };
    const char* cacheSecs[] = { "set", "cache-secs", parked ? "0" : "60", nullptr };
    if (parked && mpv_command_async(mpv, 0, pause) < 0) return false;
    if (mpv_command_async(mpv, 0, readahead) < 0 ||
        mpv_command_async(mpv, 0, cacheSecs) < 0) return false;
    return parked || mpv_command_async(mpv, 0, pause) >= 0;
}
