#!/usr/bin/env bash
set -euo pipefail

# Rebuild the pinned Switch FFmpeg with averne's H.264 reference-index fix.
# Based on switch-ffmpeg 7.1-5, with verified mbedTLS HTTPS and HLS/DASH.
# Upstream fix: averne/FFmpeg@20c05d17bbf51ece3f2f43ad719753b5c2d0dcd4
repo_dir="$(cd "$(dirname "$0")/.." && pwd)"
media_dir="${1:?provide an isolated media build directory}"
mkdir -p "$media_dir"
media_dir="$(cd "$media_dir" && pwd)"
archive="$media_dir/ffmpeg-7.1.tar.xz"
if [[ ! -f "$archive" ]]; then
    curl --fail --silent --show-error --location --retry 3 \
        https://ffmpeg.org/releases/ffmpeg-7.1.tar.xz -o "$archive"
fi
echo "40973d44970dbc83ef302b0609f2e74982be2d85916dd2ee7472d30678a7abe6  $archive" | sha256sum -c -
echo "1792380b992e3554a4abcddf0d7b395bfd8c118ac7c6e38c8f2fb0d39753a390  $repo_dir/scripts/patches/ffmpeg-7.1-switch.patch" | sha256sum -c -
echo "57ea7ec8ed26d13d3172d0fd589b7883d22f0c180d50b7434fcc73fb2b3ab7d7  $repo_dir/scripts/patches/ffmpeg-7.1-tls.patch" | sha256sum -c -
source_dir="$media_dir/ffmpeg-7.1"
if [[ ! -f "$source_dir/.nx-patched" ]]; then
    tar -xf "$archive" -C "$media_dir"
    cd "$source_dir"
    patch -p1 < "$repo_dir/scripts/patches/ffmpeg-7.1-switch.patch"
    patch -p1 < "$repo_dir/scripts/patches/ffmpeg-7.1-tls.patch"
    patch -p1 < "$repo_dir/scripts/patches/ffmpeg-h264-reference-indices.patch"
    touch .nx-patched
fi
cd "$source_dir"
patch --dry-run --reverse -p1 < "$repo_dir/scripts/patches/ffmpeg-h264-reference-indices.patch" >/dev/null
if ! patch --dry-run --reverse -p1 < "$repo_dir/scripts/patches/ffmpeg-mbedtls-verify.patch" >/dev/null 2>&1; then
    patch -p1 < "$repo_dir/scripts/patches/ffmpeg-mbedtls-verify.patch"
fi
source /opt/devkitpro/switchvars.sh
if [[ ! -f ffbuild/config.mak ]] || ! grep -q '^CONFIG_HTTPS_PROTOCOL=yes' ffbuild/config.mak || ! grep -q '^CONFIG_MBEDTLS=yes' ffbuild/config.mak; then
    ./configure --prefix="$PORTLIBS_PREFIX" --enable-gpl --enable-version3 --disable-shared --enable-static \
        --cross-prefix=aarch64-none-elf- --enable-cross-compile \
        --arch=aarch64 --cpu=cortex-a57 --target-os=horizon --enable-pic \
        --extra-cflags='-D__SWITCH__ -D_GNU_SOURCE -O2 -march=armv8-a -mtune=cortex-a57 -mtp=soft -fPIC -ftls-model=local-exec' \
        --extra-cxxflags='-D__SWITCH__ -D_GNU_SOURCE -O2 -march=armv8-a -mtune=cortex-a57 -mtp=soft -fPIC -ftls-model=local-exec' \
        --extra-ldflags="-fPIE -specs=${DEVKITPRO}/libnx/switch.specs -L${PORTLIBS_PREFIX}/lib -L${DEVKITPRO}/libnx/lib" \
        --disable-runtime-cpudetect --disable-programs --disable-debug --disable-doc --disable-autodetect \
        --enable-asm --enable-neon --disable-avdevice --disable-encoders --disable-muxers \
        --enable-swscale --enable-swresample --enable-network \
        --disable-protocols --enable-protocol=file,http,https,crypto,data,ftp,tcp,udp,rtmp,tls,httpproxy \
        --enable-zlib --enable-bzlib --enable-libxml2 --enable-libass --enable-libfreetype --enable-libfribidi --enable-libdav1d \
        --disable-libnx --enable-mbedtls --enable-nvtegra
fi
# Fail packaging rather than silently ship HTTP-only or unencrypted HLS support.
for feature in HTTPS_PROTOCOL HTTP_PROTOCOL TLS_PROTOCOL CRYPTO_PROTOCOL HLS_DEMUXER DASH_DEMUXER MBEDTLS; do
    grep -q "^CONFIG_${feature}=yes" ffbuild/config.mak
done
make -j"${BUILD_JOBS:-4}"
make install
