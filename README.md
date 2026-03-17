# Missile-to-AWS (MTA)

[![Language](https://img.shields.io/badge/언어-C%2B%2B-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B)
[![GUI](https://img.shields.io/badge/GUI-Win32_API-lightgrey.svg)](https://docs.microsoft.com/en-us/windows/win32/)

**Missile-to-AWS (MTA)**는 `aws-nuke` 도구를 보다 안전하고 효율적으로 실행하기 위해 Win32 API로 밑바닥부터 재구축한 전용 래퍼 프로그램입니다. 인메모리 실행 및 프로세스 할로잉(Process Hollowing) 기술을 적용하여 보안성을 극대화했습니다.

---

## 🛠 주요 기능

- **순수 Win32 API 구현**: Tauri나 Next.js와 같은 무거운 프레임워크 없이 C++와 Win32 API, GDI+만을 사용하여 매우 가볍고 빠릅니다.
- **인메모리 바이너리 로딩**: `data/` 디렉토리에 XOR 암호화(Key: `0xAB`)되어 분할 저장된 `aws-nuke.exe` 조각들을 런타임에 메모리상에서 재조립하여 실행합니다.
- **프로세스 할로잉 (Process Hollowing)**:
    - 재조립된 바이너리를 디스크에 쓰지 않고, 일시 중지된 `svchost.exe` 프로세스 내부에 직접 주입하여 실행합니다.
    - 파일 시스템에 흔적을 남기지 않는 제로-풋프린트(Zero-footprint) 실행을 지원합니다.
- **대화형 터미널 에뮬레이터**:
    - 주입된 프로세스의 `stdout`/`stderr` 출력을 실시간으로 파이핑하여 표시합니다.
    - 사용자 확인이 필요한 경우를 위해 `stdin` 리다이렉션을 통한 실시간 입력 기능을 지원합니다.
    - `Ctrl+A`(전체 선택) 등 표준 단축키를 지원하는 커스텀 에디트 컨트롤을 탑재했습니다.
- **안전한 자격 증명 관리**:
    - AWS Secret Access Key는 기본적으로 `●` 문자로 마스킹 처리되며, 필요한 경우 토글하여 확인할 수 있습니다.
    - 설정된 계정 ID, 액세스 키, 리전 선택 정보는 `_mta.json` 및 AWS 표준 설정 파일에 실시간으로 동기화됩니다.
- **상태 인디케이터**:
    - 실시간 IME 상태 감지 (영어 `[A]` vs 한국어 `[한]`) 및 Caps Lock 활성화 여부를 시각적으로 표시합니다.
- **비동기 처리 구조**: 모든 I/O 작업과 프로세스 주입 과정을 별도 쓰레드에서 처리하여 GUI의 응답성을 상시 유지합니다.

## 🏗 시스템 아키텍처

### 1. 페이로드 보호 (Payload Protection)
실행 파일(`aws-nuke.exe`)은 약 100개의 암호화된 세그먼트로 나뉘어 저장됩니다. 이는 정적 분석을 통한 바이너리 탐지를 방지하며, 프로그램 실행 시에만 비공개 힙(Heap) 메모리에서 복구됩니다.

### 2. 프로세스 주입 파이프라인
1. **생성**: `svchost.exe`를 `CREATE_SUSPENDED` 상태로 생성합니다.
2. **할로잉**: 대상 프로세스의 메모리를 언맵(Unmap)하고 페이로드의 NT 헤더에 맞춰 새로운 공간을 할당합니다.
3. **매핑**: 재조립된 페이로드의 헤더와 섹션들을 원격 프로세스 메모리에 직접 작성합니다.
4. **연결**: 익명 파이프(Anonymous Pipe)를 생성하여 프로세스의 표준 입출력을 MTA GUI와 연결합니다.
5. **재개**: 쓰레드 컨텍스트(ImageBase, EntryPoint)를 수정 후 실행을 재개합니다.

## 🚀 빌드 및 실행

### 요구 사항
- **Visual Studio 2022** (C++ 데스크톱 개발 워크로드 포함)
- **Windows SDK**

### 빌드 단계
1. **VS 2022용 x64 Native Tools Command Prompt**를 실행합니다.
2. `cpp_doom` 디렉토리로 이동합니다.
3. 빌드 스크립트를 실행합니다:
   ```cmd
   build.bat
   ```
   이 스크립트는 `rc.exe`를 통해 리소스를 컴파일하고, `cl.exe`를 통해 최적화된 `missile-to-aws.exe`를 생성합니다.

## 📂 프로젝트 구조
- `main.cpp`: 메인 로직, Win32 GUI 및 프로세스 할로잉 구현부
- `resource.h` / `resources.rc`: 아이콘 및 이미지 리소스 정의
- `data/`: 암호화된 바이너리 조각들이 저장된 폴더
- `external/`: 생성된 AWS 설정 파일들 (`config.yaml`, `credentials.json`)
- `_mta.json`: 프로그램 상태 및 사용자 설정 저장 파일

---

> [!CAUTION]
> **주의**: 이 도구는 AWS 리소스를 파괴적으로 삭제합니다. 실행 전 반드시 리전 선택과 계정 블록리스트를 확인하십시오.
