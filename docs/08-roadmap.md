# 개발 로드맵

## 1. 개발 전략

처음부터 전체 추론 엔진을 만들지 않는다. 먼저 의미적 불변조건과 저장 구조를 안정화하고, 그 위에 쿼리 및 모델링 기능을 추가한다.

각 단계는 실행 가능한 예제, 테스트와 벤치마크를 함께 제공해야 완료된다.

## 2. 단계 0: 프로젝트 기반

산출물:

- CMake 프로젝트와 target 구조
- C++23 configure test
- Clang/GCC/MSVC CI
- 경고, sanitizer 및 formatting 설정
- 테스트와 compile-fail harness
- benchmark harness

완료 조건:

- 빈 라이브러리와 예제가 모든 CI 대상에서 빌드된다.
- 의도적으로 실패하는 compile-fail 테스트가 올바르게 판정된다.

## 3. 단계 1: 정적 스키마 코어

산출물:

- `type_list`, `node_spec`, `edge_spec`, `graph_schema`
- strong ID
- node/property/relation concepts
- consteval schema validation
- 기본 Memorial schema

완료 조건:

- 유효 스키마 예제가 컴파일된다.
- 잘못된 엔드포인트, 중복 속성, 잘못된 레이어 관계가 컴파일 단계에서 거부된다.
- 세 컴파일러의 오류 메시지가 최소한 실패한 concept을 표시한다.

## 4. 단계 2: 런타임 저장소

산출물:

- node 종류별 SoA
- typed node ID lookup
- append-only delta
- temporal state와 provenance
- relation별 adjacency
- 기본 snapshot

완료 조건:

- 의사결정 에피소드 하나를 삽입하고 조회한다.
- 잘못된 ID, 시간 구간과 세계선 조합을 거부한다.
- ASan/UBSan 단위 테스트가 통과한다.

## 5. 단계 3: 시간과 이벤트

산출물:

- valid/transaction time
- immutable event log
- checkpoint와 replay
- memory reinterpretation
- canonical graph hash

완료 조건:

- 동일 로그를 재생해 동일 해시를 얻는다.
- 당시 관점과 현재 관점의 과거 질의 결과가 구분된다.

## 6. 단계 4: 세계선

산출물:

- fork 및 intervention
- parent lookup
- copy-on-write delta
- materialized branch snapshot
- observed/simulated 상태 검증

완료 조건:

- 선택 하나를 바꾼 자식 세계선을 생성한다.
- 자식 변경이 부모에 누출되지 않는다.
- 분기 이전 데이터가 물리적으로 불필요하게 복제되지 않는다.

## 7. 단계 5: 정적 쿼리 DSL

산출물:

- source, filter, time, worldline, traverse, project, top-k
- expression AST
- 스키마 검증
- scalar 기준 실행기
- explain 출력

완료 조건:

- 주요 질의가 수동 기준 구현과 동일한 결과를 낸다.
- 없는 속성과 관계가 compile-fail 테스트에서 거부된다.

## 8. 단계 6: 최적화기와 frozen 저장소

산출물:

- 필터 푸시다운 및 융합
- 열 가지치기
- frozen CSR/CSC
- compaction
- 런타임 통계
- scalar/SIMD/index 커널 선택

완료 조건:

- 최적화 전후 결과가 속성 기반 테스트에서 동일하다.
- Tiny/Small benchmark와 컴파일 비용 보고서를 생성한다.
- 성능 개선이 없는 특화는 제거하거나 비활성화한다.

## 9. 단계 7: 확률 전파와 고스트 추론

산출물:

- probability semiring
- influence propagation
- latent cause inference interface
- model registry 및 provenance
- synthetic data generator

완료 조건:

- 알려진 작은 그래프에서 수작업 계산과 일치한다.
- 파라미터 및 모델 회복 테스트가 포함된다.
- posterior 진단과 모델 버전이 결과에 포함된다.

## 10. 단계 8: 안정화

산출물:

- 직렬화 형식
- ABI façade 필요성 검증 및 선택
- fuzzing
- TSan
- 성능 및 빌드 비용 예산
- 사용자 예제와 API reference

완료 조건:

- MVP 요구사항 전체 충족
- 알려진 제약과 지원 범위를 release note에 기록
- 스키마 및 직렬화 마이그레이션 정책 수립

## 11. 첫 번째 구현 백로그

우선순위 순서:

1. CMake와 테스트 harness
2. strong ID와 `type_list`
3. node/property/edge spec
4. schema validation 및 compile-fail test
5. typed SoA prototype
6. temporal state와 event log
7. snapshot 조회
8. 실제 에피소드 예제
9. worldline fork
10. scalar query DSL

## 12. 위험 목록

| 위험 | 대응 |
| --- | --- |
| 템플릿 인스턴스 폭증 | 후보 수 상한, 명시적 인스턴스화, 빌드 지표 |
| 타입 오류 진단 악화 | 도메인 concept, 작은 public AST, compile-fail 검사 |
| query view dangling | snapshot 소유권, owning async handle, negative test |
| 세계선 조회 비용 증가 | materialization, 깊이 제한, benchmark |
| 확률 의미 혼합 | probability policy와 provenance 필수화 |
| 고스트 과잉해석 | inferred 상태, 경쟁 가설, 홀드아웃 검증 |
| 파일 형식과 메모리 배치 결합 | 독립적인 versioned serialization |
| 최적화 복잡도 | scalar reference executor 유지 |

