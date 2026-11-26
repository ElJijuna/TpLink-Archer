#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/driverkit
clang++ -std=c++17 -Wall -Wextra -Werror -Iinclude -fsanitize=address,undefined -fno-omit-frame-pointer tests/usb_transport_state_test.cpp -o build/driverkit/usb-transport-state-test
./build/driverkit/usb-transport-state-test
