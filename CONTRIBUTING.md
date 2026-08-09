# Contributing to Memorial TPMG

Thank you for helping improve Memorial TPMG. The project is pre-alpha: APIs and file layouts may change without compatibility guarantees.

## Before You Start

For substantial API, storage, schema, or semantic changes, open an issue before implementation. Decisions that affect long-term architecture should include or update an ADR in `docs/adr/`. Never include real sensitive personal or clinical data in examples, fixtures, issues, or pull requests.

## Development Workflow

Use CMake 3.25 or newer, Ninja, and a compiler with the required C++23 features.

```sh
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
cmake --build build/dev --target format-check
```

For memory-sensitive changes, also run:

```sh
cmake --preset asan
cmake --build --preset asan
ctest --preset asan
```

Add the narrowest applicable verification: `static_assert` for type contracts, compile-fail fixtures for invalid programs, and GoogleTest for runtime behavior. Public headers must compile independently.

## Pull Requests

Keep changes focused and use short, imperative commit subjects. A pull request should explain its motivation, behavioral or architectural impact, related issues or ADRs, and verification performed. Include benchmark evidence for performance claims. All CI checks must pass before merge.

By submitting a contribution, you agree that it is licensed under Apache License 2.0 as described in `LICENSE`. Contributor authorship remains recorded in Git history; the original project attribution in `NOTICE` must be preserved.

