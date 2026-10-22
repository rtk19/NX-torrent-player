#include "media_source.hpp"
#include "mpv_network.hpp"
#include <mpv/client.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>

int main(int argc, char** argv)
{
    if (argc < 4) return 3;
    std::ifstream input(argv[2]);
    std::string json((std::istreambuf_iterator<char>(input)), {});
    mpv_handle* mpv = mpv_create();
    if (!mpv) return 3;
    for (const auto& opt : media::Headers{{"config", "no"}, {"terminal", "no"},
             {"vo", "null"}, {"ao", "null"}, {"hwdec", "no"}, {"pause", "yes"},
             {"cache", "yes"}, {"cache-pause", "yes"}, {"cache-pause-wait", "2"},
             {"demuxer-max-bytes", "128MiB"}, {"cache-secs", "60"}, {"speed", "8"}})
        if (mpv_set_option_string(mpv, opt.first.c_str(), opt.second.c_str()) < 0) return 3;
    if (!configureMpvNetwork(mpv, media::requestHeaders(json), argv[3])) return 3;
    if (argc > 4 && mpv_set_option_string(mpv, "start", argv[4]) < 0) return 3;
    if (mpv_initialize(mpv) < 0) return 3;
    mpv_observe_property(mpv, 0, "demuxer-cache-duration", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv, 0, "demuxer-cache-idle", MPV_FORMAT_FLAG);
    mpv_observe_property(mpv, 0, "duration", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv, 0, "time-pos", MPV_FORMAT_DOUBLE);
    const char* cmd[] = {"loadfile", argv[1], nullptr};
    if (mpv_command_async(mpv, 1, cmd) < 0) return 3;
    bool loaded = false, frame = false, idle = false, started = false, eof = false, seek = false;
    double cache = 0, duration = 0, position = 0, maxPos = 0;
    int error = 0;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(25);
    while (std::chrono::steady_clock::now() < deadline) {
        mpv_event* e = mpv_wait_event(mpv, 0.02);
        if (e->event_id == MPV_EVENT_FILE_LOADED) loaded = true;
        if (e->event_id == MPV_EVENT_PLAYBACK_RESTART) frame = true;
        if (e->event_id == MPV_EVENT_COMMAND_REPLY && e->error < 0) { error = e->error; break; }
        if (e->event_id == MPV_EVENT_END_FILE) {
            auto* end = (mpv_event_end_file*)e->data;
            eof = end->reason == MPV_END_FILE_REASON_EOF;
            error = end->error;
            break;
        }
        if (e->event_id == MPV_EVENT_PROPERTY_CHANGE) {
            auto* p = (mpv_event_property*)e->data;
            if (p->data && p->format == MPV_FORMAT_DOUBLE) {
                double value = *(double*)p->data;
                if (std::string(p->name) == "time-pos") { position = value; if (value > maxPos) maxPos = value; }
                if (std::string(p->name) == "duration") duration = value;
                if (std::string(p->name) == "demuxer-cache-duration") cache = value;
            }
            if (p->data && std::string(p->name) == "demuxer-cache-idle") idle = *(int*)p->data;
        }
        if (!started && media::canStart(true, loaded, frame, cache, idle, duration)) {
            const char* play[] = {"set", "pause", "no", nullptr};
            mpv_command_async(mpv, 2, play);
            started = true;
        }
        if (argc > 5 && std::string(argv[5]) == "seek" && started && !seek && position > 0.5) {
            const char* jump[] = {"seek", "4", "absolute+exact", nullptr};
            mpv_command_async(mpv, 3, jump);
            seek = true;
        }
        if (argc > 5 && std::string(argv[5]) == "live" && maxPos > 3) break;
    }
    mpv_terminate_destroy(mpv);
    std::printf("{\"loaded\":%d,\"frame\":%d,\"started\":%d,\"eof\":%d,\"seek\":%d,\"position\":%.3f,\"error\":%d}\n",
                loaded, frame, started, eof, seek, maxPos, error);
    return error < 0 ? 2 : started && (eof || maxPos > 3) ? 0 : 4;
}
