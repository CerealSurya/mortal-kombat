import sys, struct

filename = sys.argv[1] if len(sys.argv) > 1 else "round1.wav"
varname  = sys.argv[2] if len(sys.argv) > 2 else "fightSnd"

with open(filename, "rb") as f:
    raw = f.read()

# Find the "data" chunk — handles non-standard header sizes
pos = 12
while pos < len(raw) - 8:
    chunk_id   = raw[pos:pos+4]
    chunk_size = struct.unpack_from("<I", raw, pos+4)[0]
    if chunk_id == b"data":
        pcm = raw[pos+8 : pos+8+chunk_size]
        break
    pos += 8 + chunk_size
else:
    raise RuntimeError("No 'data' chunk found — is this a valid WAV?")

print(f"const uint8_t {varname}[{len(pcm)}] = {{")
# 16 values per line for readability
for i in range(0, len(pcm), 16):
    line = ", ".join(str(b) for b in pcm[i:i+16])
    print(f"  {line},")
print("};")
print(f"#define {varname.upper()}_LEN {len(pcm)}")
