# C++ 라이브러리 아키텍처

## 1. 기술 기준

- 언어: C++23
- 빌드: target 기반 CMake
- 컴파일러: Clang, GCC, MSVC 최근 안정 버전
- 오류 모델: 코어 핫 패스는 `std::expected`, 프로그래머 계약 위반은 assertion 또는 compile-time failure
- RTTI: 코어에서 필요하지 않음
- 예외: façade 정책에 따라 허용하되 소멸자와 이동 연산의 `noexcept`를 정확히 유지
- 메모리: value semantics, strong ID, `std::pmr` 기반 arena와 SoA

## 2. 아키텍처 원칙

TMP가 담당하는 영역:

- 타입 수준 스키마
- 유효한 노드, 속성 및 관계 검증
- 쿼리 AST와 정적 재작성
- 필요한 열 계산
- 저장소 및 커널 후보 생성

런타임이 담당하는 영역:

- 실제 노드와 엣지 개수
- 타임스탬프와 세계선
- 확률값과 posterior sample
- cardinality 통계
- 대규모 탐색 frontier
- 실행 커널의 최종 선택

실제 데이터 값을 NTTP로 만들지 않는다.

## 3. 컴포넌트

```text
memorial-meta
  타입 리스트, fixed string, type map, 메타 predicate

memorial-schema
  node_spec, edge_spec, graph_schema, concepts, consteval validation

memorial-storage
  SoA entity/state stores, adjacency, temporal indexes, snapshots

memorial-query
  expression template DSL, logical planner, rewrite optimizer

memorial-kernels
  traversal, temporal join, propagation, inference

memorial-runtime
  graph, transaction, event log, compaction, model registry

memorial-api
  선택적인 비템플릿 façade와 직렬화 경계
```

## 4. 저장소 구성

```cpp
template<
    typename Schema,
    typename ProbabilityPolicy,
    typename TemporalPolicy,
    typename StoragePolicy,
    typename ConcurrencyPolicy,
    typename ValidationPolicy>
class basic_graph;
```

권장 기본 조합:

```cpp
using memorial_graph = basic_graph<
    memorial_schema,
    log_probability_policy<double>,
    bitemporal_policy,
    frozen_delta_soa_policy,
    snapshot_concurrency_policy,
    strict_validation_policy>;
```

정책 개수가 늘어나면 사용자 API가 불안정해지므로 `default_graph<Schema>` 별칭과 builder를 제공한다.

## 5. 정적 및 동적 다형성

- 코어 알고리즘: concept을 만족하는 free function 또는 policy type
- 핫 패스: `if constexpr`와 제한된 커널 특화
- 런타임 이종 모델: `std::variant` 또는 좁은 type erasure
- ABI 경계: non-template façade와 opaque handle
- CRTP: 공통 인터페이스 공유가 아니라 실제 비용 제거가 입증된 경우에만 사용

## 6. 소유권과 수명

- `graph`가 persistent memory resource를 소유하거나 명시적으로 빌린다.
- `snapshot`은 참조하는 frozen/delta 세대의 수명을 유지한다.
- `query_view`는 기본적으로 non-owning이며 원본 snapshot보다 오래 살 수 없다.
- 비동기 실행은 view가 아니라 owning snapshot handle을 캡처한다.
- expression template은 lvalue 참조와 rvalue 소유를 구분한다.

```cpp
template<typename T>
using capture_t = std::conditional_t<
    std::is_lvalue_reference_v<T>,
    std::reference_wrapper<std::remove_reference_t<T>>,
    std::remove_cvref_t<T>>;
```

공개 DSL에서 dangling 방지를 위한 negative compile test를 둔다.

## 7. 동시성

MVP는 single-writer, multi-reader 스냅샷 모델을 사용한다.

- writer는 delta 세대에 append한다.
- reader는 불변 generation handle을 획득한다.
- compaction은 새 frozen generation을 만든 뒤 원자적으로 게시한다.
- 이전 generation은 마지막 reader가 해제할 때 제거한다.

그래프 객체 전체에 하나의 mutex를 두는 구현은 프로토타입에서만 허용한다.

## 8. 오류 처리

```cpp
enum class graph_errc {
    invalid_id,
    invalid_interval,
    worldline_mismatch,
    probability_out_of_domain,
    missing_provenance,
    conflict,
    capacity_exceeded
};

template<typename T>
using result = std::expected<T, graph_error>;
```

스키마 오류는 컴파일 타임, 데이터 오류는 `expected`, 내부 불변조건 파괴는 assertion 또는 명확한 실패로 처리한다.

## 9. ABI와 배포

두 사용 모델을 지원할 수 있다.

1. 소스 특화 모델: 사용자가 자신의 Schema로 템플릿을 인스턴스화한다.
2. 안정 façade 모델: 지원 스키마와 정책을 라이브러리가 명시적으로 인스턴스화한다.

공유 라이브러리 ABI에는 STL container, allocator, 내부 expression type을 직접 노출하지 않는다.

## 10. 컴파일 시간 관리

- public header와 heavy implementation header를 분리한다.
- common schema와 kernel은 명시적으로 인스턴스화한다.
- 재귀 TMP 대신 fold expression, `std::apply`, `index_sequence`를 우선한다.
- 쿼리 AST 깊이와 커널 후보 수에 상한을 둔다.
- Clang time trace와 바이너리 symbol report를 CI artifact로 저장한다.
- 스키마 변경과 저장소 변경의 재빌드 범위를 분리한다.

## 11. 제안 디렉터리

```text
include/memorial/
  meta/
  schema/
  storage/
  query/
  policy/
  graph.hpp

src/
  runtime/
  kernels/
  facade/

tests/
  static/
  compile_fail/
  unit/
  integration/
  property/
  fuzz/

benchmarks/
examples/
cmake/
```

