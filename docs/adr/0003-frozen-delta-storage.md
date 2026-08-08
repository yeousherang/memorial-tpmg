# ADR-0003: Frozen plus Delta 저장소

## 상태

제안됨

## 맥락

삶의 기록은 append가 많지만 분석은 대량 읽기와 순회를 요구한다. 단일 mutable adjacency 구조는 쓰기는 편하지만 캐시 효율과 일관된 snapshot 제공이 어렵다.

## 결정

읽기 최적화된 frozen SoA/CSR generation과 append 최적화된 delta generation을 결합한다. 주기적인 compaction으로 새 frozen generation을 게시한다.

## 결과

- 읽기와 쓰기 경로를 각각 최적화할 수 있다.
- snapshot 격리가 명확해진다.
- 질의는 frozen과 delta 결과를 병합해야 한다.
- compaction과 generation 수명 관리가 필요하다.

## 대안

- 단일 mutable graph: 구현은 단순하지만 조회 성능과 snapshot 비용이 불리하다.
- 매 변경마다 immutable graph 생성: 의미는 단순하지만 대규모 그래프에서 복사 비용이 크다.

