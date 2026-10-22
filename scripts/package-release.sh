#!/usr/bin/env bash
set -euo pipefail

# Run inside devkitpro/devkita64 with the repository and build tree mounted.
repo_dir="$(cd "$(dirname "$0")/.." && pwd)"
build_dir="${1:-/build}"
version="$(sed -nE 's/^set\(VERSION_(MAJOR|MINOR|ALTER) "([0-9]+)"\)/\2/p' "$repo_dir/CMakeLists.txt" | paste -sd .)"
if [[ -n "${GITHUB_REF_NAME:-}" && "${GITHUB_REF_TYPE:-}" == tag && "$GITHUB_REF_NAME" != "v$version" ]]; then
    echo "Tag $GITHUB_REF_NAME does not match CMake version v$version" >&2
    exit 1
fi
bash "$repo_dir/scripts/prepare-switch-ffmpeg.sh" "$build_dir/media"

cmake -S "$repo_dir" -B "$build_dir" -DPLATFORM_SWITCH=ON -DUSE_EGL=ON -DCMAKE_BUILD_TYPE=Release
cmake --build "$build_dir" --target NX-torrent-player.nro --parallel "${BUILD_JOBS:-4}"
python3 "$repo_dir/tests/validate_nro.py" "$build_dir/NX-torrent-player.nro"

dist_dir="$repo_dir/dist"
mkdir -p "$dist_dir/switch/NX-torrent-player"
cp "$build_dir/NX-torrent-player.nro" "$dist_dir/NX-torrent-player.nro"
cp "$build_dir/NX-torrent-player.nro" "$dist_dir/switch/NX-torrent-player/"
cp "$repo_dir/assets/DejaVu-LICENSE.txt" "$dist_dir/DejaVu-LICENSE.txt"
cp "$repo_dir/assets/LiberationSans-LICENSE.txt" "$dist_dir/LiberationSans-LICENSE.txt"
chmod 644 "$dist_dir/DejaVu-LICENSE.txt" "$dist_dir/LiberationSans-LICENSE.txt"
{
    echo "NX Torrent Player $version"
    echo "FFmpeg 7.1 + devkitPro switch-ffmpeg 7.1-5 patches"
    echo "H.264 reference-index fix: averne/FFmpeg@20c05d17bbf51ece3f2f43ad719753b5c2d0dcd4"
    sha256sum "$repo_dir/scripts/patches/ffmpeg-h264-reference-indices.patch"
    sha256sum /opt/devkitpro/portlibs/switch/lib/libavcodec.a
} > "$dist_dir/BUILDINFO.txt"
cd "$dist_dir"
rm -f "NX-torrent-player-$version.zip"
zip -r "NX-torrent-player-$version.zip" switch DejaVu-LICENSE.txt LiberationSans-LICENSE.txt
sha256sum NX-torrent-player.nro "NX-torrent-player-$version.zip" BUILDINFO.txt > SHA256SUMS
