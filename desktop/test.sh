#!/bin/bash
set -euo pipefail
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$PROJECT_ROOT"
mkdir -p build/desktop
xcrun swiftc -swift-version 5 -O -parse-as-library -module-cache-path build/desktop/test-cache desktop/ArcherControl/Core.swift desktop/Tests/CoreTests.swift -o build/desktop/CoreTests
build/desktop/CoreTests "$PROJECT_ROOT"
