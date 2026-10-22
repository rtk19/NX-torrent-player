"""Verify actual libmpv track selection with generated, tagged Matroska files."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

repo = Path(__file__).resolve().parents[1]

def run(args):
    result = subprocess.run([str(x) for x in args], capture_output=True, text=True)
    assert result.returncode == 0, (args, result.stdout, result.stderr)

with tempfile.TemporaryDirectory(prefix="nx-sub-languages-") as tmp:
    root = Path(tmp)
    flags = os.environ.get("NX_MPV_TEST_FLAGS")
    flags = shlex.split(flags) if flags else subprocess.check_output(
        ["pkg-config", "--cflags", "--libs", "mpv"], text=True).split()
    harness = root / "select"
    run(["c++", "-std=c++17", "-I" + str(repo / "tests/stubs"), "-I" + str(repo / "app"),
         repo / "tests/subtitle_selection_harness.cpp", repo / "app/config.cpp", "-o", harness, *flags])
    for lang, text in (("he", "כתוביות בעברית"), ("en", "English subtitles"), ("fr", "Sous-titres")):
        (root / (lang + ".srt")).write_text("1\n00:00:00,000 --> 00:00:02,000\n" + text + "\n", encoding="utf-8")

    def media(name, languages):
        file = root / (name + ".mkv")
        command = ["ffmpeg", "-v", "error", "-f", "lavfi", "-i", "color=size=64x64:rate=1:duration=3"]
        for lang, tag in languages:
            command += ["-i", root / (lang + ".srt")]
        command += ["-map", "0:v"]
        for i in range(len(languages)):
            command += ["-map", f"{i + 1}:s:0"]
        command += ["-c:v", "mpeg4", "-c:s", "srt"]
        for i, (lang, tag) in enumerate(languages):
            command += [f"-metadata:s:s:{i}", "language=" + tag,
                        f"-disposition:s:{i}", "default" if i == 0 else "0"]
        run([*command, file])
        return file

    both = media("both", [("en", "eng"), ("he", "heb")])
    secondary = media("secondary", [("fr", "fra"), ("en", "eng")])
    primary = media("primary", [("he", "he")])
    cases = [("primary wins over default English", both, "he", "en", "heb"),
             ("secondary wins over default French", secondary, "he", "en", "eng"),
             ("configured primary order", both, "en", "he", "eng"),
             ("configured secondary French", secondary, "he", "fr", "fra"),
             ("disabled secondary preserves primary", primary, "he", "none", "he"),
             ("same language in both settings", both, "he", "he", "heb"),
             ("subtitles off stays off", both, "he", "en", "off")]
    for label, file, first, second, expected in cases:
        run([harness, file, first, second, expected])
        print(label + ": PASS", flush=True)
    print(f"Subtitle language selection: {len(cases)} real-player scenarios PASS")
