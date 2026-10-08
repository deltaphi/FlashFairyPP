# Keep CMake 3.x compatibility with the project's GoogleTest 1.10 download project.
FROM mcr.microsoft.com/devcontainers/base:ubuntu-22.04

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        clang \
        clang-format \
        clang-tidy \
        cmake \
        gdb \
        gcovr \
        git \
        llvm \
        ninja-build \
    && rm -rf /var/lib/apt/lists/*
