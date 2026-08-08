# 쿼리 DSL과 최적화

## 1. 목표

쿼리는 타입 안전해야 하며 다음 정보를 컴파일 타임에 산출해야 한다.

- 쿼리가 스키마상 유효한가?
- 어떤 노드, 관계와 속성 열이 필요한가?
- 어떤 정적 재작성 규칙을 적용할 수 있는가?
- 어떤 실행 커널 후보를 생성할 것인가?

실제 cardinality와 데이터 분포에 따른 최종 커널 선택은 런타임에 수행한다.

## 2. DSL 예시

```cpp
auto q =
    from<ghost_node>()
    | active_at(t)
    | in_worldline(actual)
    | where(prop<activation> > 0.6f)
    | traverse<biases, thought_node>()
    | where(prop<confidence> > 0.7f)
    | project<node_identity, confidence>()
    | top_k<10>(by<confidence>.descending());
```

DSL은 실행하지 않고 expression AST를 만든다. 실행은 snapshot에 대해 명시적으로 요청한다.

```cpp
auto result = execute(snapshot, q);
```

## 3. 논리 연산자

- `source<Node>()`
- `active_at(time)`
- `known_at(record_time)`
- `in_worldline(id)`
- `where(expression)`
- `traverse<Relation, Target>()`
- `join(pattern)`
- `project<Properties...>()`
- `aggregate(operation)`
- `top_k<K>(order)`
- `limit(n)`

## 4. 정적 검증

컴파일 타임에 다음을 거부한다.

- 노드에 존재하지 않는 속성 접근
- 스키마에 없는 관계 탐색
- 호환되지 않는 확률 타입 결합
- 시간 질의를 지원하지 않는 저장소에서 `active_at` 호출
- projection 이후 제거된 속성을 다시 사용하는 표현
- 무한 경로를 compile-time unroll하려는 표현

## 5. 최적화 단계

### 정규화

- 비교식과 논리식을 canonical form으로 변환
- 중복 필터 제거
- 상수식 계산

### 필터 푸시다운

소스 속성 필터는 traverse 이전으로 이동한다. target 속성 필터는 이동하지 않는다.

### 필터 융합

같은 열 또는 같은 노드 저장소에 적용되는 필터를 하나의 루프로 합친다.

### 열 가지치기

최종 결과와 필터에 필요한 열만 로드한다.

### 인덱스 후보 생성

시간, 세계선, 노드 ID 및 relation adjacency 인덱스 가운데 사용할 수 있는 후보를 열거한다.

### 커널 후보 생성

```text
scalar scan
SIMD scan
sparse index lookup
single-thread traversal
parallel traversal
```

후보 수에는 상한을 둔다.

## 6. 런타임 계획

런타임 통계:

- 노드 종류별 cardinality
- 관계별 평균 및 분위수 degree
- 시간 구간 선택도
- 세계선별 delta 크기
- 속성 값 분포의 요약

최종 계획은 작은 함수 테이블 또는 `std::variant`로 선택한다. 가상 호출 제거보다 코드 크기와 branch locality가 더 중요한지 벤치마크한다.

## 7. 경로 패턴

```cpp
using influence_path = path_pattern<
    step<ghost_node, biases, thought_node>,
    step<thought_node, candidate_for, decision_node>,
    step<decision_node, selects, action_node>>;

static_assert(valid_path<memorial_schema, influence_path>);
```

고정된 짧은 경로는 특화할 수 있다. 가변 길이 탐색은 반복문과 런타임 frontier를 사용한다.

```cpp
using association_walk = bounded_path<associated_with, 1, 4>;
```

상한이 없는 재귀 탐색을 TMP로 구현하지 않는다.

## 8. 확률 전파

알고리즘은 probability semiring을 받는다.

```cpp
template<ProbabilitySemiring P, GraphSnapshot G, PathPattern Path>
auto propagate(const G&, Path, P);
```

동일한 traversal 골격으로 다음을 계산할 수 있다.

- 경로 확률 합
- 최우도 경로
- 로그 공간 주변화
- 도달 가능성
- 기대 영향 강도

독립성 가정 없이 엣지 확률을 단순 곱하지 않는다. 결합 방식은 모델 정책과 provenance에 명시한다.

## 9. 역추론

```cpp
auto posterior = infer_latent_causes(
    snapshot,
    observation,
    candidate_set<ghost_node>(),
    model);
```

결과는 하나의 고스트 ID가 아니라 후보별 posterior와 진단을 반환한다.

```text
InferenceResult
- candidate posteriors
- normalization status
- effective sample size or approximation diagnostics
- model version
- evidence cutoff
- warnings
```

## 10. 쿼리 설명

모든 쿼리에 `explain`을 제공한다.

```cpp
auto plan = explain(snapshot, q);
```

출력에는 다음이 포함된다.

- 논리 AST
- 최적화된 AST
- 필요한 열
- 선택 가능한 인덱스
- 선택한 커널
- 예상 및 실제 cardinality
- 컴파일된 query type의 짧은 안정 이름

## 11. 컴파일 비용 제한

- AST 최대 깊이
- fixed path 최대 길이
- 동시 커널 후보 최대 수
- predicate pack 크기 제한
- query signature 기반 명시적 인스턴스화 또는 캐시

진단 시 거대한 템플릿 타입 전체를 출력하지 않고 단계 이름과 실패한 concept을 출력한다.

