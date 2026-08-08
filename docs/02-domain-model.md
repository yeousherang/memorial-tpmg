# 도메인과 그래프 모델

## 1. 형식적 정의

TPMG는 다음 튜플로 정의한다.

\[
G = (V, E, L, T_v, T_r, W, P, M)
\]

- `V`: 엔티티 및 시간 상태
- `E`: 타입이 있는 시간 확률 엣지
- `L`: 레이어 집합
- `T_v`: 삶에서 유효한 시간
- `T_r`: 시스템에 기록되어 알려진 시간
- `W`: 세계선 집합
- `P`: 확률, 분포 및 추론 신뢰도
- `M`: 모델 및 생성 과정의 메타데이터

## 2. 엔티티와 상태

정체성이 지속되는 대상과 시간에 따라 변하는 값을 분리한다.

```text
Entity
- EntityId
- NodeKind
- LayerId
- canonical label
- immutable metadata

TemporalState
- EntityId
- valid interval
- transaction interval
- WorldlineId
- property values
- uncertainty
- provenance
```

예를 들어 `실패 예상`이라는 고스트 엔티티는 하나지만, 활성도와 접근성은 시점 및 세계선마다 다르다.

## 3. 노드 종류

| 종류 | 기본 레이어 | 설명 |
| --- | --- | --- |
| Experience | WORLD | 외부 사건 또는 경험의 기준점 |
| Perception | PERCEPTION | 주관적으로 인지된 상황 |
| Thought | COGNITION | 생각, 예측, 판단 또는 후보 |
| Belief | COGNITION | 비교적 지속적인 믿음 |
| Goal | COGNITION | 활성화된 목표 |
| Emotion | AFFECT_BODY | 정서 상태 |
| BodyState | AFFECT_BODY | 피로, 각성 등 신체 상태 |
| Decision | ACTION | 후보가 경쟁하는 결정 사건 |
| Action | ACTION | 실제 또는 시뮬레이션 행동 |
| Outcome | WORLD | 행동 이후 관찰된 결과 |
| Memory | MEMORY | 사건에 대한 기억 표현 |
| Ghost | LATENT_GHOST | 직접 관찰되지 않는 잠재 상태 |

## 4. 현실성과 관찰 가능성

두 축을 독립적으로 관리한다.

```text
RealityStatus
- OBSERVED
- SELF_REPORTED
- INFERRED
- SIMULATED
- COUNTERFACTUAL

ObservationStatus
- CONSCIOUS
- PARTIALLY_ACCESSIBLE
- UNREPORTED
- LATENT
```

`SELF_REPORTED + CONSCIOUS`와 `INFERRED + LATENT`는 전혀 다른 증거 등급이다. API와 출력은 이를 잃어서는 안 된다.

## 5. 엣지

```text
TemporalProbabilisticEdge
- EdgeId
- source EntityId
- target EntityId
- RelationKind
- valid interval
- transaction interval
- WorldlineId
- existence probability
- strength distribution
- causal status
- provenance
```

### 인과, 연상과 증거의 구분

- `CAUSES`: 개입적 또는 강한 생성 가정을 가진 관계
- `ASSOCIATED_WITH`: 방향 없는 통계적 연관
- `BIASES`: 선택 확률 또는 상태 전이에 영향을 주는 모델 관계
- `EVIDENCE_FOR`: 관찰이 잠재 가설을 지지하는 추론 관계

`Ghost -> BIASES -> Action`과 `Action -> EVIDENCE_FOR -> Ghost`는 반대 방향의 서로 다른 의미다. 동일 엣지로 합치지 않는다.

## 6. 의사결정 에피소드

한 번의 선택은 단일 노드가 아니라 다음 서브그래프다.

```text
context -> perception -> candidate thoughts
                         ^
ghost/emotion -----------|

candidate thoughts -> decision -> selected action -> outcome
                            |                         |
                            +-> rejected candidates  +-> memory update
```

관리 단위로 `DecisionEpisode`를 둔다.

```text
DecisionEpisode
- EpisodeId
- time window
- WorldlineId
- context nodes
- candidate nodes
- active latent hypotheses
- selected action
- outcomes
- generated updates
```

## 7. 선택 모델

행동 후보의 효용은 여러 층의 입력을 합성한다.

\[
Q(a_i,t) = b_i + \sum_l \sum_{j \in N_l(i)} w_{ji,t}x_{j,t}
\]

선택 확률의 기본 모델은 lapse를 포함한 softmax다.

\[
P(a_i|S_t) = (1-\lambda)
\frac{\exp(\beta Q(a_i,t))}{\sum_k\exp(\beta Q(a_k,t))}
+ \lambda P_{lapse}(a_i)
\]

이는 인간이 실제로 해당 산술을 수행한다는 주장이 아니라 관찰을 설명하고 예측하는 생성 모델이다.

## 8. 고스트 모델

고스트는 다음 상태를 가진 잠재 엔티티다.

```text
GhostState
- activation
- accessibility
- persistence
- emotional valence
- posterior probability
- candidate origins
- evidence links
- model version
```

기본 전이는 다음과 같다.

\[
g_{k,t+1}=\rho_k g_{k,t}+\sum_jw_{jk}x_{j,t}-r_{k,t}+\eta_{k,t}
\]

- `rho`: 지속성
- `x`: 활성화 자극
- `r`: 재평가 또는 해소 효과
- `eta`: 설명되지 않는 변동

동일 관찰을 이용해 고스트를 만들고 같은 관찰로 다시 검증하는 순환을 피해야 한다. 가능하면 홀드아웃 데이터나 후속 관찰로 검증한다.

## 9. 기억과 재해석

역사적 사건과 기억 표현을 분리한다.

```text
HistoricalEvent E17
  <- REPRESENTS - Memory M17_2018
  <- REPRESENTS - Memory M17_2022

M17_2022 - REINTERPRETS -> M17_2018
```

재해석은 사건을 수정하지 않는다. 새로운 기억 표현과 버전 관계를 추가한다.

## 10. 세계선

```text
Worldline
- WorldlineId
- optional parent WorldlineId
- fork point
- intervention
- assumptions
- epistemic status
- simulation provenance
```

분기 이전 상태는 부모와 공유하고, 분기 이후 변경만 copy-on-write로 저장한다. 반사실 노드는 `OBSERVED`로 승격할 수 없다.

## 11. 시간 가역성

가역성은 세 가지로 정의한다.

1. 상태 가역성: 이벤트 로그에서 이전 상태를 복원한다.
2. 경로 가역성: 과거 선택점에서 새 세계선을 만든다.
3. 추론 가역성: 현재 관찰에서 과거 잠재 원인의 사후확률을 계산한다.

완전한 과거 복원이나 유일한 무의식 원인을 보장하지 않는다.

## 12. 불변조건

- 동일 세계선에서 한 엔티티의 상충하는 상태 구간은 정책 없이 겹치지 않는다.
- `recorded_at` 이후에만 해당 정보가 질의에서 알려진 것으로 간주된다.
- 실제 세계선의 역사 이벤트는 덮어쓰지 않는다.
- 인과 엣지는 단순 연상보다 높은 증거 요건을 요구한다.
- 모든 `INFERRED` 및 `SIMULATED` 값은 provenance를 가진다.
- 확률값의 의미와 결합 연산은 probability policy로 결정한다.

