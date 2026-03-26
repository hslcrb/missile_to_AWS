# 📊 aws-nuke Go 소스코드 → C++ 포팅 가능성 분석 보고서

---

## 요약

> **Go 소스를 참고용으로 제공할 경우: 이전 접근법보다 훨씬 현실적이고, 2~4개월 수준으로 공수가 줄어든다.**  
> 단, Go-C++ 언어 철학의 차이로 인해 "1:1 번역"이 아닌 "의미적 재구현"이 요구된다.

---

## 1. Go 소스코드를 가지면 무엇이 달라지는가?

이전 보고서에서 가장 큰 장벽은 **"400종 리소스의 API 호출 스펙을 처음부터 알아야 한다"** 는 것이었다.

Go 소스를 갖게 되면:

| 이전 (블랙박스) | Go 소스 참고 시 |
|----------------|-----------------|
| 어떤 API 엔드포인트를 쓰는지 모름 | ✅ 각 리소스의 List/Delete API가 명확히 보임 |
| 삭제 순서/조건 로직 불명 | ✅ 필터링, 의존성 처리 로직이 코드로 드러남 |
| 페이지네이션 방식 모름 | ✅ NextToken, Marker 처리 패턴이 코드에 있음 |
| 재시도 로직 불명 | ✅ 멀티패스 조건과 backoff 로직이 명시됨 |

즉, **"무엇을 만들어야 하는가"의 90%가 해결**된다. 남는 것은 "어떻게 Go 로직을 C++로 옮기는가"이다.

---

## 2. Go와 C++ 간 핵심 언어 차이 및 대응 전략

### ① Goroutine(고루틴) → C++ `std::thread` + `std::async`
aws-nuke는 여러 리전/리소스를 **고루틴으로 병렬 처리**한다.

```go
// Go
go func() { deleteEC2Instances(region) }()
```
```cpp
// C++ 대응
std::async(std::launch::async, deleteEC2Instances, region);
```
- ✅ 구조적으로 변환 가능하나, Go의 채널(channel) 통신 패턴을 C++의 `std::queue` + `std::mutex`로 재구현해야 한다.
- ⚠️ Goroutine은 수천 개가 동시에 가동 가능하지만 C++ 스레드는 수십~수백 개가 현실적 한계. **스레드풀(ThreadPool)** 구현 필요.

### ② Go AWS SDK (`aws-sdk-go-v2`) → WinHTTP + SigV4 직접 구현
Go는 공식 AWS SDK가 있어 인증/호출이 추상화되어 있다. C++에는 공식 SDK가 있으나 (`aws-sdk-cpp`) 의존성이 매우 크다.

**선택지:**
| 방법 | 장점 | 단점 |
|------|------|------|
| `aws-sdk-cpp` 사용 | 완전한 AWS API 지원 | 수백 MB 이상의 거대한 빌드 의존성 |
| WinHTTP + SigV4 직접 구현 | 경량, 외부 의존성 0 | SigV4 서명 로직 직접 구현 필요 (~2주) |

→ 현재 프로젝트의 **"외부 의존성 최소화" 철학**에 맞춰 WinHTTP 직접 구현 권장.

### ③ Go `interface{}` 다형성 → C++ `virtual` / `std::variant`
aws-nuke는 모든 리소스를 하나의 인터페이스로 추상화한다:

```go
// Go
type Resource interface {
    Remove(session) error
    String() string
    Filter() error
}
```
```cpp
// C++ 대응
class AWSResource {
public:
    virtual std::wstring Name() const = 0;
    virtual bool Remove(const AWSSession&) = 0;
    virtual bool Filter(const FilterConfig&) = 0;
};
```
- ✅ C++의 순수 가상 함수(추상 클래스)로 **거의 1:1 대응** 가능.

### ④ Go `error` 반환 → C++ 예외 or `std::expected`
Go의 멀티 반환값 에러 처리는 C++ `std::pair<Result, Error>` 또는 예외로 대응.

### ⑤ JSON 처리 (`encoding/json`) → `nlohmann/json.hpp`
Go는 JSON이 표준 라이브러리에 있다. C++은 없으므로 헤더 전용 `nlohmann/json`(단일 `.hpp` 파일) 사용 강력 권장.

---

## 3. 포팅 작업 구조 및 공수 재추정

Go 소스를 참고 가능할 경우 다음 순서로 진행:

### Phase 1: 코어 엔진 (3-4주)
- SigV4 서명 엔진 구현 (WinHTTP 기반)
- JSON 파싱 통합 (nlohmann/json)
- AWSResource 추상 클래스 설계
- 스레드풀 구현

### Phase 2: 핵심 리소스 핸들러 (4-6주)
Go 소스를 직접 참고하여 자주 쓰이는 20~30종 구현:
- EC2 (Instances, VPC, Security Groups, Keypairs)
- S3 (Buckets, Objects)
- IAM (Users, Roles, Policies)
- RDS (DB Instances, Snapshots)
- Lambda Functions
- CloudFormation Stacks

### Phase 3: 전체 400종 포팅 (2~6개월)
- Go 소스의 각 resource 파일을 1:1로 C++ 클래스로 번역
- 테스트 및 검증

| Phase | 공수 (단독 개발 기준) |
|-------|-----------------------|
| Phase 1 | 3~4주 |
| Phase 2 | 4~6주 |
| Phase 3 | 2~6개월 |
| **총합** | **약 3~9개월** |

*이전 보고서(6~18개월) 대비 약 절반으로 단축.*

---

## 4. 리스크 및 주의사항

| 리스크 | 설명 |
|--------|------|
| **Go 코드 라이선스** | aws-nuke는 MIT 라이선스이므로 소스 참고/포팅은 법적으로 허용됨. |
| **API 호환성 유지** | AWS API는 계속 업데이트됨. Go 소스 버전에 종속될 수 있음. |
| **로직 오해 위험** | Go 특유의 패턴(defer, context.WithCancel 등)을 잘못 해석할 수 있음. |
| **드라이런(Dry-run) 필수** | 포팅 완료 전 실제 삭제 API를 절대 호출하지 말 것. |

---

## 5. 결론 및 권장사항

```
[권장 단계]
1단계 (지금): Go 소스 제공 → Phase 1 (코어 엔진 PoC) 구현
2단계: EC2, S3, IAM 중 자주 쓰는 리소스 20종 C++ 포팅
3단계: aws-nuke 비중을 점진적으로 줄이며 완전 대체
```

**Go 소스를 제공하는 방식은 이전 접근법보다 훨씬 실현 가능하다.**  
특히 각 리소스 핸들러가 명확히 보이기 때문에, 번역 작업은 창의적 설계 문제가 아닌  
**기계적 변환 + 언어 관용구 적응 문제**로 수준이 낮아진다.

> PoC 시작을 원한다면, Go 소스를 공유해 주시면 SigV4 엔진과 EC2 DescribeInstances → DeleteInstance 흐름부터 C++로 바로 구현을 시작할 수 있습니다.
