"""Check the shipped NRO's metadata, update feed and bundled font bytes."""
from pathlib import Path
import re
import struct
import sys

repo = Path(__file__).resolve().parents[1]
nro = Path(sys.argv[1]).read_bytes()
assert nro[0x10:0x14] == b"NRO0", "Invalid NRO header"
asset_offset = struct.unpack_from("<I", nro, 0x18)[0]
assert nro[asset_offset:asset_offset + 4] == b"ASET", "Missing NRO assets"
nacp_offset, nacp_size = struct.unpack_from("<QQ", nro, asset_offset + 0x18)
nacp = nro[asset_offset + nacp_offset:asset_offset + nacp_offset + nacp_size]
assert len(nacp) == 0x4000
assert nacp[0x200:0x300].split(b"\0")[0] == b"Refael Bar"
version = ".".join(re.findall(r'^set\(VERSION_(?:MAJOR|MINOR|ALTER) "(\d+)"\)',
                              (repo / "CMakeLists.txt").read_text(), re.M))
assert nacp[0x3060:0x3070].split(b"\0")[0].decode() == version
for suffix in (b"latest", b"tags/"):
    assert b"https://api.github.com/repos/rtk19/NX-torrent-player/releases/" + suffix in nro
assert b"https://api.github.com/repos/shodowlo/NX-torrent-player/releases/" not in nro
for font in (repo / "assets/subtitle-fonts").glob("*.ttf"):
    assert font.read_bytes() in nro, f"Missing font: {font.name}"
assert (repo / "assets/DejaVu-LICENSE.txt").read_bytes() in nro
assert (repo / "assets/LiberationSans-LICENSE.txt").read_bytes() in nro
print(f"NRO {version}: metadata, fork update endpoints, fonts and license PASS ({len(nro):,} bytes)")
