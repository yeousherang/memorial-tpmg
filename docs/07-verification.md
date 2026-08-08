# 검증과 성능 계획

## 1. 검증 원칙

이 라이브러리는 세 가지를 동시에 검증해야 한다.

1. C++ 타입 및 수명 안전성
2. 그래프와 시간 모델의 의미적 정확성
3. 확률 모델의 식별 가능성과 예측 성능

빠른 실행만으로 모델의 과학적 타당성을 주장하지 않는다.

## 2. 정적 테스트

`static_assert` 테스트 대상:

- 노드 스키마 만족 여부
- 유효 및 무효 엣지
- property 존재 여부
- 경로 패턴 유효성
- policy concept 만족 여부
- strong ID 간 비변환성
- trivial/noexcept 요구사항

## 3. Compile-fail 테스트

별도 CMake target으로 다음 소스가 실패하는지 검사한다.

- 잘못된 source/target relation
- 없는 속성 열 접근
- provenance 없는 inferred node 삽입
- invalid interval을 정적으로 구성
- snapshot보다 오래 사는 borrowed query view
- 지원하지 않는 probability policy
- 반사실 노드를 observed로 기록

오류 메시지에 기대한 concept 이름이 포함되는지도 확인한다.

## 4. 단위 테스트

- strong ID와 handle
- SoA column 접근
- interval overlap 및 containment
- event append/replay
- snapshot generation lifetime
- CSR/CSC traversal
- 세계선 override lookup
- probability combine/marginalize
- query rewrite rules

## 5. 속성 기반 테스트

무작위 작은 그래프를 생성해 다음 속성을 검사한다.

- frozen 및 delta 질의 결과가 기준 구현과 동일하다.
- compaction 전후 canonical graph hash가 동일하다.
- checkpoint replay와 전체 replay가 동일하다.
- 역연산 지원 이벤트는 forward 후 inverse에서 원상 복원된다.
- 실제 세계선의 변경이 자식 세계선에 의도대로 보이고 반대 방향으로 누출되지 않는다.
- 필터 푸시다운 전후 결과가 동일하다.

## 6. Fuzzing

- 직렬화 reader
- 동적 schema 입력 변환기
- event log parser
- query DSL의 런타임 parser가 추가될 경우 해당 parser
- 손상된 interval 및 worldline 참조

ASan/UBSan과 함께 실행한다.

## 7. 동시성 테스트

- writer append 중 reader snapshot 일관성
- compaction generation 교체
- 마지막 reader 해제 이후 generation 회수
- TSan 기반 data race 검사
- 취소되거나 실패한 transaction의 원자성

## 8. 확률 모델 검증

### 파라미터 회복

알려진 파라미터로 데이터를 시뮬레이션하고 학습률, 선택 민감도, lapse, 고스트 지속성 및 영향 강도를 회복할 수 있는지 검사한다.

### 모델 회복

경쟁 모델에서 데이터를 생성하고 올바른 모델을 선택할 수 있는지 confusion matrix로 평가한다.

### 사전 및 사후 예측 검사

- 불가능한 확률이나 상태가 생성되지 않는가?
- 선택 분포와 시간별 활성도가 관찰 패턴을 재현하는가?
- 특정 고스트 하나가 모든 행동을 과도하게 설명하지 않는가?

### 홀드아웃 검증

인물, 세션, 에피소드 또는 시간 구간을 홀드아웃한다. 고스트를 추론한 데이터와 동일한 데이터만으로 성공을 평가하지 않는다.

## 9. 벤치마크 데이터셋

| 규모 | 노드 | 엣지 | 세계선 | 목적 |
| --- | ---: | ---: | ---: | --- |
| Tiny | 1K | 5K | 2 | 정확성 및 디버깅 |
| Small | 100K | 1M | 10 | 일반 개발 |
| Medium | 10M | 100M | 100 | 확장성 |
| Stress | 정책에 따라 | 정책에 따라 | 1K+ | 한계 확인 |

실제 민감 데이터 대신 재현 가능한 synthetic generator를 우선 사용한다.

## 10. 런타임 벤치마크

- 노드 및 엣지 append 처리량
- 시간 단면 조회 지연
- 관계별 1-hop 및 고정 경로 탐색
- 필터 선택도별 scan/index crossover
- 세계선 깊이별 조회 비용
- frozen/delta 크기 비율별 조회
- compaction 처리량과 peak memory
- 확률 전파 및 top-k
- snapshot 생성과 회수

각 결과에 compiler, flags, CPU, allocator, dataset seed를 기록한다.

## 11. 빌드 비용 벤치마크

- clean build 시간
- incremental build 시간
- peak compiler memory
- template instantiation count
- object 및 최종 binary 크기
- query AST 깊이별 컴파일 시간
- schema 노드/엣지 종류 증가에 따른 기울기

Clang time trace 또는 대응 도구를 CI artifact로 보관한다.

## 12. 도구chain 행렬

```text
Clang + libc++
Clang + libstdc++
GCC + libstdc++
MSVC + MSVC STL
```

C++23 라이브러리 지원은 컴파일러 버전 문자열만 믿지 않고 feature-test macro와 CMake configure test로 확인한다.

## 13. 품질 게이트

PR 병합 전:

- formatting 및 warnings
- static/compile-fail/unit test
- ASan/UBSan
- 소형 benchmark regression
- public header self-containment

정기 실행:

- TSan
- fuzzing
- 전체 compiler matrix
- medium benchmark
- compile-time/binary-size 추세
- parameter/model recovery suite

