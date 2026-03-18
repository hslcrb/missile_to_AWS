import hashlib
import os

def get_sha256(filename):
    sha256_hash = hashlib.sha256()
    with open(filename, "rb") as f:
        for byte_block in iter(lambda: f.read(4096), b""):
            sha256_hash.update(byte_block)
    return sha256_hash.hexdigest()

data_dir = "data"
hashes = []
for i in range(15):
    filename = os.path.join(data_dir, f"data.{i:03d}")
    if os.path.exists(filename):
        h = get_sha256(filename)
        hashes.append(f'L"{h.upper()}"')
    else:
        break

print("const wchar_t* g_datHashes[] = {")
print("    " + ", ".join(hashes))
print("};")
print(f"const int g_numDatFiles = {len(hashes)};")
