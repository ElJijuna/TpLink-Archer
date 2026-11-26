#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
SDK_PATH=$(xcrun --sdk driverkit --show-sdk-path)
BUILD_DIR=build/driverkit/service
mkdir -p "$BUILD_DIR"
for TARGET_ARCH in arm64 x86_64; do
  ARCH_DIR="$BUILD_DIR/$TARGET_ARCH"
  mkdir -p "$ARCH_DIR"
  set -- -isysroot "$SDK_PATH" -Iinclude -isystem "$SDK_PATH/System/DriverKit/usr/include/c++/v1" -isystem "$SDK_PATH/System/DriverKit/usr/include" -F"$SDK_PATH/System/DriverKit/System/Library/Frameworks"
  xcrun iig --def driverkit/ArcherUSB/ArcherUSB.iig --header "$ARCH_DIR/ArcherUSB.h" --impl "$ARCH_DIR/ArcherUSB.iig.cpp" --deployment-target 24.0 -- "$@" -x c++ -std=c++17 -D__IIG=1 -Wno-deprecated-declarations
  # SDK still requires legacy pure-virtual networking overrides; suppress only their deprecation notices.
  xcrun --sdk driverkit clang++ -target "$TARGET_ARCH-apple-driverkit24.0" -std=c++17 -Wall -Wextra -Werror -Wno-deprecated-declarations "$@" -I"$ARCH_DIR" -c driverkit/ArcherUSB/ArcherUSB.cpp -o "$ARCH_DIR/ArcherUSB.o"
  xcrun --sdk driverkit clang++ -target "$TARGET_ARCH-apple-driverkit24.0" -std=c++17 -Wall -Wextra -Werror -Wno-deprecated-declarations "$@" -I"$ARCH_DIR" -c "$ARCH_DIR/ArcherUSB.iig.cpp" -o "$ARCH_DIR/ArcherUSB.iig.o"
  xcrun --sdk driverkit clang++ -target "$TARGET_ARCH-apple-driverkit24.0" "$@" "$ARCH_DIR/ArcherUSB.o" "$ARCH_DIR/ArcherUSB.iig.o" -framework DriverKit -framework USBDriverKit -framework NetworkingDriverKit -o "$ARCH_DIR/ArcherUSB"
done
BUNDLE="$BUILD_DIR/ArcherUSB.dext"
mkdir -p "$BUNDLE/Contents/MacOS"
xcrun lipo -create "$BUILD_DIR/arm64/ArcherUSB" "$BUILD_DIR/x86_64/ArcherUSB" -output "$BUNDLE/Contents/MacOS/ArcherUSB.new"
mv "$BUNDLE/Contents/MacOS/ArcherUSB.new" "$BUNDLE/Contents/MacOS/ArcherUSB"
cp driverkit/ArcherUSB/Info.plist "$BUNDLE/Contents/Info.plist"
/usr/bin/plutil -lint "$BUNDLE/Contents/Info.plist"
printf 'Built unsigned DriverKit service: %s (not installed).\n' "$BUNDLE"
