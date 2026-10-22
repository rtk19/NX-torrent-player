#pragma once

#include <string>
#include <utility>
#include <vector>

namespace media
{
using Headers = std::vector<std::pair<std::string, std::string>>;

// Normalize only the scheme: signed paths/query strings must remain byte exact.
std::string httpUrl(const std::string& url);
inline bool isHttpUrl(const std::string& url) { return !httpUrl(url).empty(); }
Headers requestHeaders(const std::string& streamJson);
std::vector<std::string> headerFields(const Headers& headers);

inline double initialBuffer(bool network) { return network ? 2.0 : 15.0; }
inline bool canStart(bool network, bool loaded, bool firstFrame,
                     double seconds, bool idle, double duration)
{
    return loaded && firstFrame &&
           (seconds >= initialBuffer(network) || idle || (network && duration <= 0.0));
}
} // namespace media
