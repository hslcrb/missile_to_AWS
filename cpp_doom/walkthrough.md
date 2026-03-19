# Settings Popup Implementation Walkthrough

## 작업 개요
사용자의 요청에 따라 `전체 선택` 버튼 옆에 동일한 스타일의 검은색 `설정` 버튼을 추가하였고, 이 버튼을 통해 열리는 **설정 팝업(Dialog)**을 구현하였습니다.

## 주요 변경 사항
1. **버튼 UI 추가** ([main.cpp](file:///c:/Users/Administrator/Documents/missile_to_AWS/cpp_doom/main.cpp)): 
   - `WM_CREATE`에서 `설정` 버튼 크기와 위치 지정 (`x=495, y=groupY+30`).
   - `WM_DRAWITEM`을 오버라이딩하여, 사용자가 요청한 것과 동일한 둥근 모양(`RoundRect`)으로 검은색(#1e1e1e) 배경과 원형 아이콘(설정 표시)을 Owner-Draw로 렌더링하도록 구현했습니다.

2. **설정 다이얼로그 프로시저 구현 ([SettingsDlgProc](file:///c:/Users/Administrator/Documents/missile_to_AWS/cpp_doom/main.cpp#915-988))**
   - **리스트뷰 컨트롤 (SysListView32)** 추가: 영어 이름, 한글 이름, 태그 정보 상제 목록 구성 완료 (`LVS_EX_CHECKBOXES` 사용).
   - **설정 파일 상태 조회 기능 및 생성 로직**: `external/credentials.json`, `config.yaml`, [_mta.json](file:///c:/Users/Administrator/Documents/missile_to_AWS/cpp_doom/AWSCleaner_mta.json) 파일 존재 여부 검사.
   - 누락된 파일이 감지될 시, "자동생성 하시겠습니까? 없어도 실행 시 자동으로 생성됩니다." 텍스트 메시지를 추가로 출력합니다.
   - **설정파일 생성 버튼 연동**: 평소에는 비활성화(`WS_DISABLED`) 되어 있으나, 누락된 파일이 있으면 활성화되어 버튼 클릭 한 번으로 기본 설정 파일들을 디스크에 자동 생성해줍니다.

3. **리소스 파일 업데이트 ([resource.h](file:///c:/Users/Administrator/Documents/missile_to_AWS/cpp_doom/resource.h), [resources.rc](file:///c:/Users/Administrator/Documents/missile_to_AWS/cpp_doom/resources.rc))**
   - RC 파일 내에서 한글 깨짐 현상을 해결하기 위해 최상단에 `#pragma code_page(65001)` 선언(UTF-8)을 명시했습니다.
   - `IDD_SETTINGS_DIALOG` 하단에 설정파일 생성(`IDC_BTN_CREATE_CONFIG`) 버튼을 추가하였습니다.

## 테스트 및 빌드 과정 (Validate)
수정된 코드 및 리소스들을 바탕으로 [build.bat](file:///c:/Users/Administrator/Documents/missile_to_AWS/cpp_doom/build.bat)을 재실행한 결과, MSVC 빌드가 **에러 및 경고 없이 성공(Exit Code 0)**하여 [AWSCleaner.exe](file:///c:/Users/Administrator/Documents/missile_to_AWS/cpp_doom/AWSCleaner.exe) 파일이 정상적으로 갱신되었습니다.
이제 새롭게 빌드된 실행 파일을 통해 '설정' 버튼의 시각적 요소와 다이얼로그의 동작을 직접 사용하며 테스트해 주시면 됩니다.
