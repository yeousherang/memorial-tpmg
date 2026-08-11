# Memorial TPMG

[![CI](https://github.com/yeousherang/memorial-tpmg/actions/workflows/ci.yml/badge.svg)](https://github.com/yeousherang/memorial-tpmg/actions/workflows/ci.yml)
[![License](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](LICENSE)

> **Experimental pre-alpha:** 런타임 저장소는 초기 프로토타입이며 쿼리 실행기는 아직 구현되지 않았다. API는 예고 없이 변경될 수 있으며, 이 프로젝트는 임상 또는 진단 시스템이 아니다.

Memorial TPMG는 인간의 경험, 지각, 생각, 감정, 선택, 결과, 기억 및 잠재적 영향을 시간과 세계선 위에 표현하는 `Temporal Probabilistic Multilayer Graph` 라이브러리 프로젝트다.

내부 표현은 트리가 아니라 시간 확률 다층 그래프이며, 트리는 실제 선택 경로를 보여주는 하나의 뷰다. 구현은 C++23을 기준으로 한다. TMP는 스키마 검증, 쿼리 계획 및 커널 특화에 사용하고, 실제 노드와 확률값은 런타임 SoA 저장소에 보관한다.

## 문서 지도

1. [제품 및 요구사항](docs/01-product-requirements.md)
2. [도메인과 그래프 모델](docs/02-domain-model.md)
3. [C++ 라이브러리 아키텍처](docs/03-cpp-architecture.md)
4. [정적 스키마와 공개 API](docs/04-schema-and-api.md)
5. [저장소, 시간 및 세계선](docs/05-storage-time-worldlines.md)
6. [쿼리 DSL과 최적화](docs/06-query-and-optimization.md)
7. [검증과 성능 계획](docs/07-verification.md)
8. [개발 로드맵](docs/08-roadmap.md)
9. [아키텍처 결정 기록](docs/adr/README.md)

## 권장 읽기 순서

- 기획 및 연구 담당자: 1 → 2 → 7
- 라이브러리 개발자: 2 → 3 → 4 → 5 → 6 → 7 → 8
- 신규 기여자: 1 → 3 → 8 → ADR

## 현재 상태

이 저장소는 초기 구현 단계다. C++23/CMake 프로젝트 기반, 예제, 테스트 및 compile-fail 하네스가 준비되어 있다. API와 파일 배치는 구현 과정에서 변경될 수 있으며, 중요한 변경은 ADR로 남긴다.

현재 구현된 범위는 정적 스키마, strong ID, 기본 Memorial 도메인 스키마, 컴파일타임 검증, 런타임 ID·시간·provenance 타입, append-only typed node delta, relation별 양방향 adjacency, 불변 generation snapshot, sequence 기반 이벤트 로그, checkpoint/replay, canonical state/log hash, 이중 시간 memory reinterpretation, worldline registry/fork, copy-on-write branch snapshot, 정적 쿼리 AST DSL, 기본 스냅샷 실행기, 초기 정적 최적화와 `explain()` 실행 계획이다. 비용 기반 인덱스·커널 선택은 아직 구현되지 않았다.

의사결정 에피소드 통합 예제는 다음과 같이 실행한다.

```sh
./build/dev/examples/memorial_decision_episode
```

예제는 경험·지각·생각과 결정·행동·결과·기억을 삽입한 뒤, 불변 snapshot에서 선택된 행동과 결과 confidence를 조회한다.

## 빌드 및 테스트

개발 프리셋은 CMake 3.25 이상과 Ninja를 사용한다.

```sh
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
```

ASan/UBSan 검증은 지원되는 Clang/GCC 환경에서 `asan` 프리셋으로 실행한다. 프리셋을 사용하지 않을 경우 일반적인 `cmake -S . -B build` 흐름도 지원한다.

구성 단계는 컴파일러 버전 문자열만 확인하지 않는다. 실제 코드로 `std::expected`와 structural non-type template parameter 지원을 검사하며, 필요한 C++23 기능이 없으면 구성에 실패한다. 테스트 빌드는 모든 public header를 각각 독립된 번역 단위에서 컴파일해 self-containment도 확인한다.

CI는 포맷, 일반 빌드·테스트, ASan/UBSan을 독립된 품질 게이트로 실행한다. 일반 테스트 매트릭스는 GCC/libstdc++, Clang/libstdc++, Clang/libc++, Apple Clang/libc++, MSVC/MSVC STL을 포함한다.

런타임 단위 테스트는 GoogleTest 1.17을 사용한다. CMake는 시스템에 설치된 패키지를 우선 사용하고, 찾지 못하면 구성 단계에서 고정된 릴리스를 가져온다. 네트워크 사용을 금지하려면 GoogleTest를 먼저 설치하고 `-DMEMORIAL_FETCH_DEPENDENCIES=OFF`를 지정한다. 타입 계약은 `static_assert`, 잘못된 코드 경로는 compile-fail 테스트로 계속 검증한다.

## 핵심 원칙

- 역사적 사건과 그 사건에 대한 현재의 해석을 분리한다.
- 관찰, 자기보고, 추론 및 시뮬레이션을 동일한 사실 등급으로 취급하지 않는다.
- 고스트 노드는 확정된 무의식이 아니라 증거와 불확실성을 가진 잠재 변수다.
- 실제 세계선과 반사실 세계선을 섞지 않는다.
- 미래에 얻은 정보를 과거 상태 복원에 누출하지 않는다.
- TMP의 대상은 데이터가 아니라 데이터의 형태, 계약 및 실행 규칙이다.
- 런타임 성능, 컴파일 시간, 바이너리 크기를 함께 측정한다.

## 기여 및 보안

기여 절차는 [CONTRIBUTING.md](CONTRIBUTING.md), 커뮤니티 행동 기준은 [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md)를 따른다. 보안 문제나 민감한 내용은 공개 issue 대신 [SECURITY.md](SECURITY.md)의 비공개 연락처로 제보한다.

## 라이선스와 저작자

Copyright 2026 yeousherang. Apache License 2.0으로 배포한다. 전체 조건은 [LICENSE](LICENSE), 원 저작자 표시는 [NOTICE](NOTICE), 저자 정보는 [AUTHORS.md](AUTHORS.md)에서 확인할 수 있다.
