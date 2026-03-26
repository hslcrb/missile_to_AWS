# 📊 AWS Cleaner 소스코드 최적화 평가 보고서

---

## 1. 현재 소스코드 라인 수

| 파일 | 용도 | 라인 수 | 크기 |
|------|------|---------|------|
| [main.cpp](file:///d:/missile_to_AWS/cpp_doom/main.cpp) | 메인 로직 전체 | **2,115** | 96 KB |
| [resource_list.h](file:///d:/missile_to_AWS/cpp_doom/resource_list.h) | 400+ AWS 리소스 한글화 테이블 | **421** | 92 KB |
| [resource.h](file:///d:/missile_to_AWS/cpp_doom/resource.h) | 컨트롤 ID 정의 | **50** | 1.3 KB |
| [resources.rc](file:///d:/missile_to_AWS/cpp_doom/resources.rc) | 리소스 파일 (아이콘, 다이얼로그 등) | **38** | 1.5 KB |
| **합계** | | **2,624** | **~191 KB** |

> [resource_list.h](file:///d:/missile_to_AWS/cpp_doom/resource_list.h)의 92 KB는 순수 데이터(한글/영어 문자열)이므로 *로직 최적화 대상이 아님.*  
> **실질적 최적화 대상: [main.cpp](file:///d:/missile_to_AWS/cpp_doom/main.cpp) 2,115줄**

---

## 2. [main.cpp](file:///d:/missile_to_AWS/cpp_doom/main.cpp) 구역별 현황 분석

| 구역 | 대략적 줄 수 | 특이사항 |
|------|-------------|---------|
| 전역 변수 및 자료구조 | ~120줄 | 분산 선언, 일부 중복 |
| 바이너리 로딩/무결성 (SHA-256) | ~120줄 | 비교적 효율적 |
| 한글 초성 검색 + 필터 로직 | ~110줄 | 효율적 |
| JSON 설정 파일 쓰기 ([SaveFiles](file:///d:/missile_to_AWS/cpp_doom/main.cpp#934-992)) | ~80줄 | 중복 `wofstream` 패턴 |
| MTA 설정 읽기 ([LoadMTAConfig](file:///d:/missile_to_AWS/cpp_doom/main.cpp#360-474)) | ~90줄 | 개선 가능 |
| DPAPI 암호화 | ~80줄 | 비교적 효율적 |
| ProcessHollow + ReadPipe | ~90줄 | 비교적 효율적 |
| **WM_DRAWITEM (버튼 그리기)** | **~470줄** | ⚠️ **최대 중복 구역** |
| WM_CREATE (컨트롤 생성) | ~140줄 | 개선 가능 |
| WM_TIMER (타이머 핸들러) | ~140줄 | 분리 가능 |
| [SettingsDlgProc](file:///d:/missile_to_AWS/cpp_doom/main.cpp#1126-1235) (설정 다이얼로그) | ~120줄 | 비교적 효율적 |
| WndProc 기타 메시지 처리 | ~350줄 | 분리 가능 |

---

## 3. 발견된 주요 중복 및 최적화 포인트

### 🔴 [HIGH] WM_DRAWITEM 버튼 그리기 블록 반복 (~470줄 → ~180줄 가능)

현재 각 커스텀 버튼(`ID_BTN_NUKE`, `ID_BTN_SAVE`, `ID_BTN_SETTINGS`, `ID_BTN_CANCEL`, `ID_CHK_SELECT_ALL`, 리전 체크박스)마다 **독립된 RoundRect + FillRect + SelectObject + DrawText 패턴이 5~6회 반복**됩니다.

```cpp
// 현재: 버튼마다 30~80줄 중복
FillRect(hdc, &rc, hBrush);
HPEN hPen = CreatePen(...); SelectObject(hdc, hPen);
RoundRect(...); DeleteObject(hPen); ...
DrawText(hdc, L"텍스트", ...);
```

**개선안:** `DrawStyledButton(HDC, RECT, Color, Text, Style)` 헬퍼 함수 1개로 통합 → **~290줄 제거 가능**

---

### 🔴 [HIGH] `CreateFont` 호출 중복 (7회 → 1개 팩토리로 통합)

현재 코드 전체에 `CreateFont(크기, 0, 0, 0, 굵기, FALSE, FALSE, FALSE, DEFAULT_CHARSET, ...)` 형태가 **7회** 등장합니다. 24개 파라미터를 매번 나열합니다.

```cpp
// 현재: 7회 반복
g_hFontBold    = CreateFont(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, ...);
g_hFontPrefix  = CreateFont(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, ...);
g_hFontIndicator = CreateFont(16, 0, 0, 0, FW_BOLD, FALSE, ...);
```

**개선안:** `MakeFont(size, weight)` 인라인 헬퍼 → **~25줄 제거 및 가독성 향상**

---

### 🟡 [MED] [SaveFiles()](file:///d:/missile_to_AWS/cpp_doom/main.cpp#934-992) JSON 기록 중복 패턴

[SaveFiles()](file:///d:/missile_to_AWS/cpp_doom/main.cpp#934-992)에서 `credentials.json`, `config.yaml`, [_mta.json](file:///d:/missile_to_AWS/cpp_doom/AWSCleaner_mta.json) 세 파일을 각각 `wofstream`으로 열고 동일한 패턴으로 씁니다. `wofstream` 열기/닫기 코드가 3회 반복됩니다. → **~20줄 제거 가능**

---

### 🟡 [MED] 단일 거대 `WndProc` 분리

현재 [WindowProc](file:///d:/missile_to_AWS/cpp_doom/main.cpp#1236-1997) 함수가 약 **1,600줄**에 달하는 단일 함수입니다. `WM_DRAWITEM`, `WM_COMMAND`, `WM_TIMER`, `WM_CREATE` 각각을 별도 핸들러 함수로 분리하면:
- 가독성: 대폭 향상
- 라인 자체 감소: 없음 (로직 동일)

---

### 🟢 [LOW] 이중 `#include "resource.h"` (라인 20, 36)

[main.cpp](file:///d:/missile_to_AWS/cpp_doom/main.cpp) 내에 [resource.h](file:///d:/missile_to_AWS/cpp_doom/resource.h)가 **두 번 포함**되어 있습니다. 헤더 가드가 있어 컴파일 오류는 없지만, 36번째 줄은 불필요합니다. → **1줄 제거**

---

### 🟢 [LOW] `std::wstring` 연결 대신 `wostringstream` 활용

로그 조합 등 여러 곳에서 `L"..." + std::to_wstring(n) + L"..."` 패턴이 반복됩니다. `std::wostringstream`으로 통합하면 중간 임시 객체 생성이 줄어 **런타임 효율 소폭 향상**됩니다.

---

## 4. 최적화 후 예상 라인 수

| 항목 | 제거 가능 줄 수 |
|------|----------------|
| WM_DRAWITEM 버튼 통합 | ~290줄 |
| CreateFont 팩토리화 | ~25줄 |
| SaveFiles 중복 정리 | ~20줄 |
| 중복 include 제거 | ~1줄 |
| 기타 소소한 정리 | ~30줄 |
| **합계** | **~366줄 (약 17% 감소)** |

| 상태 | [main.cpp](file:///d:/missile_to_AWS/cpp_doom/main.cpp) 줄 수 |
|------|----------------|
| 현재 | 2,115줄 |
| 최적화 후 예상 | **~1,750줄** |

> **기능 100% 동일**, 신규 버그 위험 낮음 (순수 구조적 리팩토링)

---

## 5. 결론

- **즉시 적용 가능하고 효과가 큰 것:** `WM_DRAWITEM` 버튼 그리기 헬퍼화, `CreateFont` 팩토리화
- **선택적 적용:** 파일 저장 패턴 정리, WndProc 분리
- **런타임 성능 이점:** 현재 구조도 충분히 최적화되어 있어 체감 속도 차이는 없으나, **코드 유지보수성과 가독성이 크게 향상**됩니다.

원하신다면 `DrawStyledButton` 헬퍼 함수를 구현하여 WM_DRAWITEM 약 290줄 감소부터 바로 시작할 수 있습니다.
