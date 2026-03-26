# 디렉토리 구조 재배치 계획

전수실태 보고서 기반으로 `d:\missile_to_AWS` 프로젝트의 파일/폴더를 체계적으로 재배치하는 계획입니다.

## User Review Required

> [!WARNING]
> 이 계획에는 파일 **삭제** 작업이 포함되어 있습니다. 실행 전 반드시 확인해 주세요.  
> 릴리즈 ZIP 파일은 이동 전 원본 위치를 Git에서 제거하고 `releases/`에 재추가합니다.

---

## Proposed Changes

### Phase 1 — 임시/불필요 파일 삭제

#### [DELETE] `d:\missile_to_AWS\ESDomain`
- 내용 없는 정체불명 항목.

#### [DELETE] `d:\missile_to_AWS\cpp_doom\RCa11096`
- RC 컴파일러 임시 잔재 파일.

#### [DELETE] `d:\missile_to_AWS\cpp_doom\fav.txt`
- 개발 중 생성된 임시 즐겨찾기 파일. 코드는 `g_favorites` 구조체에서 관리됨.

#### [DELETE] `d:\missile_to_AWS\cpp_doom\out.txt`
- 임시 출력 파일.

#### [DELETE] `d:\missile_to_AWS\cpp_doom\build_output.txt`
- 빌드 로그 임시 파일.

#### [DELETE] `d:\missile_to_AWS\cpp_doom\20260319thu_Gamma-Realease - 복사본.zip`
- 원본 `20260319thu_Gamma-Realease.zip`과 완전히 동일한 중복 파일 (32MB).

---

### Phase 2 — 개발 도구 스크립트 격리

#### [NEW] `d:\missile_to_AWS\cpp_doom\tools\` 폴더 생성

#### [MODIFY] 스크립트 3개를 `tools\`로 이동
- `gen_cpp.py` → `cpp_doom\tools\gen_cpp.py`
- `gen_hashes.py` → `cpp_doom\tools\gen_hashes.py`
- `gen_tags.py` → `cpp_doom\tools\gen_tags.py`

---

### Phase 3 — 릴리즈 아카이브 격리

#### [NEW] `d:\missile_to_AWS\releases\` 폴더 생성

#### [MODIFY] ZIP 파일 2개를 `releases\`로 이동
- `20260318wed_Alpha-Realease.zip` → `releases\Alpha-20260318.zip`
- `20260319thu_Gamma-Realease.zip` → `releases\Gamma-20260319.zip`

---

### Phase 4 — `.gitignore` 강화

#### [MODIFY] [.gitignore](file:///d:/missile_to_AWS/.gitignore)

현재 95B의 최소 `.gitignore`에 아래 항목들을 추가:

```gitignore
# 빌드 산출물
cpp_doom/*.exe
cpp_doom/*.obj
cpp_doom/*.res

# 런타임 설정 (보안 민감)
cpp_doom/AWSCleaner_mta.json
cpp_doom/external/

# 임시 파일
cpp_doom/fav.txt
cpp_doom/out.txt
cpp_doom/build_output.txt
cpp_doom/RCa*

# 대용량 페이로드 (Git LFS 또는 제외 권장)
# cpp_doom/data/
```

> [!IMPORTANT]
> `external/credentials.json`은 평문 AWS Access Key를 포함합니다.  
> Git 이력에 이미 커밋된 경우 `git filter-branch` 또는 BFG로 이력 정리 필요.

---

## 최종 목표 구조

```
d:\missile_to_AWS\
├── .gitignore         (강화됨)
├── LICENSE
├── README.md
├── CHANGELOG.md
├── CONTRIBUTING.md
├── USAGE.md
├── binary_loading_logic.md
│
├── releases\          [신규]
│   ├── Alpha-20260318.zip
│   └── Gamma-20260319.zip
│
└── cpp_doom\
    ├── main.cpp
    ├── resource.h
    ├── resource_list.h
    ├── resources.rc
    ├── build.bat
    ├── tools\         [신규]
    │   ├── gen_cpp.py
    │   ├── gen_hashes.py
    │   └── gen_tags.py
    ├── assets\
    ├── data\
    └── external\      (런타임 생성, .gitignored)
```

---

## Verification Plan

### Automated Tests
- Phase 1~3 완료 후 `build.bat` 실행 → Exit code 0 확인
- `gen_*.py` 경로가 바뀌었으므로 스크립트 내 상대 경로 참조 여부 확인

### Manual Verification
- `resources.rc`의 `assets/` 경로 참조가 변경 없이 정상인지 확인
- `.gitignore`에 `external/`이 추가된 후 `git status`로 추적 제외 여부 확인
- `git log --oneline -5`로 커밋 연속성 확인
