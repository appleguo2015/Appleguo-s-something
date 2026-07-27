#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "$0")/.." && pwd)"
package_dir="$root_dir/package"

rm -rf "$package_dir"
mkdir -p "$package_dir/assets" "$package_dir/icons/hicolor/512x512/apps"
cp "$root_dir/out/virtual_singer" "$package_dir/virtual_singer"
cp -R "$root_dir/out/assets/." "$package_dir/assets/"
cp "$root_dir/assets/images/apple.png" "$package_dir/icons/hicolor/512x512/apps/appleguo-voice.png"
cat > "$package_dir/appleguo-voice.desktop" <<'DESKTOP'
[Desktop Entry]
Type=Application
Name=AppleGuo Voice
Comment=Offline phoneme character
Exec=virtual_singer
Icon=appleguo-voice
Categories=AudioVideo;Audio;
Terminal=false
DESKTOP
