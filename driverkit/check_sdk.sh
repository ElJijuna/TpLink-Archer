#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
SDK_PATH=$(xcrun --sdk driverkit --show-sdk-path)
mkdir -p build/driverkit
for TARGET_ARCH in arm64 x86_64; do
  xcrun --sdk driverkit clang++ -std=c++17 -Wall -Wextra -Werror -target "$TARGET_ARCH-apple-driverkit24.0" -isysroot "$SDK_PATH" -Iinclude -isystem "$SDK_PATH/System/DriverKit/usr/include/c++/v1" -isystem "$SDK_PATH/System/DriverKit/usr/include" -iframework "$SDK_PATH/System/DriverKit/System/Library/Frameworks" -c driverkit/bridge_compile.cpp -o "build/driverkit/bridge-$TARGET_ARCH.o"
done
printf 'DriverKit bridge headers compiled for arm64 and x86_64 (not linked or installed).\n'
