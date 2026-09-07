// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "ScopeCore.h"
#include <portaudio.h>
#include <string>

class AudioCapture {
public:
    AudioCapture() = default;
    ~AudioCapture() { stop(); }
    AudioCapture(const AudioCapture&) = delete;
    AudioCapture& operator=(const AudioCapture&) = delete;
    bool start(std::string& error);
    void stop();
    bool latest(scope::Frame& frame) { return queue_.latest(frame); }
private:
    static int callback(const void*, void*, unsigned long, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void*);
    PaStream* stream_ = nullptr;
    bool acquired_ = false;
    scope::FrameQueue queue_;
    scope::FrameAssembler assembler_;
};
