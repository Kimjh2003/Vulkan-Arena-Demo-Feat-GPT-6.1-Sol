"""Compile Vulkan 1.0 shaders and regenerate the C++ embedded shader payload.

Usage: python tools/compile_vulkan_shaders.py --glslc /path/to/ndk/shader-tools/.../glslc
"""
from pathlib import Path
import argparse
import hashlib
import struct
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument("--glslc", default="glslc")
args = parser.parse_args()
root = Path(__file__).resolve().parents[1] / "app/src/main/cpp/vulkan/shaders"
header = ["#pragma once", "#include <cstdint>", "#include <vector>", "namespace arena_spv {"]
hashes = []
for name, symbol in [("arena.vert", "vertex"), ("arena.frag", "fragment"), ("copy.comp", None)]:
    output = root / (name + ".spv")
    subprocess.run([args.glslc, "--target-env=vulkan1.0", str(root / name), "-o", str(output)], check=True)
    data = output.read_bytes()
    hashes.append(f"{hashlib.sha256(data).hexdigest()}  {output.name}")
    if symbol:
        words = struct.unpack("<" + "I" * (len(data) // 4), data)
        header.append(f"inline const std::vector<std::uint32_t>& {symbol}() {{ static const std::vector<std::uint32_t> words={{")
        for i in range(0, len(words), 8):
            header.append(",".join(f"0x{v:08x}u" for v in words[i:i + 8]) + ",")
        header.append("};return words;}")
header.append("}")
(root / "embedded.h").write_text("\n".join(header) + "\n", encoding="utf-8")
(root / "SHA256SUMS.txt").write_text("\n".join(hashes) + "\n", encoding="utf-8")
