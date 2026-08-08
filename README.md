# Memorial TPMG

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

## 빌드 및 테스트

개발 프리셋은 CMake 3.25 이상과 Ninja를 사용한다.

```sh
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
```

ASan/UBSan 검증은 지원되는 Clang/GCC 환경에서 `asan` 프리셋으로 실행한다. 프리셋을 사용하지 않을 경우 일반적인 `cmake -S . -B build` 흐름도 지원한다.

## 핵심 원칙

- 역사적 사건과 그 사건에 대한 현재의 해석을 분리한다.
- 관찰, 자기보고, 추론 및 시뮬레이션을 동일한 사실 등급으로 취급하지 않는다.
- 고스트 노드는 확정된 무의식이 아니라 증거와 불확실성을 가진 잠재 변수다.
- 실제 세계선과 반사실 세계선을 섞지 않는다.
- 미래에 얻은 정보를 과거 상태 복원에 누출하지 않는다.
- TMP의 대상은 데이터가 아니라 데이터의 형태, 계약 및 실행 규칙이다.
- 런타임 성능, 컴파일 시간, 바이너리 크기를 함께 측정한다.
