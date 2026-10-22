#include "subtitle_selection.hpp"
#include "config.hpp"
#include <algorithm>

namespace subtitle_selection
{
int addonCandidate(const std::vector<std::string>& embedded,
                   const std::vector<std::string>& addons)
{
    int best = (int)config::preferredSubLangs().size();
    for (const auto& lang : embedded) best = std::min(best, config::subtitlePriority(lang));
    int pick = -1;
    for (size_t i = 0; i < addons.size(); ++i) {
        if (addons[i].empty()) continue;
        int priority = config::subtitlePriority(addons[i]);
        if (priority < best) { best = priority; pick = (int)i; }
    }
    return pick;
}
}
