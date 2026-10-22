#include "mpv_network.hpp"
#include <mpv/client.h>

bool configureMpvNetwork(mpv_handle* mpv, const media::Headers& headers, const char* caFile)
{
    // Check every network option: failure must not silently disable TLS checks.
    for (const auto& option : media::Headers{
             {"network-timeout", "15"}, {"tls-verify", "yes"}, {"tls-ca-file", caFile},
             {"stream-lavf-o", "reconnect=1,reconnect_on_network_error=1,reconnect_on_http_error=5xx,reconnect_delay_max=3"}})
        if (mpv_set_option_string(mpv, option.first.c_str(), option.second.c_str()) < 0)
            return false;

    // Nodes preserve commas/equals in Cookie, User-Agent and signed Referer
    // values. A comma-separated option string would split these into headers.
    auto fields = media::headerFields(headers);
    std::vector<mpv_node> values(fields.size());
    for (size_t i = 0; i < fields.size(); ++i) {
        values[i].format = MPV_FORMAT_STRING;
        values[i].u.string = const_cast<char*>(fields[i].c_str());
    }
    mpv_node_list list{};
    list.num = (int)values.size();
    list.values = values.data();
    mpv_node node{};
    node.format = MPV_FORMAT_NODE_ARRAY;
    node.u.list = &list;
    return mpv_set_option(mpv, "http-header-fields", MPV_FORMAT_NODE, &node) >= 0;
}
