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
- **Stremio integration** — sign in, browse your library, drill down through
  seasons → episodes → addons → sources, artwork and all.
- **Local torrents & magnets** — drop `.torrent` files on the SD card, or add
  magnet links (a `magnet.txt` file, or the on-screen keyboard). A magnet's name
  and file list are resolved and cached so you can pick which video to stream.
- **Player** — audio/subtitle track menu, analog scrubbing (either stick), touch
  controls, a control lock (Y), and auto-resume where you left off.
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
`he`, `heb` or `iw` are preferred, and Hebrew Stremio addon results are listed
first. Select an addon subtitle in the player's **X** menu to download it.

DejaVu Sans regular/bold are bundled inside the NRO, so no font installation on
the SD card is needed. libass handles right-to-left text, mixed Hebrew/English,
digits and combining marks. UTF-8 remains supported; legacy Hebrew text uses
Windows-1255 as the fallback when Hebrew is preferred (or when downloading a
Hebrew addon subtitle). Bitmap subtitles retain their original appearance.
This adds subtitle support; the app's interface languages remain English/French.

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
borealis is vendored under `library/`.

To build and package release assets in one step, run
`bash /project/scripts/package-release.sh /build` inside that container. The
release workflow pins the tested devkitPro image. For the next update, increase
the CMake version, commit it, and push a matching tag such as `v0.5.4`.
GitHub Actions builds the NRO and publishes the NRO, SD-layout ZIP, license and
SHA256SUMS and BUILDINFO.txt. The tag must match the compiled version for update detection to work.
The workflow can also be run manually to build downloadable artifacts.

Host language regression check:
`c++ -std=c++17 -Itests/stubs -Iapp tests/subtitle_config.cpp app/config.cpp -o /tmp/subtitle-test && /tmp/subtitle-test`.
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
- [DejaVu Fonts](https://dejavu-fonts.github.io/) — bundled subtitle faces,
  license in `assets/DejaVu-LICENSE.txt`

Host GL regression check (Mesa/EGL, Linux):

```sh
c++ -shared -fPIC -std=c++17 -Iapp -Ilibrary/borealis/library/include/borealis/extern \
    tests/video_surface_harness.cpp app/video_surface.cpp \
    library/borealis/library/lib/extern/glad/glad.c -ldl -o /tmp/nx-video-surface.so
python3 tests/video_surface_stress.py
```
