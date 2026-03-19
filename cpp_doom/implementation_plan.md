# Settings Update Plan

## 변경 사항 개요
1. **한글 깨짐 현상 수정**: `rc.exe`가 UTF-8로 작성된 [resources.rc](file:///c:/Users/Administrator/Documents/missile_to_AWS/cpp_doom/resources.rc) 내의 한글을 올바르게 컴파일할 수 있도록 코드 페이지 지시어를 추가합니다.
2. **설정파일 생성 버튼 추가**: 다이얼로그 디자인을 변경하여 '설정파일 생성' 버튼을 하단에 추가합니다. 파일들이 모두 존재할 경우 이 버튼은 비활성화(흐리게) 처리되며, 누락된 파일이 감지될 때만 활성화됩니다.
3. **파일 상태 안내 메시지 변경**: 누락된 파일이 있을 경우 상황을 알리고, "자동생성 하시겠습니까? 없어도 실행 시 자동으로 생성됩니다." 라는 안내 문구를 추가합니다.

## 세부 변경 내용
- **resource.h**: `IDC_BTN_CREATE_CONFIG` (예: 2021) 추가.
- **resources.rc**:
  - 최상단에 `#pragma code_page(65001)` 추가하여 UTF-8 인코딩 명시.
  - `IDD_SETTINGS_DIALOG` 하단 영역 넓이를 살짝 조정하거나 여백에 `PUSHBUTTON` 추가.
- **main.cpp**:
  - `WM_INITDIALOG`: `credentials.json`, `config.yaml`, [_mta.json](file:///c:/Users/Administrator/Documents/missile_to_AWS/cpp_doom/AWSCleaner_mta.json) 파일의 누락 개수를 카운트. 
  - 상태 문자열(`statusText`) 최하단에 자동 생성 안내 문구 결합.
  - 누락 파일 갯수가 1개 이상이면 `EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_CREATE_CONFIG), TRUE)`, 0개이면 `FALSE` 적용.
  - `WM_COMMAND`에서 `IDC_BTN_CREATE_CONFIG` 클릭 시, [SaveFiles(GetParent(hwndDlg))](file:///c:/Users/Administrator/Documents/missile_to_AWS/cpp_doom/main.cpp#722-811) 등을 간접 활용하거나 빈 기본 파일을 생성하는 보조 함수 호출 후 텍스트/상태 갱신.
