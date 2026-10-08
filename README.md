# FlashFairyPP

![Windows CI](https://github.com/deltaphi/FlashFairyPP/workflows/Windows%20CI/badge.svg)
![Linux CI](https://github.com/deltaphi/FlashFairyPP/workflows/Linux%20CI/badge.svg)

A Flash-EEPROM emulation initially geared at the STM32.

## Development container

With Docker running and the VS Code **Dev Containers** extension installed, open
this repository and run **Dev Containers: Reopen in Container**.

The container includes GCC, Clang, CMake, Ninja, GDB, clang-format, clang-tidy,
and gcovr. Creation configures a Debug build in `build/devcontainer`; initial
configuration needs network access to fetch GoogleTest 1.10.0. Build and test:

```sh
cmake --build build/devcontainer
ctest --test-dir build/devcontainer --output-on-failure
```

Run a focused test or format a changed file:

```sh
./build/devcontainer/FlashFairyPPTest --gtest_filter=VirtualFlashFixture.WriteSecondPage
clang-format -i lib/FlashFairyPP/FlashFairyPP.cpp
```

CMake runs clang-tidy automatically when building the library. For debugging,
select `FlashFairyPPTest` as the CMake Tools launch target and use **CMake: Debug**.
These are RAM-backed host tests; no STM32 hardware is required.

For coverage, use a separate build directory:

```sh
cmake -S . -B build/coverage -G Ninja -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON
cmake --build build/coverage --target FlashFairyPPTest-gcovr
```
