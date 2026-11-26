#!/bin/bash
set -euo pipefail
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$PROJECT_ROOT"
APP="$PROJECT_ROOT/build/Archer Control.app"
SDK="$(xcrun --show-sdk-path)"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources" build/desktop
for ARCH in arm64 x86_64; do
  echo "Building Archer Control ($ARCH)…"
  xcrun swiftc -swift-version 5 -O -parse-as-library -sdk "$SDK" -module-cache-path "build/desktop/cache-$ARCH" -target "$ARCH-apple-macosx14.0" desktop/ArcherControl/*.swift -o "build/desktop/ArcherControl-$ARCH"
done
xcrun lipo -create build/desktop/ArcherControl-arm64 build/desktop/ArcherControl-x86_64 -output "$APP/Contents/MacOS/ArcherControl"
cp desktop/workflows.json "$APP/Contents/Resources/workflows.json"
printf '%s\n' "$PROJECT_ROOT" > "$APP/Contents/Resources/DefaultProject.txt"
xcrun swift -module-cache-path build/desktop/icon-cache desktop/MakeIcon.swift build/desktop/ArcherIcon.iconset
iconutil -c icns build/desktop/ArcherIcon.iconset -o "$APP/Contents/Resources/AppIcon.icns"
cat > "$APP/Contents/Info.plist" <<'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
<key>CFBundleName</key><string>Archer Control</string>
<key>CFBundleDisplayName</key><string>Archer Control</string>
<key>CFBundleIdentifier</key><string>local.archer.control</string>
<key>CFBundleExecutable</key><string>ArcherControl</string>
<key>CFBundlePackageType</key><string>APPL</string>
<key>CFBundleShortVersionString</key><string>0.1.0</string>
<key>CFBundleVersion</key><string>1</string>
<key>CFBundleIconFile</key><string>AppIcon</string>
<key>LSMinimumSystemVersion</key><string>14.0</string>
<key>LSApplicationCategoryType</key><string>public.app-category.developer-tools</string>
<key>NSHighResolutionCapable</key><true/>
<key>NSAppleEventsUsageDescription</key><string>Abre en Terminal los ensayos locales preparados. Las contraseñas se introducen únicamente allí.</string>
</dict></plist>
PLIST
codesign --force --sign - "$APP"
echo "Built: $APP"
