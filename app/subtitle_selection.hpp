#pragma once
#include <string>
#include <vector>

namespace subtitle_selection
{
// Select an addon only when it improves on the best embedded language. Empty
// entries represent failed downloads and are skipped. Equal rank keeps the
// embedded track and avoids downloading another version unnecessarily.
int addonCandidate(const std::vector<std::string>& embedded,
                   const std::vector<std::string>& addons);
}
