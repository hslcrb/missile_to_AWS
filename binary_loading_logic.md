# Binary Payload Loading Logic Analysis

The `missile_to_aws` application reconstructs its core functional binary (likely `aws-nuke.exe`) at runtime from encrypted data fragments.

## 1. File Structure
- **Directory**: `data/`
- **File Pattern**: `data.xxx` (`data.000`, `data.001`, ..., up to `data.099`).
- **Function**: Reconstructs a full binary in memory for process injection.

## 2. Decryption Mechanism ([LoadAndDecryptBinary](file:///f:/aws-nuke/cpp_doom/main.cpp#362-380))
The application uses a simple **XOR-based decryption** scheme:
- **XOR Key**: `0x0AB`
- **Logic**: Each byte in the `.dat` files is XORed with `0xAB` during the reading process.
- **Sequence**: Files are loaded sequentially. If a file in the sequence is missing, the loading process halts.

## 3. Reassembly and Execution
1. **Reassembly**: All decrypted chunks are appended into a single `std::vector<unsigned char>` named `g_binaryPayload`.
2. **Timing**: The reassembly happens once (lazy-loaded) when the "NUKE" process is first launched.
3. **Execution**: The resulting binary is injected into a suspended host process (`svchost.exe`) via **Process Hollowing** in the [ProcessHollow](file:///f:/aws-nuke/cpp_doom/main.cpp#502-583) function.

## 4. Key Code Implementation (main.cpp: 341-358)
```cpp
std::vector<unsigned char> LoadAndDecryptBinary() {
    std::vector<unsigned char> buffer;
    unsigned char xor_key = 0xAB;
    
    for (int i = 0; i < 100; ++i) {
        wchar_t filename[256];
        swprintf(filename, 256, L"data/data.%03d", i);
        
        std::ifstream f(filename, std::ios::binary);
        if (!f.is_open()) break;
        
        std::vector<unsigned char> chunk((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        for (auto& b : chunk) b ^= xor_key;
        
        buffer.insert(buffer.end(), chunk.begin(), chunk.end());
    }
    return buffer;
}
```
