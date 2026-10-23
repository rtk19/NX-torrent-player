#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

// A single session, owned and expired on the UI/GL thread. Never serialize URLs,
// account tokens or buffered media to disk for this cache.
struct PlaybackKey
{
    std::string source, account, type, item, video;
    int fileIndex = -1;
    std::vector<std::pair<std::string, std::string>> headers;
    bool ram = true;

    bool operator==(const PlaybackKey& b) const
    {
        return std::tie(source, account, type, item, video, fileIndex, headers, ram)
            == std::tie(b.source, b.account, b.type, b.item, b.video,
                        b.fileIndex, b.headers, b.ram);
    }
};

template<class T> class PlaybackCache
{
  public:
    using Clock = std::chrono::steady_clock;
    static constexpr auto lifetime = std::chrono::minutes(3);

    void expire(Clock::time_point now = Clock::now())
    {
        if (value && now >= until) clear();
    }

    std::unique_ptr<T> take(const PlaybackKey& requested,
                            Clock::time_point now = Clock::now())
    {
        expire(now);
        if (!(key == requested)) clear();
        return std::move(value);
    }

    void put(PlaybackKey savedKey, std::unique_ptr<T> saved,
             Clock::time_point now = Clock::now())
    {
        clear();
        key = std::move(savedKey);
        value = std::move(saved);
        until = now + lifetime;
    }

    void clear() { value.reset(); key = {}; }

  private:
    PlaybackKey key;
    std::unique_ptr<T> value;
    Clock::time_point until;
};
