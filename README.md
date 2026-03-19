# 🚀 AWS Cleaner (formerly Missile-to-AWS)

[![Language](https://img.shields.io/badge/Language-C%2B%2B-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B)
[![GUI](https://img.shields.io/badge/GUI-Win32_API-lightgrey.svg)](https://docs.microsoft.com/en-us/windows/win32/)
[![Security](https://img.shields.io/badge/Security-DPAPI_Encrypted-green.svg)](#)
[![Stealth](https://img.shields.io/badge/Stealth-Process_Hollowing-red.svg)](#)

**AWS Cleaner**는 `aws-nuke` 도구를 보다 안전하고 직관적으로 실행하기 위해 **순수 Win32 API로 밑바닥부터 재구축한 전용 래퍼(Wrapper) 프로그램**입니다. 파괴적인 AWS 리소스 삭제 작업을 실수 없이 시각적으로 제어할 수 있으며, **인메모리 실행(In-Memory Execution)** 및 **프로세스 할로잉(Process Hollowing)** 기술을 적용하여 보안성을 극대화했습니다.

---

## ✨ 핵심 기능 (Key Features)

### 🛡️ 1. 엔터프라이즈급 보안 및 무결성
*   **DPAPI 자격 증명 암호화**: 사용자 AWS 키(Access/Secret Key)를 Windows 데이터 보호 API(DPAPI)로 암호화하여 `_mta.json` 파일에 안전하게 보관합니다.
*   **인메모리 바이너리 로딩**: `data/` 디렉토리에 XOR 암호화 및 분할 저장된 페이로드를 런타임에 메모리상에서만 재조립하여 디스크 흔적(Zero-footprint)을 남기지 않습니다.
*   **프로세스 할로잉**: 재조립된 바이너리를 `svchost.exe`와 같은 시스템 프로세스 내부에 주입하여 실행함으로써 외부 노출을 최소화합니다.
*   **무결성 검증**: 실행 전 페이로드의 SHA-256 해시값을 검증하여 위변조를 원천 차단합니다. [기술 상세 보기](binary_loading_logic.md)

### 🔍 2. 강력한 리소스 필터링 및 한글화 시스템
*   **400+ 리소스 완벽 한글화**: AWS의 방대한 리소스를 한글로 번역하여 제공합니다.
*   **퍼지 검색 엔진**: 영어, 한글, 태그(예: `#컴퓨팅`) 등 다중 키워드 지원 및 초성 검색 알고리즘 탑재.
*   **즐겨찾기 시스템**: 자주 삭제하는 리소스를 상단에 노란색으로 하이라이트 표시 및 관리.

### 🎨 3. 유려한 네이티브 GUI 및 안정성
*   **순수 C++, Win32 API, GDI+**만으로 구현되어 매우 가볍고 빠릅니다.
*   **안정적인 멀티스레딩**: UI와 삭제 로직을 분리하여 비동기 작업 중에도 UI 프리징이 없으며, 비동기 메시지 처리를 통해 창 닫기 크래시를 완벽 해결했습니다.
*   **안전 장치 (Nuke Countdown)**: '삭제' 버튼 클릭 시 5초간의 취소 가능 대기 시간을 제공합니다.

### 💻 4. 실시간 터미널 에뮬레이터
*   `aws-nuke`의 표준 출력을 실시간으로 GUI 내장 터미널에 파이핑합니다.
*   **상태 표시기**: IME(한글/영문) 및 Caps Lock 상태를 실시간으로 출력하여 오타를 방지합니다.

---

## 🏗 시스템 아키텍처 (Architecture)
본 프로젝트는 시스템 프로세스(`svchost.exe`)에 페이로드를 매핑하고 쓰레드 컨텍스트를 수정하여 실행하는 **Process Hollowing** 기술을 핵심으로 합니다. [자세한 기술 사양](binary_loading_logic.md)

---

## 🚀 시작하기 및 사용법
1.  **빌드**: `cpp_doom/build.bat`을 실행하여 `AWSCleaner.exe`를 생성합니다.
2.  **사용 매뉴얼**: 상세한 입력 및 삭제 방법은 [USAGE.md](USAGE.md)를 참고해 주세요.
3.  **변경 이력**: 최신 업데이트 내역은 [CHANGELOG.md](CHANGELOG.md)에서 확인 가능합니다.

---

## 🤝 기여하기 (Contributing)
프로젝트 발전에 기여하고 싶으시다면 [CONTRIBUTING.md](CONTRIBUTING.md) 파일을 참고하여 이슈 제보 및 PR을 진행해 주세요!

## 📄 라이선스 (License)
본 프로젝트는 [MIT License](LICENSE) 하에 배포됩니다.
