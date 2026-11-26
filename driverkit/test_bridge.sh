#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/driverkit
clang++ -std=c++17 -Wall -Wextra -Werror -Iinclude -fsanitize=address,undefined -fno-omit-frame-pointer tests/ethernet_bridge_test.cpp -o build/driverkit/ethernet-bridge-test
./build/driverkit/ethernet-bridge-test
clang++ -std=c++17 -Wall -Wextra -Werror -Iinclude -arch arm64 -arch x86_64 -c tests/ethernet_bridge_test.cpp -o build/driverkit/ethernet-bridge-test-universal.o
