# Repository guidance

## Build and verification

- C++14 library; CMake target names are case-sensitive: `flashFairyPP` (library), `FlashFairyPPTest` (tests). CI builds Release on Linux and Windows.
- The devcontainer preconfigures a Debug/Ninja build in `build/devcontainer`; use that directory for container builds/tests to avoid mixing host and container CMake caches. See `README.md` for container and coverage commands.
- From the repository root, configure, build, then test:
  ```sh
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
  cmake --build build --config Release
  ctest --test-dir build -C Release --output-on-failure
  ```
- Configuration unconditionally fetches GoogleTest/GoogleMock `release-1.10.0` via Git (`CMakeLists.txt.in`); even a library-only build needs Git/network access on initial configuration. Downloaded sources belong in the build directory.
- CTest registers one aggregate test. Run an individual case directly: `./build/FlashFairyPPTest --gtest_filter=VirtualFlashFixture.WriteSecondPage`; use `--gtest_list_tests` to discover cases. With Visual Studio, the executable is `build/Release/FlashFairyPPTest.exe`.
- If `clang-tidy` is found at configure time, CMake runs it automatically on the library target using `.clang-tidy`.
- Optional coverage: configure with `-DENABLE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug`, then build target `FlashFairyPPTest-gcovr`; requires `gcov` and `gcovr`.
- `apply-clang.sh` hardcodes `clang-format.exe` and Windows LLVM paths. On macOS/Linux, use `clang-format -i <changed .h/.cpp files>` directly; `.clang-format` is the style source.

## Storage and test boundaries

- Public includes use `FlashFairyPP/FlashFairyPP.h` with `lib` as the include root. Bulk writes and page compaction are templates implemented in that header, not in `FlashFairyPP.cpp`.
- Hardware integration supplies four `extern "C"` hooks: `FlashFairy_Erase_Page`, `FlashFairy_Write_Word`, `flash_lock`, and `flash_unlock`. The repository's implementations in `test/Mocks.cpp` are host-test substitutes, not an STM32 driver.
- Storage uses two 1024-byte pages and 32-bit records (key in upper 16 bits, value in lower 16 bits). Valid keys for `setValue` are 0–255; erased records are `0xFFFFFFFF`, while missing reads return `npos` (`0xCAFE`).
- Reads visit records oldest-first, so a key may appear multiple times; the last occurrence wins. Compaction scans backward and excludes keys supplied by the incoming visitor; bulk-write visitors need both iteration over key/value pairs and `contains(key)`.
- Host tests use `VirtualFlashFixture` with RAM-backed pages and assume little-endian byte layout. Page size is also hardcoded in `test/Mocks.h` and `test/Mocks.cpp`; changing `Config_t::pageSize` alone does not update the fixture.
