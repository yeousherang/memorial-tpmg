# Repository Guidelines

## Project Structure & Module Organization

This repository currently contains the design specification for Memorial TPMG, a C++23 temporal probabilistic multilayer graph library. Start with `README.md`, then follow the numbered documents in `docs/`. Architectural decisions live in `docs/adr/`; add a new numbered ADR for decisions that change core semantics, storage, APIs, or build strategy.

The planned implementation layout is:

- `include/memorial/`: public headers for schema, storage, query, policy, and graph APIs.
- `src/`: runtime, kernel, and non-template façade implementations.
- `tests/static`, `tests/compile_fail`, `tests/unit`, and `tests/integration`: verification by test category.
- `benchmarks/` and `examples/`: reproducible performance cases and runnable usage samples.

Keep documentation aligned with this structure as code is introduced.

## Build, Test, and Development Commands

No build files exist yet. Do not assume a working build until the Phase 0 CMake foundation is added. The intended workflow will be:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
cmake --build build --target format
```

When adding the initial build system, preserve these conventional entry points and document any required compiler versions or optional targets in `README.md`.

## Coding Style & Naming Conventions

Use C++23, four-space indentation, and target-based CMake. Prefer `snake_case` for functions, variables, files, and namespaces; `PascalCase` for concepts or public types only where established by the implementation; and `SCREAMING_SNAKE_CASE` only for macros. Favor value semantics, strong IDs, `std::expected` for runtime errors, and `noexcept` where contracts permit. Public headers must be self-contained. Format C++ with `clang-format` and treat compiler warnings as errors in CI.

## Testing Guidelines

Every implementation change should include the narrowest relevant test. Use `static_assert` for type contracts, compile-fail cases for invalid schemas and lifetimes, unit tests for isolated behavior, and integration tests for event replay, snapshots, and worldlines. Name tests after observable behavior, for example `snapshot_preserves_generation_lifetime`. Run sanitizer builds (ASan/UBSan) before merging memory-sensitive changes.

## Commit & Pull Request Guidelines

Git history is not available in this checkout, so no repository-specific commit convention can be inferred. Use short, imperative subjects such as `Add temporal interval validation`, and keep unrelated changes separate. Pull requests should explain the motivation, summarize behavioral or architectural effects, link relevant issues or ADRs, and list verification performed. Include benchmark results for performance claims and update affected design documents alongside code.
