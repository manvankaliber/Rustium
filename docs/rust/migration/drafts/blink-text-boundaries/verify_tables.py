"""Source-to-cell/hash verifier for the frozen native/generated tables.

Run with Python 3; this is a deterministic source check, not a behavior test.
"""
import hashlib
import pathlib
import re

ROOT = pathlib.Path(__file__).parent
FAST = ROOT / "frozen/closure/configured/gen/third_party/blink/renderer/platform/text/break_iterator_data_inline_header.h"
BA = ROOT / "frozen/text_break_iterator.cc"
FAST_RS = ROOT / "src/fast_pair_data.rs"
BA_RS = ROOT / "src/break_all_data.rs"
assert hashlib.sha256(FAST.read_bytes()).hexdigest() == "a32fc28f5ffcca51e0aad5d586df4361b321906f5428b0fd0e75e391724e540b"

rows = re.findall(r"/\* ([0-9A-F]{2})[^\r\n]*?\*/ \{([^\r\n]+)\}", FAST.read_text())
assert len(rows) == 223
fast_cells = []
for n, (hx, row) in enumerate(rows):
    assert int(hx, 16) == 0x21 + n
    values = []
    for args in re.findall(r"B\(([^)]*)\)", row):
        bits = list(map(int, args.split(",")))
        assert len(bits) == 8 and set(bits) <= {0, 1}
        values.append(sum(bit << i for i, bit in enumerate(bits)))
    assert len(values) == 28
    fast_cells.extend(values)
fast_rs = [int(x, 16) for x in re.findall(r"0x([0-9a-f]{2})", FAST_RS.read_text())]
assert len(fast_rs) == 223 * 28 and fast_rs == fast_cells

cpp = BA.read_text().split("kBreakAllLineBreakClassTable = {{", 1)[1].split("}};", 1)[0]
native_rows = re.findall(r"\{\s*(0b[01]+),\s*(0b[01]+),\s*(0b[01]+),\s*(0b[01]+),\s*(0b[01]+),\s*(0b[01]+),\s*(0b[01]+)\s*\},\s*//\s*(\w+)", cpp)
assert len(native_rows) == 49
native_cells = [int(cell, 2) for row in native_rows for cell in row[:7]]
ba_rows = re.findall(r"^\s+\[((?:0x[0-9a-f]{2},\s*){6}0x[0-9a-f]{2})\], //", BA_RS.read_text(), re.M)
ba_rs = [int(x, 16) for row in ba_rows for x in re.findall(r"0x([0-9a-f]{2})", row)]
assert len(ba_rs) == 49 * 7 and ba_rs == native_cells
print("verified fast pair cells: 6244; BreakAll cells: 343; frozen fast SHA-256 matches")
