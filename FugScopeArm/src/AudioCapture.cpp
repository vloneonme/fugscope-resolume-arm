// SPDX-License-Identifier: GPL-3.0-only
#include "AudioCapture.h"
#include <mutex>

namespace { std::mutex runtimeMutex; unsigned runtimeUsers = 0; }

bool AudioCapture::start(std::string& error) {
    if (stream_) return true;
    scope::Frame discarded{};
    queue_.latest(discarded);
    assembler_ = scope::FrameAssembler{};
    std::lock_guard<std::mutex> lock(runtimeMutex);
    auto release = [&] {
        if (stream_) { Pa_CloseStream(stream_); stream_ = nullptr; }
        if (acquired_ && --runtimeUsers == 0) Pa_Terminate();
        acquired_ = false;
    };
    if (!runtimeUsers) {
        const auto status = Pa_Initialize();
        if (status != paNoError) { error = Pa_GetErrorText(status); return false; }
    }
    ++runtimeUsers;
    acquired_ = true;
    const auto device = Pa_GetDefaultInputDevice();
    const auto* info = device == paNoDevice ? nullptr : Pa_GetDeviceInfo(device);
    if (!info || info->maxInputChannels < 1) {
        error = "No default input device. Select an input in macOS Sound settings.";
        release(); return false;
    }
    PaStreamParameters input{};
    input.device = device;
    input.channelCount = 1; // First channel, as in the original fugScopeGL.
    input.sampleFormat = paFloat32;
    input.suggestedLatency = info->defaultLowInputLatency;
    double rate = 44100.0;
    if (Pa_IsFormatSupported(&input, nullptr, rate) != paFormatIsSupported) rate = info->defaultSampleRate;
    auto status = Pa_OpenStream(&stream_, &input, nullptr, rate, scope::FrameSize,
                                paNoFlag, callback, this);
    if (status == paNoError) status = Pa_StartStream(stream_);
    if (status != paNoError) { error = Pa_GetErrorText(status); release(); return false; }
    return true;
}
void AudioCapture::stop() {
    std::lock_guard<std::mutex> lock(runtimeMutex);
    if (stream_) {
        Pa_StopStream(stream_); // Joins callbacks before any queue storage is destroyed.
        Pa_CloseStream(stream_);
        stream_ = nullptr;
    }
    if (acquired_ && --runtimeUsers == 0) Pa_Terminate();
    acquired_ = false;
}
int AudioCapture::callback(const void* input, void*, unsigned long count,
                           const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void* user) {
    auto& self = *static_cast<AudioCapture*>(user);
    self.assembler_.append(static_cast<const float*>(input), count, self.queue_);
    return paContinue;
}
