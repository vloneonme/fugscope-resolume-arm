#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build/tests
compiler="${CXX:-c++}"
"$compiler" -std=c++17 -O1 -g -Wall -Wextra -Werror -pthread \
    -fsanitize=address,undefined -fno-omit-frame-pointer -Isrc tests/core_test.cpp -o build/tests/core-test
build/tests/core-test
"$compiler" -std=c++17 -O1 -g -Wall -Wextra -Werror -pthread \
    -fsanitize=address,undefined -fno-omit-frame-pointer -Isrc -Ivendor/portaudio/include \
    tests/audio_test.cpp src/AudioCapture.cpp -o build/tests/audio-test
build/tests/audio-test
