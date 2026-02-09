# Repro Checklist

Use this checklist before filing a bug:

1. Confirm the issue reproduces on the latest local build.
2. Capture exact steps from app launch to failure.
3. Record expected behavior and actual behavior.
4. Reduce to a minimal project/media set that still reproduces.
5. Include environment details and preset used.

Recommended validation commands:

```bash
cmake --preset dev-debug
cmake --build --preset dev-debug --parallel
ctest --preset dev-debug
```

If the bug looks memory/undefined-behavior related, also run:

```bash
cmake --preset sanitizer-debug
cmake --build --preset sanitizer-debug --parallel
ctest --preset sanitizer-debug
```

Minimum environment details to include:

- OS and version
- Qt version
- libmpv version
- CMake version
- C++ compiler and version
- Preset name (`dev-debug` or `sanitizer-debug`)
