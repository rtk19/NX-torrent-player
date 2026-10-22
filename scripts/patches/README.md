These patches are applied to the SHA-256-pinned FFmpeg 7.1 source archive by
`scripts/prepare-switch-ffmpeg.sh`. FFmpeg retains its upstream licenses.

- `ffmpeg-7.1-switch.patch` and `ffmpeg-7.1-tls.patch` are the unmodified devkitPro
  switch-ffmpeg 7.1-5 patches, from
  https://github.com/devkitPro/portlibs/tree/b9e8b0de6a6423557fd2a57dc100bf327c79dd4a/switch/ffmpeg
  (their hashes are checked by the build script).
- `ffmpeg-h264-reference-indices.patch` backports averne's
  https://github.com/averne/FFmpeg/commit/20c05d17bbf51ece3f2f43ad719753b5c2d0dcd4
  to FFmpeg 7.1 without changing its meaning. Reference reordering must not
  overwrite the parent picture's index, which hardware decoders use to identify
  references. The reorder list uses a local index instead.
- `ffmpeg-mbedtls-verify.patch` makes a certificate-verification failure return
  an error rather than the successful handshake result (zero). This fork uses
  FFmpeg's mbedTLS backend and the bundled Mozilla CA file for verified HTTPS,
  including playlist segments and keys. The libnx TLS backend is disabled;
  the hardware decoder remains enabled. Enabling mbedTLS with GPL components
  requires FFmpeg's `--enable-version3` (GPLv3 or later for the linked build).

Native FFmpeg 7.1 with mbedTLS and both local backports also decoded all 192
frames of generated HTTP/HTTPS MP4, redirected/retried/truncated responses,
chunked transfer, HTTPS HLS/AES-128 and HTTPS DASH identically to the local file.
Untrusted and hostname-mismatched certificates were rejected. The optional
`NX_FFMPEG_TRANSPORT=/path/to/ffmpeg python3 tests/http_playback.py` check
reproduces this comparison with a native FFmpeg build.

Native FFmpeg 7.1 built with this backport decoded MR6_BT_B (60 frames) and
MR8_BT_B (58 frames), matching every frame's official FFmpeg FATE checksum.
These are the two ITU conformance samples identified by the fix's author.
Samples: https://fate-suite.ffmpeg.org/h264-conformance/
References: FFmpeg 7.1 `tests/ref/fate/h264-conformance-mr{6,8}_bt_b`.
This checks software decode regressions; it does not emulate Switch nvtegra.
