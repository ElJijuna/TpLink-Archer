#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/driverkit
python3 - <<'PY'
from pathlib import Path
import json
fixture=json.loads(Path('tests/fixtures/driver-boot-controls-stage194.json').read_text())
lines=['#include <array>','#include <cstdint>']
for key,name in [('cold','cold_fixture'),('synthetic_running','running_fixture')]:
    values=fixture[key]
    assert len(values)==6 and all(isinstance(x,int) and 0<=x<=0xffffffff for x in values)
    lines.append('inline constexpr std::array<std::uint32_t,6> '+name+'{'+','.join(str(x)+'u' for x in values)+'};')
Path('build/driverkit/boot-control-fixtures.hpp').write_text('\n'.join(lines)+'\n')
PY
clang++ -std=c++17 -Wall -Wextra -Werror -Iinclude -Ibuild/driverkit -fsanitize=address,undefined -fno-omit-frame-pointer tests/driver_boot_controls_test.cpp -o build/driverkit/driver-boot-controls-test
./build/driverkit/driver-boot-controls-test
