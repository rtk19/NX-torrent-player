#include "media_source.hpp"
#include "json.hpp"

#include <cassert>
#include <iostream>

int main()
{
    using namespace media;
    assert(httpUrl("HTTPS://cdn.test/Vid.mp4?sig=a,B%2fC&n=1#fragment") ==
           "https://cdn.test/Vid.mp4?sig=a,B%2fC&n=1");
    for (const auto* url : {"", "https://", "http:///no-host", "http://host/\r\nX:bad",
                            "file:///sdmc/foo", "ftp://host/video", "https://a/has space",
                            "magnet:?xt=urn:btih:test", "https:/host/video", "http://a\\b"})
        assert(!isHttpUrl(url));
    assert(isHttpUrl("http://[::1]:9000/video.mkv"));
    const std::string payload = R"json({
        "name":"A label with } and \"request\": { inside",
        "url":"https://cdn.test/video?token=a\u0026b=c",
        "behaviorHints":{"proxyHeaders":{
            "response":{"X-Wrong":"never send"},
            "request":{"User-Agent":"Player, 1.0", "Referer":"https://site.test/?x=a,b",
                       "Cookie":"session=a=b; mode=c,d", "X-Token":"abc\u003ddef",
                       "X-Null":"abc\u0000def", "Bad\r\nName":"oops",
                       "X-Bad":"a\r\nInjected: yes", "Range":"bytes=5-", "X-Number":123}
        }}, "externalUrl":"https://external.test/"})json";
    auto fields = headerFields(requestHeaders(payload));
    assert(fields.size() == 4);
    assert(fields[0] == "User-Agent: Player, 1.0");
    assert(fields[1] == "Referer: https://site.test/?x=a,b");
    assert(fields[2] == "Cookie: session=a=b; mode=c,d");
    assert(fields[3] == "X-Token: abc=def");
    assert(headerFields({{"user-agent", "first"}, {"User-Agent", "second"}}) ==
           std::vector<std::string>{"User-Agent: second"});
    assert(requestHeaders(R"({"request":{"Cookie":"wrong"}})").empty());
    assert(requestHeaders(R"({"behaviorHints":{"proxyHeaders":{"response":{"Cookie":"wrong"}}}})").empty());
    auto direct = json::stringMembers(payload);
    assert(direct.size() == 3);
    assert(direct[1].first == "url" && direct[1].second == "https://cdn.test/video?token=a&b=c");
    assert(json::stringMembers(R"({"externalUrl":"https://site/","behaviorHints":{"url":"nested"}})").size() == 1);
    assert(json::object(R"({"label":"\"request\": {}", "request":{"A":"B"}})", "request") == "{\"A\":\"B\"}");

    assert(!canStart(true, false, false, 20, true, 10));
    assert(!canStart(true, true, false, 20, true, 10));
    assert(!canStart(true, true, true, 0.2, false, 120));
    assert(canStart(true, true, true, 2, false, 120));
    assert(canStart(true, true, true, 0, true, 0.1)); // tiny clip/full cache
    assert(canStart(true, true, true, 0.1, false, 0)); // live
    assert(!canStart(false, true, true, 2, false, 120));
    assert(canStart(false, true, true, 15, false, 120));
    assert(canStart(false, true, true, 0.1, true, 0.1));
    std::cout << "Media source validation, scoped addon headers and startup policy PASS\n";
}
