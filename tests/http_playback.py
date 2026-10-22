"""Exercise the production network options and startup policy with real libmpv.

Local HTTP/TLS fixtures cover Range, redirect, addon auth headers, retries,
truncated responses, short/chunked clips, HLS (also AES-128), DASH and errors.
No internet media, account credentials or physical Switch are involved.
"""
import http.server
import json
import os
from pathlib import Path
import re
import shlex
import socket
import ssl
import subprocess
import tempfile
import threading
from urllib.parse import urlsplit

REPO = Path(__file__).resolve().parents[1]


def run(args, **kw):
    return subprocess.run([str(a) for a in args], check=True, capture_output=True, **kw)


with tempfile.TemporaryDirectory(prefix="nx-http-test-") as tmp:
    root = Path(tmp)
    harness = root / "player"
    explicit_flags = os.environ.get("NX_MPV_TEST_FLAGS")
    flags = shlex.split(explicit_flags) if explicit_flags else subprocess.check_output(
        ["pkg-config", "--cflags", "--libs", "mpv"], text=True).split()
    run(["c++", "-std=c++17", "-I" + str(REPO / "app"),
         REPO / "tests/mpv_network_harness.cpp", REPO / "app/mpv_network.cpp",
         REPO / "app/media_source.cpp", REPO / "app/json.cpp", "-o", harness, *flags])
    # Fast-start MP4 works on nonseekable/chunked servers; ordinary MP4 tests
    # the Range-based reads needed when the index is at the end of the file.
    run(["ffmpeg", "-v", "error", "-f", "lavfi", "-i", "testsrc2=size=320x180:rate=24",
         "-f", "lavfi", "-i", "sine=frequency=440:sample_rate=48000", "-t", "8",
         "-c:v", "libx264", "-preset", "ultrafast", "-g", "48", "-c:a", "aac", root / "video.mp4"])
    run(["ffmpeg", "-v", "error", "-i", root / "video.mp4", "-c", "copy", "-movflags", "+faststart", root / "fast.mp4"])
    run(["ffmpeg", "-v", "error", "-i", root / "fast.mp4", "-t", "0.4", "-c", "copy", "-movflags", "+faststart", root / "tiny.mp4"])
    for name in ("hls", "encrypted", "dash"):
        (root / name).mkdir()
    (root / "encrypted/key").write_bytes(bytes(range(16)))
    (root / "keyinfo").write_text("key\n" + str(root / "encrypted/key") + "\n")
    for name in ("hls", "encrypted"):
        extra = ["-hls_key_info_file", root / "keyinfo"] if name == "encrypted" else []
        run(["ffmpeg", "-v", "error", "-i", root / "video.mp4", "-c", "copy", "-f", "hls",
             "-hls_time", "2", "-hls_list_size", "0", *extra, root / name / "index.m3u8"])
    (root / "hls/live.m3u8").write_text((root / "hls/index.m3u8").read_text().replace("#EXT-X-ENDLIST", ""))
    run(["ffmpeg", "-v", "error", "-i", root / "video.mp4", "-c", "copy", "-f", "dash",
         "-seg_duration", "2", root / "dash/index.mpd"])
    # Locally trusted cert, plus bad trust and hostname cases.
    for name, hostname in (("cert", "localhost"), ("wrong", "wrong.invalid")):
        run(["openssl", "req", "-x509", "-newkey", "rsa:2048", "-nodes", "-days", "1",
             "-subj", "/CN=" + hostname, "-addext", "subjectAltName=DNS:" + hostname,
             "-keyout", root / (name + ".key"), "-out", root / (name + ".pem")])
    expected = {"User-Agent": "Player, 1.0", "Referer": "https://site.test/?x=a,b",
                "Cookie": "session=a=b; mode=c,d"}
    addon = root / "stream.json"
    addon.write_text(json.dumps({"behaviorHints": {"proxyHeaders": {"request": expected}}}))
    counts = {}
    ranges = []
    failures = []

    class Handler(http.server.BaseHTTPRequestHandler):
        protocol_version = "HTTP/1.1"

        def log_message(self, *args):
            pass

        def do_GET(self):
            path = urlsplit(self.path).path
            if "?token=" in self.path and self.path != "/video.mp4?token=a%2fb&x=A,B":
                failures.append((self.path, "signed query modified"))
            counts[path] = counts.get(path, 0) + 1
            if any(self.headers.get(k) != v for k, v in expected.items()):
                failures.append((path, "headers"))
                self.send_error(403)
                return
            if path == "/redirect":
                self.send_response(302)
                self.send_header("Location", "/video.mp4?token=a%2fb&x=A,B")
                self.send_header("Content-Length", "0")
                self.end_headers()
                return
            if path in ("/forbidden", "/missing"):
                self.send_error(403 if path == "/forbidden" else 404)
                return
            if path == "/retry" and counts[path] < 3:
                self.send_error(503)
                return
            filename = {"/retry": "video.mp4", "/cut.mp4": "fast.mp4", "/chunked": "fast.mp4"}.get(path, path.lstrip("/"))
            file = root / filename
            if not file.is_file():
                self.send_error(404)
                return
            data = file.read_bytes()
            offset, end = 0, len(data) - 1
            header = self.headers.get("Range", "")
            match = re.fullmatch(r"bytes=(\d+)-(\d*)", header)
            if match:
                offset = int(match[1])
                end = min(end, int(match[2])) if match[2] else end
                ranges.append((path, offset))
            if offset > end:
                self.send_error(416)
                return
            if path == "/chunked":
                self.send_response(200)
                self.send_header("Transfer-Encoding", "chunked")
                self.end_headers()
                try:
                    for start in range(0, len(data), 4096):
                        chunk = data[start:start + 4096]
                        self.wfile.write(("%x\r\n" % len(chunk)).encode() + chunk + b"\r\n")
                    self.wfile.write(b"0\r\n\r\n")
                except (BrokenPipeError, ConnectionResetError):
                    pass
                return
            self.send_response(206 if match else 200)
            self.send_header("Accept-Ranges", "bytes")
            self.send_header("Content-Length", str(end - offset + 1))
            if match:
                self.send_header("Content-Range", f"bytes {offset}-{end}/{len(data)}")
            if filename.endswith(".m3u8"):
                self.send_header("Content-Type", "application/vnd.apple.mpegurl")
            elif filename.endswith(".mpd"):
                self.send_header("Content-Type", "application/dash+xml")
            self.end_headers()
            if path == "/cut.mp4" and counts[path] == 1:
                self.wfile.write(data[offset:offset + 32768])
                self.wfile.flush()
                self.close_connection = True
                self.connection.shutdown(socket.SHUT_RDWR)
                return
            try:
                self.wfile.write(data[offset:end + 1])
            except (BrokenPipeError, ConnectionResetError):
                pass

    servers = []
    for cert in (None, "cert", "wrong"):
        server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), Handler)
        server.daemon_threads = True
        if cert:
            ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
            ctx.load_cert_chain(root / (cert + ".pem"), root / (cert + ".key"))
            server.socket = ctx.wrap_socket(server.socket, server_side=True)
        threading.Thread(target=server.serve_forever, daemon=True).start()
        servers.append(server)
    http = f"http://localhost:{servers[0].server_port}"
    https = f"https://localhost:{servers[1].server_port}"
    wrong = f"https://localhost:{servers[2].server_port}"
    cases = [("HTTP + seek", http + "/video.mp4?token=a%2fb&x=A,B", "cert", "0", "seek", False),
             ("HTTPS + resume", https + "/video.mp4", "cert", "4", "", False),
             ("redirect", http + "/redirect", "cert", "0", "", False),
             ("503 retry", http + "/retry", "cert", "0", "", False),
             ("disconnect + Range recovery", http + "/cut.mp4", "cert", "0", "", False),
             ("chunked nonseekable", http + "/chunked", "cert", "0", "", False),
             ("short clip", http + "/tiny.mp4", "cert", "0", "", False),
             ("HLS HTTPS", https + "/hls/index.m3u8", "cert", "0", "", False),
             ("AES-128 HLS", https + "/encrypted/index.m3u8", "cert", "0", "", False),
             ("live HLS", http + "/hls/live.m3u8", "cert", "0", "live", False),
             ("DASH HTTPS", https + "/dash/index.mpd", "cert", "0", "", False),
             ("403", http + "/forbidden", "cert", "0", "", True),
             ("404", http + "/missing", "cert", "0", "", True),
             ("untrusted TLS", https + "/video.mp4", "wrong", "0", "", True),
             ("TLS hostname mismatch", wrong + "/video.mp4", "wrong", "0", "", True)]
    try:
        for label, url, ca, resume, mode, expect_error in cases:
            result = subprocess.run([str(harness), url, str(addon), str(root / (ca + ".pem")), resume, mode],
                                    capture_output=True, text=True, timeout=35)
            assert result.returncode == (2 if expect_error else 0), (label, result.stdout, result.stderr)
            state = json.loads(result.stdout)
            if not expect_error:
                assert state["loaded"] and state["frame"] and state["started"], (label, state)
                if label != "short clip":
                    assert state["position"] >= (3 if mode == "live" else 6), (label, state)
                if mode == "seek":
                    assert state["seek"], state
            print(label + ": PASS", flush=True)
        assert not failures, failures
        assert counts["/retry"] >= 3
        assert any(p == "/cut.mp4" and n > 0 for p, n in ranges), ranges
        assert counts["/forbidden"] == 1 and counts["/missing"] == 1
        print(f"HTTP playback: {len(cases)} scenarios PASS")
        # Optional exact-version backend check. Build FFmpeg 7.1 with mbedTLS
        # and our verification patch, then set NX_FFMPEG_TRANSPORT to its CLI.
        # Compare decoded pixels, not just the ability to open a URL.
        native = os.environ.get("NX_FFMPEG_TRANSPORT")
        if native:
            def hashes(args):
                output = root / "frames.md5"
                output.unlink(missing_ok=True)
                result = subprocess.run([native, "-v", "error", *args, "-an", "-c:v", "rawvideo",
                                         "-f", "framemd5", str(output)], capture_output=True, text=True, timeout=35)
                text = output.read_text() if output.exists() else ""
                return result, [line.split(",")[-1].strip() for line in text.splitlines()
                                if line and not line.startswith("#")]
            baseline, reference = hashes(["-i", str(root / "video.mp4")])
            assert baseline.returncode == 0 and len(reference) == 192, baseline.stderr
            headers = "".join(k + ": " + v + "\r\n" for k, v in expected.items())
            counts.clear()
            for label, url, ca, resume, mode, expect_error in cases:
                if label in ("short clip", "live HLS"):
                    continue
                tls = ["-tls_verify", "1", "-ca_file", str(root / (ca + ".pem"))] if url.startswith("https:") else []
                result, decoded = hashes(["-headers", headers, *tls,
                                          "-reconnect", "1", "-reconnect_on_network_error", "1",
                                          "-reconnect_on_http_error", "5xx", "-reconnect_delay_max", "3", "-i", url])
                if expect_error:
                    assert result.returncode != 0 and not decoded, (label, result.stdout, result.stderr)
                else:
                    assert result.returncode == 0 and decoded == reference, (label, len(decoded), result.stderr)
                print("FFmpeg 7.1/mbedTLS " + label + ": PASS", flush=True)
            assert not failures, failures
    finally:
        for server in servers:
            server.shutdown()
