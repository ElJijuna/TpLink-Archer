#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/driverkit
clang++ -std=c++17 -Wall -Wextra -Werror -Iinclude -fsanitize=address,undefined -fno-omit-frame-pointer tests/driver_power_windows_test.cpp -o build/driverkit/driver-power-windows-test
./build/driverkit/driver-power-windows-test
