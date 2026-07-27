#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "$0")/.." && pwd)"
app_dir="$root_dir/dist/AppleGuo Voice.app"
binary="$root_dir/out/virtual_singer"

if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "macOS application bundles can only be created on macOS."
    exit 1
fi
if ! command -v magick >/dev/null; then
    echo "ImageMagick is required to build the application icon: brew install imagemagick"
    exit 1
fi

rm -rf "$app_dir"
mkdir -p "$app_dir/Contents/MacOS" "$app_dir/Contents/Resources" "$app_dir/Contents/Frameworks"
cp "$binary" "$app_dir/Contents/MacOS/AppleGuo Voice"
cp -R "$root_dir/out/assets" "$app_dir/Contents/Resources/assets"

iconset_dir="$(mktemp -d /tmp/appleguo-icon-XXXXXX.iconset)"
trap 'rm -rf "$iconset_dir"' EXIT
create_icon() {
    local pixels="$1"
    local name="$2"
    # Keep the apple artwork's portrait aspect ratio and pad the remainder
    # transparently, instead of stretching it into a square app icon.
    magick "$root_dir/assets/images/apple.png" -resize "${pixels}x${pixels}" \
        -background none -gravity center -extent "${pixels}x${pixels}" "$iconset_dir/$name"
}
create_icon 16 icon_16x16.png
create_icon 32 icon_16x16@2x.png
create_icon 32 icon_32x32.png
create_icon 64 icon_32x32@2x.png
create_icon 128 icon_128x128.png
create_icon 256 icon_128x128@2x.png
create_icon 256 icon_256x256.png
create_icon 512 icon_256x256@2x.png
create_icon 512 icon_512x512.png
create_icon 1024 icon_512x512@2x.png
iconutil -c icns "$iconset_dir" -o "$app_dir/Contents/Resources/AppleGuo.icns"

raylib_path="$(otool -L "$binary" | awk '/libraylib/ { print $1; exit }')"
if [[ -z "$raylib_path" || ! -f "$raylib_path" ]]; then
    echo "Could not locate the raylib dynamic library used by $binary."
    exit 1
fi

raylib_name="$(basename "$raylib_path")"
cp "$raylib_path" "$app_dir/Contents/Frameworks/$raylib_name"
install_name_tool -change "$raylib_path" "@executable_path/../Frameworks/$raylib_name" \
    "$app_dir/Contents/MacOS/AppleGuo Voice"

cat > "$app_dir/Contents/Info.plist" <<'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
  <key>CFBundleDisplayName</key><string>AppleGuo Voice</string>
  <key>CFBundleExecutable</key><string>AppleGuo Voice</string>
  <key>CFBundleIdentifier</key><string>com.appleguo.virtual-singer</string>
  <key>CFBundleIconFile</key><string>AppleGuo.icns</string>
  <key>CFBundleName</key><string>AppleGuo Voice</string>
  <key>CFBundlePackageType</key><string>APPL</string>
  <key>CFBundleShortVersionString</key><string>0.1.0</string>
  <key>LSMinimumSystemVersion</key><string>11.0</string>
</dict></plist>
PLIST

codesign --force --deep --sign - "$app_dir"
ditto -c -k --sequesterRsrc --keepParent "$app_dir" "$root_dir/dist/appleguo-voice-macos.zip"
echo "Created: $app_dir"
echo "Created: $root_dir/dist/appleguo-voice-macos.zip"
