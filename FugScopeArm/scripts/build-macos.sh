#!/bin/bash
set -euo pipefail
project_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$project_dir"
if [[ "$(uname -s)" != Darwin ]]; then
    echo 'Для сборки .bundle нужен Mac с Xcode Command Line Tools.' >&2
    exit 1
fi
if ! xcrun --find clang++ >/dev/null 2>&1; then
    echo 'Установите инструменты: xcode-select --install; затем повторите сборку.' >&2
    exit 1
fi
architecture="${1:-arm64}"
case "$architecture" in
    arm64) arch_flags=(-arch arm64); verify_arches=(arm64) ;;
    universal) arch_flags=(-arch arm64 -arch x86_64); verify_arches=(arm64 x86_64) ;;
    *) echo 'Usage: build-macos.sh [arm64|universal]' >&2; exit 2 ;;
esac
build_dir="$project_dir/build/$architecture"
bundle="$build_dir/FugScopeArm.bundle"
mkdir -p "$build_dir/obj" "$bundle/Contents/MacOS" "$bundle/Contents/Resources/licenses"
sdk_path="$(xcrun --sdk macosx --show-sdk-path)"
common=(-O2 -DNDEBUG -isysroot "$sdk_path" -mmacosx-version-min=11.0 "${arch_flags[@]}")
pa="$project_dir/vendor/portaudio"
pa_sources=("$pa"/src/common/*.c "$pa"/src/os/unix/*.c "$pa"/src/hostapi/coreaudio/*.c)
objects=()
for source in "${pa_sources[@]}"; do
    object="$build_dir/obj/$(basename "${source%.c}").o"
    xcrun clang "${common[@]}" -std=c99 -DPA_USE_COREAUDIO=1 -DPA_LITTLE_ENDIAN=1 \
        -DSIZEOF_LONG=8 -DHAVE_NANOSLEEP=1 -Wno-deprecated-declarations \
        -I"$pa/include" -I"$pa/src/common" -I"$pa/src/os/unix" \
        -c "$source" -o "$object"
    objects+=("$object")
done
xcrun clang++ "${common[@]}" -std=c++17 -Wno-deprecated-declarations \
    -Ivendor/ffgl/lib -Ivendor/portaudio/include -Isrc \
    src/FugScope.cpp src/AudioCapture.cpp vendor/ffgl/lib/FFGLSDK.cpp "${objects[@]}" \
    -bundle -Wl,-exported_symbol,_plugMain -Wl,-exported_symbol,_SetLogCallback \
    -framework OpenGL -framework CoreAudio -framework AudioToolbox -framework AudioUnit \
    -framework CoreFoundation -framework CoreServices -lpthread \
    -o "$bundle/Contents/MacOS/FugScopeArm"
cp resources/Info.plist "$bundle/Contents/Info.plist"
cp LICENSE NOTICE.md "$bundle/Contents/Resources/"
cp licenses/* "$bundle/Contents/Resources/licenses/"
plutil -lint "$bundle/Contents/Info.plist"
xcrun lipo -verify_arch "${verify_arches[@]}" "$bundle/Contents/MacOS/FugScopeArm"
codesign --force --sign - "$bundle"
codesign --verify --strict --verbose=2 "$bundle"
file "$bundle/Contents/MacOS/FugScopeArm"
echo "Готово: $bundle"
