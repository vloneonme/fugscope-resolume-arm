#!/bin/bash
# Linux verification only; output is NOT a macOS plugin.
set -euo pipefail
cd "$(dirname "$0")/.."
: "${GL_TEST_SYSROOT:?Set GL_TEST_SYSROOT to the directory containing usr/include/GL/glew.h and libGLEW}"
mkdir -p build/linux/obj build/linux/frames
pa=vendor/portaudio
for source in "$pa"/src/common/*.c "$pa"/src/os/unix/*.c; do
    gcc -O2 -fPIC -DPA_LITTLE_ENDIAN=1 -DSIZEOF_LONG=8 -DHAVE_NANOSLEEP=1 \
        -I"$pa/include" -I"$pa/src/common" -I"$pa/src/os/unix" \
        -c "$source" -o "build/linux/obj/$(basename "${source%.c}").o"
done
flags=(-std=c++17 -O2 -DGLEW_NO_GLU -I"$GL_TEST_SYSROOT/usr/include" -Ivendor/ffgl/lib)
libs=(-L"$GL_TEST_SYSROOT/usr/lib/x86_64-linux-gnu" -lGLEW -l:libGL.so.1)
g++ "${flags[@]}" -fPIC -Ivendor/portaudio/include -Isrc src/FugScope.cpp src/AudioCapture.cpp \
    vendor/ffgl/lib/FFGLSDK.cpp build/linux/obj/*.o -shared "${libs[@]}" -lpthread -o build/linux/FugScopeArm.so
g++ "${flags[@]}" tests/gl_host.cpp "${libs[@]}" -l:libEGL.so.1 -ldl -o build/linux/gl-host
LD_LIBRARY_PATH="$GL_TEST_SYSROOT/usr/lib/x86_64-linux-gnu${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
    LIBGL_ALWAYS_SOFTWARE=1 MESA_GL_VERSION_OVERRIDE=4.1 MESA_GLSL_VERSION_OVERRIDE=410 \
    build/linux/gl-host "$PWD/build/linux/FugScopeArm.so" "$PWD/build/linux/frames"
