# 정적 스키마와 공개 API

## 1. 타입 수준 스키마

```cpp
template<typename... Ts>
struct type_list {};

template<typename Tag, typename Layer, typename... Properties>
struct node_spec {
    using tag = Tag;
    using layer = Layer;
    using properties = type_list<Properties...>;
};

template<typename Source, typename Relation, typename Target,
         typename... Properties>
struct edge_spec {
    using source = Source;
    using relation = Relation;
    using target = Target;
    using properties = type_list<Properties...>;
};
```

스키마는 노드와 엣지 목록을 가진다.

```cpp
using memorial_schema = graph_schema<
    type_list<experience_spec, thought_spec, decision_spec,
              action_spec, outcome_spec, memory_spec, ghost_spec>,
    type_list<experience_generates_thought,
              ghost_biases_thought,
              decision_selects_action,
              action_causes_outcome>>;
```

## 2. 속성

초기 구현은 문자열 NTTP 대신 enum 기반 키를 권장한다.

```cpp
enum class property_key : std::uint16_t {
    activation,
    confidence,
    valence,
    existence_probability,
    strength
};

template<property_key Key, typename T>
struct property_spec {
    static constexpr auto key = Key;
    using value_type = T;
};
```

문자열 이름은 별도 constexpr metadata table로 제공한다. 이렇게 하면 오류 진단은 유지하면서 인스턴스 수를 제한할 수 있다.

## 3. Concepts

```cpp
template<typename T>
concept NodeSpec = requires {
    typename T::tag;
    typename T::layer;
    typename T::properties;
};

template<typename Schema, typename Src, typename Rel, typename Dst>
concept ValidEdge = Schema::template contains_edge<Src, Rel, Dst>;

template<typename T>
concept ProbabilitySemiring = requires(T a, T b) {
    { combine(a, b) } -> std::same_as<T>;
    { marginalize(a, b) } -> std::same_as<T>;
};
```

Concept 이름은 컴파일 오류에서 도메인 계약이 드러나게 설계한다.

## 4. Strong ID

```cpp
template<typename Tag>
class node_id {
public:
    using value_type = std::uint32_t;

    explicit constexpr node_id(value_type value) noexcept
        : value_{value} {}

    [[nodiscard]] constexpr value_type value() const noexcept {
        return value_;
    }

    friend constexpr auto operator<=>(node_id, node_id) = default;

private:
    value_type value_;
};
```

`node_id<ghost>`와 `node_id<action>`은 상호 변환되지 않는다. 직렬화 경계에서만 raw ID로 변환한다.

## 5. 노드 삽입 API

```cpp
auto ghost = graph.insert<ghost_node>(
    worldline,
    valid_interval,
    inferred_provenance{model_run},
    set<activation>(0.42f),
    set<confidence>(0.61f));
```

필수 속성과 provenance가 빠지면 컴파일 타임 또는 입력 builder 완료 시 실패한다.

런타임 JSON처럼 동적 입력을 받는 경계에서는 검증 후 typed command로 변환한다.

## 6. 연결 API

```cpp
auto edge = graph.connect<biases>(
    ghost,
    thought,
    temporal_scope,
    probability{0.68},
    normal_strength{0.35, 0.12},
    inferred_provenance{model_run});
```

소스와 대상 ID 타입으로 관계 유효성을 검사한다.

```cpp
template<typename Rel, typename Src, typename Dst>
requires ValidEdge<Schema, typename Src::tag, Rel, typename Dst::tag>
result<edge_id> connect(Src, Dst, ...);
```

## 7. 상태 버전 API

```cpp
auto next = graph.update_state(
    ghost,
    new_valid_interval,
    set<activation>(0.73f),
    caused_by(trigger_event));
```

이 연산은 기존 상태를 덮어쓰지 않고 새 `TemporalState`와 이벤트를 추가한다.

## 8. 메모리 재해석 API

```cpp
auto revision = graph.reinterpret(
    previous_memory,
    current_time,
    perspective_id,
    memory_payload{...},
    self_reported_provenance{...});
```

역사적 사건 ID는 유지되며 `REINTERPRETS` 및 `REPRESENTS` 엣지가 추가된다.

## 9. 세계선 API

```cpp
auto alternative = graph.fork_worldline(
    actual_worldline,
    decision,
    intervention::replace_action(new_action),
    simulation_context{model_version, seed});
```

새 세계선은 부모와 분기 이전 스냅샷을 공유한다.

## 10. 읽기 API

```cpp
auto snapshot = graph.snapshot(
    valid_at(time),
    known_at(record_time),
    in_worldline(worldline));

auto state = snapshot.state(ghost);
```

`snapshot`이 살아 있는 동안 반환된 view가 유효해야 한다. view를 비동기 작업이나 장기 저장소에 넘기는 API는 제공하지 않거나 owning handle을 요구한다.

## 11. 확장점

공개 확장점은 다음 중 하나로 제한한다.

- schema type 구성
- policy type
- 명시적인 CPO
- runtime model registry의 type-erased model

임의의 traits specialization이나 전역 operator overload를 기본 확장점으로 사용하지 않는다.

## 12. API 안정성

- `meta` 및 expression 내부 타입은 API 안정성을 보장하지 않는다.
- node, relation tag 및 strong ID는 소스 호환 대상이다.
- façade API만 바이너리 호환 후보로 취급한다.
- 직렬화 형식 버전은 C++ ABI 버전과 독립적으로 관리한다.

