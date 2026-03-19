# AWS Cleaner 보안 및 안정성 패치 결과 리포트

`allbugfix` 브랜치에 적용된 주요 보안 강화 및 프로그램 안정성 개선 사항을 요약합니다.

## 🛠️ 주요 수정 사항

### 1. 보안강화 (자격 증명 암호화)
- **DPAPI 적용**: [AWSCleaner_mta.json](file:///d:/missile_to_AWS/cpp_doom/AWSCleaner_mta.json) 파일에 저장되던 AWS Access/Secret Key를 Windows 데이터 보호 API(DPAPI)를 사용하여 사용자 계정별로 암호화했습니다.
- **평문 노출 방지**: 파일 시스템에 저장될 때 더 이상 평문으로 남지 않으며, 메모리 내에서도 필요한 순간에만 복호화되어 사용됩니다.

### 2. 치명적 크래시 해결 (설정 창 종료 관련)
- **데이터 구조 불일치 수정**: [SaveFilesAsync](file:///d:/missile_to_AWS/cpp_doom/main.cpp#963-974) 스레드 함수가 `HWND`를 인자로 받으면서 내부에서는 `MTAConfigData*`로 간주하고 `delete`를 호출하던 타입 불일치 버그를 해결했습니다.
- **비동기 메시지 처리**: 대화상자 종료 시 `SendMessage` 대신 `PostMessage`를 사용하여 대화상자 매니저가 유효하지 않은 핸들에 접근하지 않도록 안전하게 처리했습니다.

### 3. 동시성 및 스레드 안정성
- **UI 스레드 분리**: 백그라운드 스레드에서 `GetWindowText` 등 UI 컨트롤에 직접 접근하던 위험한 코드를 제거했습니다. 대신 [MTAConfigData](file:///d:/missile_to_AWS/cpp_doom/main.cpp#90-99) 구조체에 필요한 데이터를 모두 복사하여 전달하는 방식을 채택했습니다.
- **핸들 누수 방지**: 스레드 생성 후 방치되던 핸들들에 대해 `CloseHandle()`을 즉시 호출하여 시스템 리소스 고갈을 방지했습니다.

### 4. 기타 버그 및 최적화
- **GDI 리소스 정리**: 프로그램 종료 시 폰트와 브러시를 `DeleteObject`로 반환하여 GDI 객체 누수를 해결했습니다.
- **JSON 이스케이프**: 설정 저장 시 특수문자(`"`, `\`)가 포함될 경우 JSON 형식이 깨지는 문제를 방지하기 위해 [EscapeJSON](file:///d:/missile_to_AWS/cpp_doom/main.cpp#100-101) 루틴을 추가했습니다.

## ✅ 검증 결과
- **로컬 빌드**: [build.bat](file:///d:/missile_to_AWS/cpp_doom/build.bat)을 통한 x64 릴리스 빌드 성공.
- **UI 자동화 테스트**: Python 스크립트를 통해 설정 변경 및 저장 시 발생하던 액세스 위반(Access Violation) 크래시가 더 이상 발생하지 않음을 확인했습니다.

## 🔗 Pull Request 정보
1.  **allbugfix ➔ next-gamma**: [https://github.com/hslcrb/missile_to_AWS/pull/2](https://github.com/hslcrb/missile_to_AWS/pull/2) (MERGED)
2.  **next-gamma ➔ main**: [https://github.com/hslcrb/missile_to_AWS/pull/3](https://github.com/hslcrb/missile_to_AWS/pull/3)

---
모든 치명적 버그와 보안 취약점이 `main` 브랜치까지 전파될 준비가 완료되었습니다.
