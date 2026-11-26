#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/driverkit
python3 driverkit/bind_firmware.py
clang++ -std=c++17 -Wall -Wextra -Werror -Iinclude -Ibuild/driverkit/firmware -fsanitize=address,undefined -fno-omit-frame-pointer tests/driver_firmware_engine_test.cpp -o build/driverkit/driver-firmware-engine-test
./build/driverkit/driver-firmware-engine-test
