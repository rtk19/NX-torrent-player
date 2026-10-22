#include "config.hpp"
#include <mpv/client.h>
#include <cassert>
#include <chrono>
#include <cstring>
#include <iostream>

namespace i18n { const char* tr(const char* s) { return s; } }

int main(int argc, char** argv)
{
    assert(argc == 5);
    config::get().subLang = argv[2];
    config::get().secondarySubLang = argv[3];
    bool off = std::string(argv[4]) == "off";
    auto* mpv = mpv_create();
    assert(mpv);
    for (auto option : {"config", "terminal"}) assert(mpv_set_option_string(mpv, option, "no") >= 0);
    assert(mpv_set_option_string(mpv, "vo", "null") >= 0);
    assert(mpv_set_option_string(mpv, "ao", "null") >= 0);
    assert(mpv_set_option_string(mpv, "pause", "yes") >= 0);
    assert(mpv_set_option_string(mpv, "slang", config::mpvSubLangList().c_str()) >= 0);
    assert(mpv_set_option_string(mpv, "sid", off ? "no" : "auto") >= 0);
    assert(mpv_initialize(mpv) >= 0);
    const char* command[] = {"loadfile", argv[1], nullptr};
    assert(mpv_command_async(mpv, 1, command) >= 0);
    bool checked = false;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (std::chrono::steady_clock::now() < deadline) {
        auto* event = mpv_wait_event(mpv, 0.05);
        if (event->event_id == MPV_EVENT_FILE_LOADED) {
            if (off) {
                char* sid = mpv_get_property_string(mpv, "sid");
                assert(sid && std::string(sid) == "no");
                mpv_free(sid);
            } else {
                char* lang = mpv_get_property_string(mpv, "current-tracks/sub/lang");
                if (!lang || std::string(lang) != argv[4]) {
                    std::cerr << "Expected " << argv[4] << ", selected " << (lang ? lang : "none") << '\n';
                    return 1;
                }
                mpv_free(lang);
            }
            checked = true;
            break;
        }
    }
    mpv_terminate_destroy(mpv);
    assert(checked);
}
