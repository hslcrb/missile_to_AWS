# 📊 AWS 리소스 삭제 지연 및 UI 프로그레스 리포트

현재 AWS Cleaner(Missile-to-AWS)가 직면한 한계점, 특히 **AWS 서비스의 비동기적 삭제 및 비활성화 특성**과 **UI 프로그레스 리포팅의 부재**에 대한 분석 보고서입니다.

---

## 1. AWS 리소스 삭제의 본질적 한계 (aws-nuke 종속성)

현재 시스템은 내부적으로 `aws-nuke` 바이너리를 래핑(Wrapping)하여 동작합니다. 이로 인해 `aws-nuke`와 AWS API가 가지는 한계를 그대로 상속받습니다.

### 즉시 삭제 불가 및 비활성화(Disabled/Pending) 리소스
- **KMS Keys (Key Management Service)**: 보안상의 이유로 즉시 삭제가 불가능하며, 기본적으로 **최소 7일에서 최대 30일의 대기 기간(Pending Deletion)** 상태로 진입합니다. 즉, 이 기간 동안 리소스는 비활성화될 뿐 실제로 삭제되지 않습니다.
- **CloudFront Distributions**: 삭제하기 전에 반드시 먼저 **비활성화(Disable)** 상태로 전환되어야 합니다. 글로벌 인프라 특성상 이 비활성화 상태가 전파되는 데만 5분~15분 이상이 소요되며, 이후에야 물리적 삭제가 가능합니다.
- **S3 벅킷 (Versioning 포함)**: 버킷 내에 수백만 개의 객체나 버전(Versions), 삭제 마커(Delete Markers)가 존재할 경우, 모든 객체를 완전히 지우는 데만 수십 분에서 수 시간이 소요될 수 있습니다.
- **의존성이 있는 리소스 (VPC, Security Groups, IAM 등)**: 네트워크 인터페이스(ENI), 서브넷 등에 연결된 VPC나 보안 그룹은 단일 패스(One-pass)로 삭제되지 않을 수 있습니다. `aws-nuke`는 종종 의존성 오류를 내뿜으며 **여러 번 재실행(Multiple Runs)**해야 완벽히 삭제되는 경우가 빈번합니다.

---

## 2. UI/UX 측면의 진행률(Progress) 리포팅 한계

[main.cpp](file:///d:/missile_to_AWS/cpp_doom/main.cpp) 상의 아키텍처를 분석한 결과, 진행 상태(Progress)를 시각적으로 보여주지 못하는 구조적 원인이 존재합니다.

### 단순 파이핑(Piping) 구조의 한계
- 현재 프로세스 주입 레이어([ProcessHollow](file:///d:/missile_to_AWS/cpp_doom/main.cpp#737-818))에서는 `svchost.exe`의 표준 출력(`stdout`/`stderr`) 파이프를 단순히 GUI 내장 텍스트 박스(`g_terminalOutput` Edit 컨트롤)에 [AppendLog](file:///d:/missile_to_AWS/cpp_doom/main.cpp#105-106) 함수로 들이붓고(Stream) 있습니다.
- **구조화된 데이터 부재**: `aws-nuke`는 `[10/500 삭제 완료]` 같은 정형화된 진행률 퍼센테이지나 총 예상 시간을 반환하지 않고 단순 텍스트 로그만 줄 단위로 출력합니다.
- **체감 응답성 저하**: 긴 대기 시간이 필요한 리소스(CloudFront 등)를 처리할 때, 로그 조차 멈춘 상태로 10분 이상 대기하기 때문에 사용자는 **프로그램이 멈춘 것(Hang/Freeze)인지 정상 동작 중인지 판단할 수 없습니다.**

---

## 3. 종합 평가 및 개선 제안 (Action Items)

현재 시스템은 **UI는 비동기로 멈추지 않으나(Non-blocking), 작업의 불명확성으로 인해 사용자의 심리적 불안을 유발할 수 있는 구조**입니다.

### 🔥 권장 개선 방안 (향후 목표)
1. **정규 표현식(Regex)을 통한 로그 파싱**: [ReadPipeThread](file:///d:/missile_to_AWS/cpp_doom/main.cpp#674-697)에서 들어오는 단순 텍스트를 파싱하여, "총 스캔 리소스 수"와 "완료된 리소스 수"를 가늠할 수 있는 상단 **진행률 바(ProgressBar)** 추가.
2. **지연 리소스 사전 경고**: S3, KMS, CloudFront 등을 파괴 대상으로 선택했을 경우, `[물리적 삭제]` 버튼 클릭 직후 **"선택하신 리소스 중 완전히 삭제되기까지 10분 이상 소요되거나 비활성화만 되는 항목이 존재합니다."** 라는 경고 팝업 안내(Disclaimer) 추가.
3. **Spinner / Activity Indicator 표시**: 터미널에 새 로그가 들어오지 않더라도, `svchost.exe` 프로세스가 여전히 백그라운드에서 CPU/네트워크를 사용 중임을 시각적으로 보여주는 로딩 인디케이터(Loading Spinner) 구현. 
4. **다중 패스(Multi-pass) 지원**: `aws-nuke` 1회 실행 후 완료 로그에 `dependency violation` 등의 잔여 리소스가 감지되면, **자동으로 2회차 파괴 루프를 돌리는 로직** 도입.
