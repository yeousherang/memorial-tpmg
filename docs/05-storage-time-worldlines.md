# 저장소, 시간 및 세계선

## 1. 저장 전략

그래프를 두 영역으로 나눈다.

```text
FrozenGraph
- 읽기 전용
- 노드 종류별 SoA
- 관계 종류별 CSR/CSC
- 시간 및 세계선 기준 정렬

DeltaGraph
- append-only
- chunked SoA
- 작은 adjacency buffer
- 빠른 트랜잭션

Snapshot
- 특정 frozen generation과 delta generation의 논리적 결합
```

## 2. SoA

노드 종류마다 속성 열을 분리한다.

```text
GhostStore
- ids[]
- valid_from[]
- valid_to[]
- transaction_from[]
- transaction_to[]
- worldline_ids[]
- activation[]
- confidence[]
- valence[]
```

장점:

- 필요한 열만 읽는다.
- 연속된 float 열에 SIMD 필터를 적용한다.
- 동일 노드 종류를 대량 순회할 때 캐시 효율이 높다.

## 3. 이중 시간

```text
ValidInterval
- 사건이나 상태가 삶에서 유효했던 기간

TransactionInterval
- 시스템이 해당 버전을 사실 또는 주장으로 알고 있었던 기간
```

질의는 두 시간을 모두 지정할 수 있다.

```cpp
snapshot(valid_at(t_2018), known_at(t_2026));
```

이는 2026년 현재 알고 있는 정보로 2018년을 보는 질의다.

```cpp
snapshot(valid_at(t_2018), known_at(t_2018));
```

이는 2018년 당시 알려진 정보만으로 보는 질의다.

## 4. 시간 인덱스

초기 구현:

- 상태는 `(worldline, valid_from)` 기준 정렬
- 종료 시간은 별도 열
- 시간 단면은 lower_bound와 구간 필터 조합

데이터가 커지면 다음을 비교한다.

- interval tree
- segment tree
- roaring bitmap 기반 이산 시간 인덱스
- 세계선과 시간 복합 B-tree

인덱스 선택은 벤치마크 전에 고정하지 않는다.

## 5. 세계선 저장

```text
WorldlineRecord
- id
- parent id
- fork event sequence
- fork valid time
- intervention id
- simulation provenance
- generation id
```

조회 우선순위:

1. 현재 세계선의 delta
2. 현재 세계선의 frozen override
3. 부모 세계선
4. 루트 세계선

분기 깊이가 깊어지면 lookup 비용이 증가하므로 주기적으로 materialized snapshot을 생성한다.

## 6. 불변 이벤트 로그

```text
GraphEvent
- sequence number
- event kind
- valid time
- recorded time
- worldline id
- command payload
- previous-value reference
- provenance
- checksum
```

이벤트 예:

- EntityCreated
- StateAppended
- EdgeCreated
- WorldlineForked
- MemoryReinterpreted
- ModelInferenceRecorded
- SnapshotCompacted

논리 삭제는 tombstone 이벤트로 표현한다. 역사 이벤트를 물리적으로 제거하는 개인정보 삭제 요구는 별도의 redaction 절차와 감사 기록이 필요하다.

## 7. 가역성

두 경로로 상태 복원을 검증한다.

```text
reverse(current state, inverse event)
replay(checkpoint, event range)
```

두 결과의 canonical hash가 같아야 한다. 모든 이벤트에 수작업 역연산이 필요한 것은 아니다. 체크포인트 재생이 기준 구현이며, 역연산은 낮은 지연이 필요한 이벤트에 선택적으로 제공한다.

## 8. Compaction

Compaction 절차:

1. 현재 frozen 및 닫힌 delta generation을 고정한다.
2. tombstone과 중복 상태를 정책에 따라 정리한다.
3. 노드를 종류별 SoA로 재배치한다.
4. 엣지를 관계별 CSR/CSC로 생성한다.
5. 인덱스와 통계를 계산한다.
6. canonical hash를 검증한다.
7. 새 generation을 원자적으로 게시한다.

이전 generation은 활성 snapshot이 모두 해제된 뒤 회수한다.

## 9. 메모리 자원

- persistent: frozen store와 장기 문자열
- transaction: delta command와 event payload
- scratch: query frontier, sort, top-k

PMR container보다 `memory_resource`의 수명이 길어야 한다. 이를 graph 또는 generation 객체의 소유권으로 보장한다.

## 10. 직렬화

직렬화 형식에는 다음이 필요하다.

- magic 및 format version
- schema fingerprint
- endianness
- ID width
- time representation
- probability representation
- model/provenance section
- per-section checksum

메모리 레이아웃을 그대로 파일 형식으로 사용하지 않는다. 내부 SoA 배치는 변경 가능해야 한다.

