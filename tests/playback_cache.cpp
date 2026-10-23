#include "playback_cache.hpp"
#include <cassert>
#include <iostream>

struct Session
{
    int& destroyed;
    explicit Session(int& destroyed) : destroyed(destroyed) {}
    ~Session() { ++destroyed; }
    int position = 125;
};

int main()
{
    using Cache = PlaybackCache<Session>;
    const auto start = Cache::Clock::time_point{};
    PlaybackKey movie{"magnet:?xt=urn:btih:abc", "account", "series", "tt1", "tt1:1:1", 2, {}, true};
    int destroyed = 0;
    Cache cache;
    auto session = std::make_unique<Session>(destroyed);
    auto* identity = session.get();
    cache.put(movie, std::move(session), start);
    auto resumed = cache.take(movie, start + std::chrono::seconds(179));
    assert(resumed.get() == identity && resumed->position == 125);
    assert(destroyed == 0); // the demuxer/engine is reused, never reconstructed

    // Each departure starts a new, bounded grace period. Taking consumes it.
    cache.put(movie, std::move(resumed), start + std::chrono::seconds(179));
    cache.expire(start + std::chrono::seconds(358));
    resumed = cache.take(movie, start + std::chrono::seconds(358));
    assert(resumed.get() == identity);
    assert(!cache.take(movie, start + std::chrono::seconds(358)));
    cache.put(movie, std::move(resumed), start);
    cache.expire(start + std::chrono::minutes(3));
    assert(destroyed == 1 && !cache.take(movie, start + std::chrono::minutes(3)));

    // Misses must destroy the old session before the new source can be opened.
    auto different = [&](PlaybackKey next) {
        int before = destroyed;
        cache.put(movie, std::make_unique<Session>(destroyed), start);
        assert(!cache.take(next, start));
        assert(destroyed == before + 1);
    };
    auto next = movie; next.source += "&tr=new-source"; different(next);
    next = movie; next.video = "tt1:1:2"; different(next);
    next = movie; next.item = "tt2"; different(next);
    next = movie; next.type = "movie"; different(next);
    next = movie; next.fileIndex = 3; different(next);
    next = movie; next.account = "another-account"; different(next);
    next = movie; next.headers = {{"Authorization", "new-token"}}; different(next);
    next = movie; next.ram = false; different(next);

    cache.put(movie, std::make_unique<Session>(destroyed), start);
    int before = destroyed;
    cache.clear();
    cache.clear();
    assert(destroyed == before + 1); // shutdown is safe and idempotent
    std::cout << "Playback cache: reuse, isolation, expiry and shutdown PASS\n";
}
