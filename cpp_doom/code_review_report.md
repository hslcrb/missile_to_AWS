# 🚨 AWS Cleaner 코드 리뷰 및 개선 보고서

AWS Cleaner 프로젝트([main.cpp](file:///d:/missile_to_AWS/cpp_doom/main.cpp), [resource.h](file:///d:/missile_to_AWS/cpp_doom/resource.h) 등)의 코드를 종합적으로 분석하여, **보안 취약점, 메모리/리소스 누수, 동시성 오류(안정성), 성능 개선 요소**를 도출했습니다.

---

## 🔒 1. 보안 (Security Vulnerabilities)

### ⚠️ [CRITICAL] AWS 자격 증명 평문 저장
- **이슈:** [SaveFiles](file:///d:/missile_to_AWS/cpp_doom/main.cpp#803-911) 함수에서 Account ID, Access Key, Secret Key를 [AWSCleaner_mta.json](file:///d:/missile_to_AWS/cpp_doom/AWSCleaner_mta.json) 파일에 평문(Plain-text)으로 저장하고 있습니다. 로컬 PC가 침해당하면 AWS 자격 증명이 그대로 노출됩니다.
- **해결책:** Windows Data Protection API (DPAPI)의 `CryptProtectData`를 사용하여 Secret Key 등을 암호화하여 저장하고, 로드 시 `CryptUnprotectData`로 복호화해야 합니다.

---

## 💥 2. 동시성 및 스레드 안정성 (Concurrency / Thread Safety)

### ⚠️ [HIGH] 백그라운드 스레드에서 UI 자원 접근
- **이슈:** [SaveFilesAsync](file:///d:/missile_to_AWS/cpp_doom/main.cpp#912-923)는 `_beginthreadex`를 통해 백그라운드 스레드에서 실행됩니다. 하지만 [SaveFiles](file:///d:/missile_to_AWS/cpp_doom/main.cpp#803-911) 함수 내부에서 `GetWindowText`를 호출해 UI 컨트롤(`g_hAccount`, `g_hAccessKey` 등)의 텍스트를 읽고 있습니다. Win32의 UI 오프너 스레드 규칙 위반이며 데드락(Deadlock)이나 충돌을 유발할 수 있습니다.
- **해결책:** UI 스레드에서 저장할 데이터를 구조체(struct)에 모두 담은 뒤, 이 구조체를 스레드의 매개변수로 넘겨주어 백그라운드에서는 전역 변수나 UI 핸들에 직접 접근하지 않도록 수정해야 합니다.

---

## 💧 3. 자원 및 메모리 누수 (Resource & Memory Leaks)

### ⚠️ [MEDIUM] 스레드 핸들 누수 (Thread Handle Leak)
- **이슈:** 자동 저장 시 [SaveFilesAsync](file:///d:/missile_to_AWS/cpp_doom/main.cpp#912-923)가 호출되어 `_beginthreadex`로 스레드를 생성하지만, 반환된 스레드 핸들(`HANDLE`)에 대해 `CloseHandle()`을 호출하지 않습니다. 타이머에 의해 자주 저장이 발생하므로 장기 실행 시 심각한 핸들 누수가 발생합니다.
- **해결책:** 스레드 생성 직후 `CloseHandle(hThread);`를 호출하여 정리해야 합니다.

### ⚠️ [LOW] GDI 객체 초기화 해제 누락
- **이슈:** `WM_CREATE`에서 `CreateFont`, `CreateSolidBrush` 등으로 폰트와 브러시(예: `g_hBrushNavy`, `g_hBrushPastelYellow` 등)를 다수 생성하지만, `WM_DESTROY`에서 `DeleteObject`로 해제하지 않습니다. 프로세스 종료 시 OS가 회수하긴 하지만 좋은 C++ 리소스 관리 관행에 어긋납니다.
- **해결책:** `WM_DESTROY` 메시지 핸들러에 모든 전역 `HFONT`, `HBRUSH`, `HPEN`에 대한 `DeleteObject` 호출을 추가해야 합니다.

---

## 🐛 4. 로직 및 파싱 안정성 (Logic & Edge Cases)

### ⚠️ [MEDIUM] 수동 JSON 문자열 결합에 의한 파싱 깨짐 가능성
- **이슈:** [SaveFiles](file:///d:/missile_to_AWS/cpp_doom/main.cpp#803-911)에서 JSON 텍스트를 저장할 때 `std::wstring` 문자열 덧셈으로 수동 구성하고 있습니다. 필터 텍스트나 로그 내용에 큰따옴표(`"`)나 역슬래시(`\`) 같은 특수 문자가 입력될 경우 JSON 형식 에러가 발생하여 다음 실행 시 설정 파일이 로드되지 않습니다.
- **해결책:** 안전을 위해 C++용 현대적인 JSON 라이브러리(예: `nlohmann/json` 등)를 사용하여 직렬화하는 것을 강력히 권장합니다.

---

## 🚀 5. 성능 및 UI 렌더링 (Performance & Rendering)

### 💡 [INFO] 잦은 InvalidateRect와 깜빡임
- **이슈:** 여러 UI 이벤트 변경 시 최상위 부모 윈도우에서 넓은 영역을 `InvalidateRect(hwndMain, NULL, TRUE)`로 불필요하게 다시 그리고 있습니다. 창이 깜빡이거나 렌더링 자원 낭비가 발생할 수 있습니다.
- **해결책:** 변경된 컨트롤(예: 드롭다운이나 버튼) 영역만 한정해서 다시 그리도록 조치하고, 가능하면 더블 버퍼링(Double Buffering)을 적용해 깜빡임을 완전히 방지하는 것이 좋습니다.

---

### 📌 조치 요약 (Action Item)
1. **스레드 핸들 `CloseHandle` 즉시 추가 (필수)**
2. **`DPAPI` 적용하여 자격증명 암호화 (필수)**
3. **[SaveFiles](file:///d:/missile_to_AWS/cpp_doom/main.cpp#803-911) 구조 개선: UI 읽기 분리 (안정성)**
4. **`WM_DESTROY` GDI 클린업 추가 (권장)**
