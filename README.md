<img src="https://github.com/shodowlo/NX-torrent-player/blob/main/assets/NX-torrent-player-banner.png?raw=true" alt="NX Torrent Player" width="640">

# NX Torrent Player

**NX Torrent Player: Stremio on Nintendo Switch**

Hebrew-support fork maintained by **Refael Bar**. Based on
[shodowlo/NX-torrent-player](https://github.com/shodowlo/NX-torrent-player).

A homebrew video player with a BitTorrent engine built in. It does not download
a file and then play it: it downloads the pieces it is about to need, in order,
and starts playing after a few seconds of buffer. Sign in to a Stremio account
and your library, addons and sources are included too.

> Requires a Switch running custom firmware (Atmosphère).

---

## Features

- **Streams torrents** — playback starts on the first pieces; the engine keeps
  downloading ahead of the playhead, and re-centres on it when you seek.
- **HTTP and HTTPS sources** — direct video links, HLS (including AES-128)
  and DASH use the same player, subtitle menu and watch progress. Addon request
  headers, signed links, redirects and Range-based seeking are preserved.
  Remote playback starts after a short buffer and pauses to refill when needed.
  Seeking/resume require server and stream support; DRM, external web pages,
  expired links and codecs beyond the Switch's capabilities cannot be played.
- **Stremio integration** — sign in, browse your library, drill down through
  seasons → episodes → addons → sources, artwork and all.
- **Local torrents & magnets** — drop `.torrent` files on the SD card, or add
  magnet links (a `magnet.txt` file, or the on-screen keyboard). A magnet's name
  and file list are resolved and cached so you can pick which video to stream.
- **Player** — audio/subtitle track menu, analog scrubbing (either stick), touch
  controls, a control lock (Y), and auto-resume where you left off.
- **Subtitle language fallback** — Options → Playback → Secondary subtitle
  language chooses a fallback when the primary language is unavailable. Defaults
  to Hebrew first, English second; choose None to disable the secondary language.
  Embedded tracks and subtitle addons share that order. Matching embedded tracks
  avoid an unnecessary download; an addon can supply a missing preferred language.
  Manual track selection (including Off) takes priority for the rest of the video.
- **borealis** UI: Horizon look, controller-first, touch works too.

## Screenshots

<table>
  <tr>
    <td><img src="assets/image1.webp" alt="Local torrents tab" width="100%"></td>
    <td><img src="assets/image2.webp" alt="Stremio library" width="100%"></td>
  </tr>
  <tr>
    <td><img src="assets/image4.webp" alt="Episode picker" width="100%"></td>
    <td><img src="assets/image3.webp" alt="Player" width="100%"></td>
  </tr>
</table>

## Install

1. Download `NX-torrent-player.nro` from the [releases](../../releases), or
   build it (below).
2. Put it on your SD card in `/switch/NX-torrent-player/` (create the folder).
3. Launch it from the **Homebrew Menu**.

Updating is done from inside the app (Options → Check on startup is on by
default); it replaces its own .nro and restarts.

This fork checks releases from **rtk19/NX-torrent-player**, including changelogs.
Install this fork's NRO manually once to switch from the upstream update feed.
Future releases with a higher numeric version and an attached `.nro` are offered
on startup; choose Update, then Restart. Only the NRO is downloaded.

### Hebrew subtitles

In **Options → Playback → Subtitle language**, choose **Hebrew**. It is the
default on fresh installs; existing settings are preserved. Embedded tracks tagged
`he`, `heb` or `iw` are preferred, followed by the configured secondary language
(English by default). Subtitle addons automatically supply a missing preferred
language when available; the player's **X** menu also allows manual selection.

Liberation Sans regular (Arial-compatible) and DejaVu Sans fallback faces are
bundled inside the NRO, so no font installation on
the SD card is needed. libass handles right-to-left text, mixed Hebrew/English,
digits and combining marks. UTF-8 remains supported; legacy Hebrew text uses
Windows-1255 as the fallback when Hebrew is preferred (or when downloading a
Hebrew addon subtitle). Bitmap subtitles retain their original appearance.
In **Options → Playback → Subtitle background**, or in the player's **X** menu,
turn the translucent black background with softly rounded corners on/off, in the
style of iPhone captions. The choice is saved and the X-menu
switch applies immediately. It is on by default, including when upgrading from a
version without this setting. Text uses a smaller regular white face, natural
spacing, no heavy outline, and sits just inside the bottom of the video instead
of the letterbox bars. The Subtitle size control still scales it. Styling applies
to text subtitles; bitmap subtitle images retain their baked-in appearance.
The app's interface languages remain English/French.

### Returning to a video

Leaving a playing video keeps one paused session for **3 minutes**. Reopening
the same source and episode reuses its current position, selected tracks and
buffered data, for both torrents and direct HTTP(S) media. Torrent peer sockets
are released while parked and read-ahead is paused. Switching source, movie,
series, episode, torrent file, account, request headers or RAM/SD mode discards
the old session before opening another. Expiry and app exit release the session
and its temporary torrent cache; this does not create a permanent download.
Loading failures and finished videos are not retained.

## Usage

### Local torrents & magnets

Drop `.torrent` files into `sdmc:/switch/NX-torrent-player/torrents/`
(sub-folders are scanned too) and they appear in the **Local** tab with the
torrent's total size.

Magnet links show up in the same list. Add them either by editing
`torrents/magnet.txt` (one magnet link or bare info-hash per line) or with the
**Add magnet** button at the top of the tab (on-screen keyboard). A magnet's
name and file list are fetched in the background — a spinner shows which one is
resolving — and cached, so next time it lists and opens instantly.

Press **Y** on an entry to remove it (a magnet is dropped from the list; a
`.torrent` file is deleted from the SD card). When a torrent — local or magnet —
holds several videos, you get to pick which one; if it has no video, it says so.


### Stremio

In the **Stremio** tab, sign in with your email and password. The session is
kept and survives a restart (sign out from Options).

Pick a title → for a show, a season, then an episode → an addon → a source.
Sources are whatever your addons return; only BitTorrent ones can be 
played, and the ones that can't say so up front. 4K sources
are hidden by default — the Switch outputs 1080p docked, so they cost bandwidth
for pixels it cannot show. Turn that off in Options.

## Video corruption and recovery

The player renders each video image into its own cleared framebuffer and copies
it to the screen without blending with previous frames. Failed renders show
black rather than reusing a partly drawn image. The surface is recreated on
output-size changes (handheld/docked), and GPU resources are freed on exit.

The build also backports averne's documented H.264 reference-index fix into
FFmpeg 7.1. It prevents reference-list reordering from overwriting frame indices
used by hardware decoders. This is a known codec defect; it has not yet been
confirmed as the sole cause of the reported Scrubs playback corruption.

Reported video decode errors trigger a bounded recovery attempt with software
decoding for the rest of that playback. If the picture is damaged without a
reported error, open **X → Recover video (software decoding)**. It resets the
decoder and seeks back to your position, preserving tracks, subtitles and pause
state. Software decoding may be slower at high resolutions. You can also disable
Hardware decoding in Options before opening a problematic video.

Host tests cover GL surface isolation, resize, failed renders and opaque output,
and H.264 conformance output. Physical Switch nvtegra playback still requires
retesting the affected source past the failure point; screenshots and host tests
cannot establish that every possible video/driver combination is fixed.

## Building

devkitPro is not needed locally — build in the official Docker image, keeping
the build tree in a named volume so it survives between runs:

```sh
docker run --rm -v st_build:/build -v "$PWD":/project \
    devkitpro/devkita64@sha256:1fc388c3a0d34bd2045a6dadcb1020e069d5f876a187fd705de14b4440c00282 \
    bash /project/scripts/package-release.sh /build
```

On Windows/Git Bash, prefix each command with `MSYS_NO_PATHCONV=1`, or the
container paths get rewritten. Adding or removing a source file needs a `cmake .`
pass first, since the sources are globbed.

Dependencies come from devkitPro's portlibs (`switch-libmpv`, `switch-curl`,
`switch-libwebp`, `switch-mbedtls`…). The packaging script rebuilds FFmpeg 7.1
with the pinned devkitPro patches and an H.264 reference-index backport before
linking the app. A plain CMake build against the stock image does **not** include
that decoder fix. Source downloads are verified with SHA-256; builds are cached
under `/build/media`. `BUILDINFO.txt` records the fix and linked codec archive.
The media build also enables HTTPS, HLS encryption and DASH, using mbedTLS,
libxml2 and the bundled CA roots. FFmpeg is built with `--enable-version3`;
its linked components retain their GPLv3-or-later license. Sources, pinned
upstream archive and all local patches are available in the build script.
borealis is vendored under `library/`.

To build and package release assets in one step, run
`bash /project/scripts/package-release.sh /build` inside that container. The
release workflow pins the tested devkitPro image. For the next update, increase
the CMake version, commit it, and push a matching tag such as `v0.5.5`.
GitHub Actions builds the NRO and publishes the NRO, SD-layout ZIP, license and
SHA256SUMS and BUILDINFO.txt. The tag must match the compiled version for update detection to work.
The workflow can also be run manually to build downloadable artifacts.

HTTP integration checks (Linux, `libmpv-dev`, `ffmpeg`, OpenSSL and Python 3):
`python3 tests/http_playback.py`. This compiles the production network setup
into a headless libmpv harness, serves generated media over local HTTP/TLS,
and checks seeking, resume, auth headers, redirects, transient errors, a cut
connection, chunked transfer, tiny clips, HLS/AES-128/live, DASH and invalid
TLS certificates. It does not emulate Switch hardware or third-party servers.

Host language regression check:
`c++ -std=c++17 -Itests/stubs -Iapp tests/subtitle_config.cpp app/config.cpp app/subtitle_selection.cpp -o /tmp/subtitle-test && /tmp/subtitle-test`.
`python3 tests/subtitle_selection.py` checks actual libmpv selection using generated
Matroska files, including a primary language overriding the file's default track,
secondary fallback, reversed preferences, duplicate languages and subtitles Off.
`tests/fixtures/hebrew.srt` exercises Hebrew, mixed Latin/digits and niqqud during
on-device playback checks. For host visual verification, run
`python3 tests/render_subtitles.py` (requires libass and Pillow) and inspect
the previews in `/tmp/nx-subtitle-preview`. Natural letter spacing and automatic
paragraph direction are required for correct Hebrew and punctuation in libass.

## Credits

- [borealis](https://github.com/xfangfang/borealis) — UI framework (vendored,
  with one local patch: see `library/borealis/VENDORED.md`)
- [mpv](https://mpv.io/) — playback
- [libutp](https://github.com/bittorrent/libutp) — uTP transport
- [Stremio](https://www.stremio.com/) — account, library and addon protocol. This
  project is not affiliated with Stremio.
- [curl](https://curl.se/docs/caextract.html) — the CA bundle in
  `assets/cacert.pem`, Mozilla's root store, which is what certificates get
  checked against on a console that mounts none
- devkitPro / libnx — toolchain
- [Liberation Fonts](https://github.com/liberationfonts/liberation-fonts) —
  Arial-compatible subtitle face; SIL OFL license in `assets/LiberationSans-LICENSE.txt`
- [DejaVu Fonts](https://dejavu-fonts.github.io/) — bundled subtitle faces,
  license in `assets/DejaVu-LICENSE.txt`

Host GL regression check (Mesa/EGL, Linux):

```sh
c++ -shared -fPIC -std=c++17 -Iapp -Ilibrary/borealis/library/include/borealis/extern \
    tests/video_surface_harness.cpp app/video_surface.cpp \
    library/borealis/library/lib/extern/glad/glad.c -ldl -o /tmp/nx-video-surface.so
python3 tests/video_surface_stress.py
```
