#include "config.hpp"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
namespace i18n { const char* tr(const char* s) { return s; } }
int main() {
    assert(config::get().subLang == "he");
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
    std::cout << "Subtitle language preferences and codepages: PASS\n";
}
