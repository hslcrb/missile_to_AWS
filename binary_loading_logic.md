# 🛠️ Binary Payload Loading & Security Logic

AWS Cleaner는 핵심 실행 파일(`aws-nuke.exe`)을 디스크에 노출시키지 않고 메모리상에서만 안전하게 재조립하여 실행하는 **스텔스 실행 기술**을 사용합니다. 본 문서는 해당 과정의 기술적 메커니즘을 상세히 설명합니다.

---

## 1. 페이로드 구조 및 분합 (Data Fragmentation)
- **디렉토리**: `data/`
- **파일 패턴**: `data.000`, `data.001`, ..., `data.014` (현재 총 15개 조각)
- **보안 목적**: 대용량 바이너리를 잘게 쪼개어 정적 분석 도구(AV/EDR)의 시그니처 탐지를 회피하고, 각 조각에 암호화를 적용하여 원본 바이너리 유출을 방지합니다.

## 2. 암호화 및 복호화 메커니즘
### XOR 레이어
각 `.dat` 조각은 하드코딩된 단일 바이트 XOR 키(`0xAB`)로 암호화되어 있습니다.
- **XOR 키**: `0xAB`
- **복호화 로직**: `b ^= 0xAB`

### DPAPI 자격 증명 보호 (신규)
사용자의 AWS Access/Secret Key는 Windows의 **DPAPI(Data Protection API)**를 사용하여 시스템 계정 수준에서 암호화됩니다.
- **저장 위치**: `AWSCleaner_mta.json` 내 `encrypted_access_key`, `encrypted_secret_key` 필드.
- **특징**: 동일한 윈도우 사용자 계정에서만 복호화가 가능하며, 외부 유출 시 해독이 불가능합니다.

## 3. 무결성 검증 (SHA-256 Integrity Check)
바이너리 로딩 전, 프로젝트는 `VerifyEXEIntegrity()` 함수를 통해 실행 환경의 무결성을 검증합니다.
- **Windows BCrypt API**: 고속 암호화 라이브러리를 사용하여 로드된 페이로드의 SHA-256 해시를 계산합니다.
- **위변조 차단**: 예상 해시값과 다를 경우 실행을 즉각 중단하고 오류 메시지를 출력합니다.

## 4. 프로세스 할로잉 (Process Hollowing)
복호화된 바이너리는 아래의 단계를 거쳐 `svchost.exe` 내부에 주입됩니다.

1.  **Host Process Creation**: `svchost.exe`를 `CREATE_SUSPENDED` 상태로 생성.
2.  **Remote Unmapping**: 대상 프로세스의 원래 이미지 메모리를 `NtUnmapViewOfSection`으로 제거.
3.  **Memory Allocation**: 페이로드의 `SizeOfImage`에 맞춰 원격 메모리 할당.
4.  **Section Mapping**: 복호화된 페이로드의 헤더와 각 섹션(Code, Data 등)을 `WriteProcessMemory`로 주입.
5.  **Context Hijacking**: `GetThreadContext`로 원격 쓰레드 정보를 가져와서 `EAX`/`RCX` (EntryPoint) 및 `EBX`/`RDX` (ImageBase)를 수정.
6.  **Resuming**: `ResumeThread`를 호출하여 주입된 코드로 실행 개시.

---

## 5. 주요 구현 코드 (`main.cpp`)

### 바이너리 복호화 루틴
```cpp
std::vector<unsigned char> LoadAndDecryptBinary() {
    std::vector<unsigned char> buffer;
    unsigned char xor_key = 0xAB;
    for (int i = 0; i < 100; ++i) { // 최대 100개 조각 탐색
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

> [!NOTE]
> 본 기술 상세는 개발 및 분석 목적으로만 제공되며, 이를 악용하는 것은 허용되지 않습니다.
