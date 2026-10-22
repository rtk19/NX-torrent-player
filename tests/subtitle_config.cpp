#include "config.hpp"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include "subtitle_selection.hpp"
namespace i18n { const char* tr(const char* s) { return s; } }
int main() {
    assert(config::get().subLang == "he");
    assert(config::get().secondarySubLang == "en");
    assert(config::mpvSubLangList() == "he,heb,iw,en,eng");
    assert(config::subtitlePriority("heb") == 0);
    assert(config::subtitlePriority("eng") == 1);
    assert(config::subtitlePriority("fr") == 2);
    assert(subtitle_selection::addonCandidate({"heb"}, {"en", "he"}) == -1);
    assert(subtitle_selection::addonCandidate({"eng"}, {"eng", "Hebrew"}) == 1);
    assert(subtitle_selection::addonCandidate({}, {"fr", "eng"}) == 1);
    assert(subtitle_selection::addonCandidate({}, {"en", "he"}) == 1);
    assert(subtitle_selection::addonCandidate({}, {"", "eng"}) == 1); // failed primary -> secondary
    assert(subtitle_selection::addonCandidate({"eng"}, {"eng"}) == -1);
    assert(subtitle_selection::addonCandidate({}, {"fr", ""}) == -1);
    config::get().secondarySubLang = "he";
    assert(config::preferredSubLangs().size() == 1);
    assert(config::mpvSubLangList() == "he,heb,iw");
    config::get().secondarySubLang = "none";
    assert(config::mpvSubLangList() == "he,heb,iw");
    assert(subtitle_selection::addonCandidate({}, {"eng"}) == -1);
    config::get().secondarySubLang = "auto";
    assert(config::mpvSubLangList() == "he,heb,iw,en,eng");
    config::get().secondarySubLang = "en";
    assert(config::preferredSubLang() == "he");
    assert(config::mpvLangList("he") == "he,heb,iw");
    for (const char* tag : {"he", "heb", "iw", "HEB", "Hebrew", "he-IL", "iw_IL"}) {
        assert(config::langLabelFor(tag) == "Hebrew");
        assert(std::strcmp(config::subtitleCodepage(tag), "cp1255") == 0);
    }
    assert(config::langLabelFor("fre") == "French");
    assert(std::strcmp(config::subtitleCodepage("fre"), "cp1252") == 0);
    assert(config::langCodes().size() == config::langLabels().size());
    auto he = std::find(config::langCodes().begin(), config::langCodes().end(), "he");
    assert(he != config::langCodes().end());
    assert(config::langLabels()[he - config::langCodes().begin()] == "Hebrew");
    config::get().subLang = "auto";
    assert(config::preferredSubLang() == "en");
    assert(config::mpvLangList("auto") == "en,eng");
    config::get().subLang = "fr";
    config::load(); // missing config file must preserve current preferences
    assert(config::get().subLang == "fr");
    // Save/reload, and migration from the old primary-only config. Keep all
    // filesystem operations isolated from the real app's config file.
    auto previous = std::filesystem::current_path();
    char pattern[] = "/tmp/nx-subtitle-config-XXXXXX";
    auto* directory = mkdtemp(pattern);
    assert(directory);
    auto temp = std::filesystem::path(directory);
    std::filesystem::create_directories(temp / "sdmc:/switch/NX-torrent-player");
    std::filesystem::current_path(temp);
    config::get().secondarySubLang = "de";
    assert(config::save());
    config::get() = config::Config{};
    config::load();
    assert(config::get().subLang == "fr" && config::get().secondarySubLang == "de");
    std::ofstream("sdmc:/switch/NX-torrent-player/config.json") << "{\"subLang\":\"es\"}";
    config::get() = config::Config{};
    config::load();
    assert(config::get().subLang == "es" && config::get().secondarySubLang == "en");
    std::filesystem::current_path(previous);
    std::filesystem::remove_all(temp);
    std::cout << "Subtitle language preferences and codepages: PASS\n";
}
