# main.cpp 코드 최적화 리팩토링 계획

현재 [main.cpp](file:///d:/missile_to_AWS/cpp_doom/main.cpp) 2,115줄을 **기능 100% 유지**하면서 약 **~1,750줄(약 17% 감소)**으로 줄이는 구조적 리팩토링 계획입니다.

## 변경 규칙
- 새로운 기능 추가 없음. 순수 구조 개선만.
- 각 Phase마다 빌드 성공 확인 후 커밋.
- [resource_list.h](file:///d:/missile_to_AWS/cpp_doom/resource_list.h), [resource.h](file:///d:/missile_to_AWS/cpp_doom/resource.h), [resources.rc](file:///d:/missile_to_AWS/cpp_doom/resources.rc)는 변경하지 않음.

---

## Proposed Changes

### Phase 1 — DrawStyledButton 헬퍼 함수 도입 (~290줄 감소)

#### [MODIFY] [main.cpp](file:///d:/missile_to_AWS/cpp_doom/main.cpp)

현재 WM_DRAWITEM 내의 각 커스텀 버튼(`ID_BTN_SETTINGS`, `ID_CHK_SELECT_ALL`, `ID_BTN_CANCEL`, `ID_BTN_ISSUES`, `ID_BTN_AWS_CONSOLE`, 리전 체크박스)이 모두 유사한 구조로 `FillRect`, `CreatePen`, `RoundRect`, `SelectObject`, `DeleteObject`, `SetTextColor`, `DrawText`를 반복하고 있다.

**추가할 헬퍼 함수:**

```cpp
// 스타일 정의
struct BtnStyle {
    COLORREF bg;       // 배경색
    COLORREF text;     // 텍스트색
    COLORREF border;   // 테두리색
    int cornerRadius;  // RoundRect 반경 (0=Rectangle)
    bool bold;
};

// 단일 헬퍼로 모든 커스텀 버튼 렌더링
static void DrawStyledButton(HDC hdc, const RECT& rc, 
                             const BtnStyle& style,
                             const wchar_t* topText,   // 아이콘/큰 텍스트
                             const wchar_t* bottomText, // 하단 소 텍스트 (NULL 가능)
                             HFONT hTopFont, HFONT hBotFont);
```

**영향 범위:** `WM_DRAWITEM` 케이스 내 5개 블록, 약 470줄 → 약 180줄

---

### Phase 2 — CreateFont 팩토리 함수 (~25줄 감소)

#### [MODIFY] [main.cpp](file:///d:/missile_to_AWS/cpp_doom/main.cpp)

현재 WM_CREATE와 SettingsDlgProc에 `CreateFont(크기, 0, 0, 0, 굵기, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Noto Sans KR")` 패턴이 7회 반복된다.

**추가할 인라인 헬퍼:**
```cpp
inline HFONT MakeFont(int size, int weight = FW_NORMAL, 
                      const wchar_t* face = L"Noto Sans KR") {
    return CreateFont(size, 0, 0, 0, weight, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, face);
}
```

**사용 예:**
```cpp
// Before (3줄 × 7회)
g_hFontBold = CreateFont(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ...);
// After (1줄 × 7회)
g_hFontBold = MakeFont(18, FW_BOLD);
```

---

### Phase 3 — SaveFiles JSON 쓰기 패턴 정리 (~20줄 감소)

#### [MODIFY] [main.cpp](file:///d:/missile_to_AWS/cpp_doom/main.cpp)

현재 [SaveFiles()](file:///d:/missile_to_AWS/cpp_doom/main.cpp#113-114)에서 3개 파일(`credentials.json`, `config.yaml`, [_mta.json](file:///d:/missile_to_AWS/cpp_doom/AWSCleaner_mta.json))을 각각 `std::wofstream`으로 열고 닫는 패턴이 3회 반복된다.

**개선:** 람다 `auto writeFile = [](path, content)` 또는 별도 `WriteUTF8File()` 헬퍼로 묶어 중복 제거.

---

### Phase 4 — 중복 include 제거 및 기타 정리 (~5줄 감소)

#### [MODIFY] [main.cpp](file:///d:/missile_to_AWS/cpp_doom/main.cpp)

- **라인 36**: `#include "resource.h"` 중복 선언 제거 (라인 20에 이미 포함)
- 빈 [if](file:///d:/missile_to_AWS/cpp_doom/main.cpp#549-578) 블록 정리 (WM_TIMER의 ComboBox 포커스 분기 등)
- `std::wstring` 연결 체인을 `wostringstream`으로 교체 (런타임 임시 객체 감소)

---

## 예상 결과

| 항목 | 현재 | 변경 후 |
|------|------|---------|
| [main.cpp](file:///d:/missile_to_AWS/cpp_doom/main.cpp) 총 줄 수 | 2,115줄 | ~1,750줄 |
| 줄 감소량 | — | **~365줄 (17%)** |
| 전체 소스 합계 | 2,624줄 | ~2,259줄 |
| 기능 변경 | — | **없음** |
| 빌드 경고 | C4312 1건 | 동일 또는 감소 |

---

## Verification Plan

### Automated Tests
- 각 Phase 후 [build.bat](file:///d:/missile_to_AWS/cpp_doom/build.bat) 실행, **Exit code 0** 확인
- 빌드 경고 수가 증가하지 않는지 확인

### Manual Verification
- NUKE 버튼 클릭 후 카운트다운 및 스피너 정상 동작 확인
- 설정 창 열기/닫기 크래시 없음 확인
- 리전 체크박스 선택 및 저장 정상 동작 확인
- 커스텀 버튼(SAVE, SELECT ALL, SETTINGS) 렌더링 외관 변화 없음 확인
