# 📁 디렉토리 전수실태 파악 보고서

> **대상 경로:** `d:\missile_to_AWS`  
> **조사 기준일:** 2026-03-26

---

## 1. 현재 디렉토리 트리 (전체)

```
d:\missile_to_AWS\
├── .git/                        (Git 저장소 내부 — 손대지 말 것)
├── .gitignore                   (95 B)
├── CHANGELOG.md                 (2 KB) ✅ 정상
├── CONTRIBUTING.md              (3 KB) ✅ 정상
├── ESDomain                     (크기 없음) ⚠️ 정체불명
├── LICENSE                      (1 KB) ✅ 정상
├── README.md                    (4 KB) ✅ 정상
├── USAGE.md                     (2 KB) ✅ 정상
├── binary_loading_logic.md      (3 KB) ✅ 정상
│
└── cpp_doom\                    (소스 및 빌드 루트)
    ├── main.cpp                 (95 KB) ✅ 핵심 소스
    ├── resource.h               (1.3 KB) ✅ 정상
    ├── resource_list.h          (92 KB) ✅ 정상
    ├── resources.rc             (1.5 KB) ✅ 정상
    ├── build.bat                (284 B) ✅ 정상
    │
    ├── AWSCleaner.exe           (156 MB) ⚠️ 빌드 산출물 — 소스와 혼재
    ├── main.obj                 (1.3 MB) ⚠️ 빌드 산출물
    ├── resources.res            (155 MB) ⚠️ 빌드 산출물
    │
    ├── 20260318wed_Alpha-Realease.zip   (29 MB) ⚠️ 릴리즈 아카이브 — 소스와 혼재
    ├── 20260319thu_Gamma-Realease.zip   (32 MB) ⚠️ 릴리즈 아카이브 — 소스와 혼재
    ├── 20260319thu_Gamma-Realease - 복사본.zip (32 MB) 🔴 중복 파일
    │
    ├── AWSCleaner_mta.json      (363 B) ℹ️ 런타임 설정 (개인정보 포함)
    ├── RCa11096                 (178 B) 🔴 정체불명 임시 파일
    ├── fav.txt                  (605 B) 🔴 임시 데이터 파일
    ├── out.txt                  (2 KB)  🔴 임시 출력 파일
    ├── build_output.txt         (2.7 KB) 🔴 빌드 로그 임시 파일
    │
    ├── gen_cpp.py               (1.6 KB) ✅ 개발 도구 스크립트
    ├── gen_hashes.py            (661 B)  ✅ 개발 도구 스크립트
    ├── gen_tags.py              (16 KB)  ✅ 개발 도구 스크립트
    │
    ├── aws_nuke_limitations_report.md      (4.5 KB) 🟡 개발 보고서
    ├── aws_nuke_replacement_feasibility.md (5.8 KB) 🟡 개발 보고서
    ├── code_optimization_report.md         (6 KB)   🟡 개발 보고서
    ├── code_review_report.md               (4.8 KB) 🟡 개발 보고서
    ├── go_to_cpp_port_feasibility.md       (5.6 KB) 🟡 개발 보고서
    ├── implementation_plan.md              (4.6 KB) 🟡 개발 보고서
    ├── walkthrough.md                      (2.8 KB) 🟡 개발 보고서
    ├── walkthrough2.md                     (3 KB)   🟡 개발 보고서 (중복?)
    │
    ├── assets\
    │   ├── AWSCleanerlogobasic_outline.ico (20 KB)
    │   ├── awsc.png                        (38 KB)
    │   ├── res_del.png                     (34 KB)
    │   ├── save_button.png                 (38 KB)
    │   └── Noto_Sans_KR\
    │       ├── OFL.txt
    │       ├── README.txt
    │       └── static\                    (폰트 파일들)
    │
    ├── data\
    │   ├── data.000 ~ data.013            (각 10 MB = 140 MB)
    │   └── data.014                       (2.5 MB)
    │   → 합계 약 142.5 MB (암호화된 aws-nuke 페이로드)
    │
    └── external\
        ├── config.yaml                    (69 B)  런타임 생성
        └── credentials.json               (65 B)  런타임 생성 ⚠️ 평문 키 포함
```

---

## 2. 문제점 분류

### 🔴 즉시 삭제/처리 대상

| 파일/항목 | 위치 | 문제 |
|-----------|------|------|
| `ESDomain` | 루트 | 내용 없는 정체불명 항목 |
| `RCa11096` | cpp_doom | 정체불명 임시 파일 (RC 컴파일러 잔재 추정) |
| `fav.txt` | cpp_doom | 임시 즐겨찾기 데이터 파일 |
| `out.txt` | cpp_doom | 임시 출력 파일 |
| `build_output.txt` | cpp_doom | 빌드 로그 임시 파일 |
| `20260319thu_Gamma-Realease - 복사본.zip` | cpp_doom | 릴리즈 ZI 복사본 (원본과 완전 동일) |

### ⚠️ 위치 재배치 필요

| 파일/항목 | 현재 위치 | 이유 | 권장 위치 |
|-----------|-----------|------|-----------|
| `AWSCleaner.exe` | cpp_doom\ | 빌드 산출물 → 소스와 혼재 | `cpp_doom\bin\` |
| `main.obj` | cpp_doom\ | 빌드 산출물 | `cpp_doom\bin\` 또는 `.gitignore` |
| `resources.res` | cpp_doom\ | 빌드 산출물 | `cpp_doom\bin\` 또는 `.gitignore` |
| `*.zip` (릴리즈) | cpp_doom\ | 릴리즈 아카이브 → 소스와 혼재 | `releases\` |
| `gen_*.py` | cpp_doom\ | 개발 도구 스크립트 | `cpp_doom\tools\` |
| `개발보고서 *.md` | cpp_doom\ | 개발 문서 → 소스와 혼재 | `docs\` 또는 `.gitignore` |

### 🟡 보안 주의

| 파일 | 위치 | 내용 | 조치 |
|------|------|------|------|
| `external\credentials.json` | cpp_doom\ | AWS 평문 키 포함 | `.gitignore`에 반드시 포함 확인 |
| `AWSCleaner_mta.json` | cpp_doom\ | DPAPI 암호화 키 포함 | `.gitignore`에 포함 확인 |

---

## 3. 제안 목표 구조

```
d:\missile_to_AWS\
├── .gitignore
├── LICENSE
├── README.md
├── CHANGELOG.md
├── CONTRIBUTING.md
├── USAGE.md
├── binary_loading_logic.md
│
├── docs\                        [신규] 개발 보고서 일괄 이동
│   ├── aws_nuke_limitations_report.md
│   ├── aws_nuke_replacement_feasibility.md
│   ├── code_optimization_report.md
│   └── go_to_cpp_port_feasibility.md
│
├── releases\                    [신규] 릴리즈 아카이브
│   ├── Alpha-20260318.zip
│   └── Gamma-20260319.zip
│
└── cpp_doom\
    ├── main.cpp
    ├── resource.h
    ├── resource_list.h
    ├── resources.rc
    ├── build.bat
    │
    ├── tools\                   [신규] 개발 도구 스크립트
    │   ├── gen_cpp.py
    │   ├── gen_hashes.py
    │   └── gen_tags.py
    │
    ├── assets\                  (현재 유지)
    ├── data\                    (현재 유지 — .gitignore 권장)
    └── external\                (런타임 생성, .gitignore 필수)
```

> **빌드 산출물** (`*.exe`, `*.obj`, `*.res`)은 `cpp_doom\bin\`으로 분리하거나
> `.gitignore`에 추가하여 저장소에서 추적하지 않는 것을 강력 권장합니다.

---

## 4. .gitignore 점검 항목

현재 `.gitignore` (95 B)가 매우 작습니다. 아래 항목이 포함되어 있는지 확인해야 합니다:

```gitignore
# 빌드 산출물
*.exe
*.obj
*.res

# 런타임 설정 (보안)
*_mta.json
external/credentials.json
external/config.yaml

# 임시 파일
*.txt (단, 문서 제외)
RCa*
fav.txt
out.txt
build_output.txt

# 페이로드 데이터 (대용량)
cpp_doom/data/
```
