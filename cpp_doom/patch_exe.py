import hashlib
import os

token_placeholder = "SHA256_HASH_VALIDATION_TOKEN_XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX".encode('utf-16-le')
exe_name = "missile-to-aws.exe"

if not os.path.exists(exe_name):
    print(f"Error: {exe_name} not found")
    exit(1)

# 1. Read the EXE
with open(exe_name, "rb") as f:
    data = f.read()

# 2. Find the token position
pos = data.find(token_placeholder)
if pos == -1:
    print("Error: Could not find placeholder token in EXE")
    exit(1)

print(f"Token found at offset: {hex(pos)}")

# 3. Calculate SHA256 of the EXE (including the placeholder for now)
# To be consistent with VerifyEXEIntegrity, we should hash the whole thing.
# The VerifyEXEIntegrity in main.cpp checks:
# if (wcscmp(g_exeMasterHash, L"TOKEN") == 0) return true;
# else return VerifySHA256(exeData, g_exeMasterHash);
# So I need to calculate the hash OF THE EXE AFTER PATCHING?
# No, that's impossible to match perfectly in one step because changing the hash changes the hash.

# FIXED STRATEGY:
# The Checksum will be calculated for the EXE *EXCLUDING* the 128 bytes of the g_exeMasterHash string.
# In main.cpp, I should modify VerifyEXEIntegrity to zero out or skip that part.

# Alternative: Hardcode the hash of JUST THE RESOURCE SECTION.
# That is much more stable.
