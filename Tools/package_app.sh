#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
CONFIGURATION="${1:-debug}"
APP_NAME="TopiPal"
PRODUCT_DIR="$ROOT_DIR/.build/arm64-apple-macosx/$CONFIGURATION"
EXECUTABLE="$PRODUCT_DIR/$APP_NAME"
RESOURCE_BUNDLE="$PRODUCT_DIR/TopiPal_TopiPal.bundle"
APP_DIR="$ROOT_DIR/.build/$APP_NAME.app"
APP_ICON_SOURCE="$ROOT_DIR/Sources/TopiPal/Resources/App/AppIcon.png"
APP_ICON_SET="$ROOT_DIR/.build/$APP_NAME.iconset"
APP_ICON_FILE="$ROOT_DIR/.build/TopiPal.icns"
INSTALL_APP_DIR=""
CODE_SIGN_IDENTITY="${CODE_SIGN_IDENTITY:-}"

has_codesign_identity() {
    local identity="$1"
    security find-identity -v -p codesigning 2>/dev/null \
        | grep -F "\"${identity}\"" \
        | grep -v "(" >/dev/null
}

if [[ -z "$CODE_SIGN_IDENTITY" ]]; then
    if has_codesign_identity "TopiPal Local Code Signing"; then
        CODE_SIGN_IDENTITY="TopiPal Local Code Signing"
    else
        CODE_SIGN_IDENTITY="-"
    fi
elif [[ "$CODE_SIGN_IDENTITY" != "-" ]] && ! has_codesign_identity "$CODE_SIGN_IDENTITY"; then
    echo "warning: code signing identity '$CODE_SIGN_IDENTITY' not found; falling back to ad-hoc signing" >&2
    CODE_SIGN_IDENTITY="-"
fi

if [[ "${2:-}" == "--install" ]]; then
    INSTALL_APP_DIR="${HOME}/Applications/${APP_NAME}.app"
elif [[ "${2:-}" == "--install-system" ]]; then
    INSTALL_APP_DIR="/Applications/${APP_NAME}.app"
fi

PROJECT_CACHE_DIR="$(basename "$ROOT_DIR")"
export CLANG_MODULE_CACHE_PATH="$ROOT_DIR/.clang-cache/$PROJECT_CACHE_DIR/ModuleCache"
export SWIFTPM_MODULECACHE_OVERRIDE="$ROOT_DIR/.build-cache/$PROJECT_CACHE_DIR/ModuleCache"
mkdir -p "$CLANG_MODULE_CACHE_PATH" "$SWIFTPM_MODULECACHE_OVERRIDE"

if [[ "$CONFIGURATION" == "release" ]]; then
    swift build --configuration release
else
    swift build
fi

rm -rf "$APP_DIR"
mkdir -p "$APP_DIR/Contents/MacOS" "$APP_DIR/Contents/Resources"

cp "$EXECUTABLE" "$APP_DIR/Contents/MacOS/$APP_NAME"
cp -R "$RESOURCE_BUNDLE" "$APP_DIR/Contents/Resources/"

if [[ -f "$APP_ICON_SOURCE" ]]; then
    rm -rf "$APP_ICON_SET"
    mkdir -p "$APP_ICON_SET"
    for size in 16 32 128 256 512; do
        sips -z "$size" "$size" "$APP_ICON_SOURCE" --out "$APP_ICON_SET/icon_${size}x${size}.png" >/dev/null
        retina_size=$((size * 2))
        sips -z "$retina_size" "$retina_size" "$APP_ICON_SOURCE" --out "$APP_ICON_SET/icon_${size}x${size}@2x.png" >/dev/null
    done
    iconutil -c icns "$APP_ICON_SET" -o "$APP_ICON_FILE"
    cp "$APP_ICON_FILE" "$APP_DIR/Contents/Resources/TopiPal.icns"
fi

xattr -cr "$APP_DIR"

cat > "$APP_DIR/Contents/Info.plist" <<'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleDevelopmentRegion</key>
    <string>zh_CN</string>
    <key>CFBundleExecutable</key>
    <string>TopiPal</string>
    <key>CFBundleIdentifier</key>
    <string>com.lxhcool.topipal</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundleName</key>
    <string>TopiPal</string>
    <key>CFBundleDisplayName</key>
    <string>TopiPal</string>
    <key>CFBundleIconFile</key>
    <string>TopiPal</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleShortVersionString</key>
    <string>0.1.0</string>
    <key>CFBundleVersion</key>
    <string>1</string>
    <key>LSMinimumSystemVersion</key>
    <string>14.0</string>
    <key>NSPrincipalClass</key>
    <string>NSApplication</string>
    <key>NSAppTransportSecurity</key>
    <dict>
        <key>NSAllowsArbitraryLoads</key>
        <false/>
        <key>NSExceptionDomains</key>
        <dict>
            <key>api.deepseek.com</key>
            <dict>
                <key>NSIncludesSubdomains</key>
                <true/>
                <key>NSTemporaryExceptionAllowsInsecureHTTPLoads</key>
                <false/>
                <key>NSExceptionRequiresForwardSecrecy</key>
                <false/>
                <key>NSExceptionMinimumTLSVersion</key>
                <string>TLSv1.2</string>
            </dict>
        </dict>
    </dict>
    <key>NSHumanReadableCopyright</key>
    <string>Copyright © 2026</string>
</dict>
</plist>
PLIST

codesign --force --deep --sign "$CODE_SIGN_IDENTITY" "$APP_DIR" >/dev/null

if [[ -n "$INSTALL_APP_DIR" ]]; then
    mkdir -p "$(dirname "$INSTALL_APP_DIR")"
    rm -rf "$INSTALL_APP_DIR"
    cp -R "$APP_DIR" "$INSTALL_APP_DIR"
    codesign --force --deep --sign "$CODE_SIGN_IDENTITY" "$INSTALL_APP_DIR" >/dev/null
    echo "$INSTALL_APP_DIR"
else
    echo "$APP_DIR"
fi
