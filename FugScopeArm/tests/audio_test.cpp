// SPDX-License-Identifier: GPL-3.0-only
// Exercise production AudioCapture lifecycle with an explicit fake driver.
#include "AudioCapture.h"
#include <cassert>
#include <iostream>

namespace {
int initialized=0, terminated=0, opened=0, closed=0, stopped=0;
bool noDevice=false, failOpen=false, failStart=false, failInitialize=false, fallback=false;
double openedRate=0;
struct Stream { PaStreamCallback* callback; void* user; };
PaDeviceInfo device{};
}
extern "C" {
PaError Pa_Initialize() { ++initialized; return failInitialize ? paInternalError : paNoError; }
PaError Pa_Terminate() { ++terminated; return paNoError; }
PaDeviceIndex Pa_GetDefaultInputDevice() { return noDevice ? paNoDevice : 0; }
const PaDeviceInfo* Pa_GetDeviceInfo(PaDeviceIndex) { device.maxInputChannels=2; device.defaultSampleRate=48000; return &device; }
PaError Pa_IsFormatSupported(const PaStreamParameters*, const PaStreamParameters*, double) {
    return fallback ? paInvalidSampleRate : paFormatIsSupported;
}
const char* Pa_GetErrorText(PaError) { return "fake audio error"; }
PaError Pa_OpenStream(PaStream** stream, const PaStreamParameters* input, const PaStreamParameters* output,
    double rate, unsigned long frames, PaStreamFlags, PaStreamCallback* callback, void* user) {
    assert(input->channelCount==1 && input->sampleFormat==paFloat32 && !output && frames==1024);
    openedRate=rate;
    if(failOpen) return paDeviceUnavailable;
    *stream=new Stream{callback,user}; ++opened; return paNoError;
}
PaError Pa_StartStream(PaStream* stream) {
    if(failStart) return paDeviceUnavailable;
    auto* s=static_cast<Stream*>(stream);
    scope::Frame samples{}; samples.fill(.375f);
    assert(s->callback(samples.data(),nullptr,samples.size(),nullptr,0,s->user)==paContinue);
    return paNoError;
}
PaError Pa_StopStream(PaStream*) { ++stopped; return paNoError; }
PaError Pa_CloseStream(PaStream* stream) { ++closed; delete static_cast<Stream*>(stream); return paNoError; }
}
int main() {
    std::string error;
    {
        AudioCapture a,b;
        assert(a.start(error) && a.start(error)); // Idempotent.
        assert(initialized==1 && opened==1 && openedRate==44100);
        assert(b.start(error) && initialized==1 && opened==2);
        scope::Frame frame{}; assert(a.latest(frame));for(float v:frame) assert(v==.375f);
        a.stop();assert(terminated==0 && stopped==1);
        a.stop();assert(terminated==0 && stopped==1);
        b.stop();assert(terminated==1 && closed==2);
    }
    const int closes=closed;
    {
        AudioCapture capture;
        noDevice=true;assert(!capture.start(error));noDevice=false;
        failOpen=true;assert(!capture.start(error));failOpen=false;
        failStart=true;assert(!capture.start(error));failStart=false;
        assert(closed==closes+1); // The opened-but-not-started stream is released.
        fallback=true;assert(capture.start(error));assert(openedRate==48000);fallback=false;
    }
    assert(opened==closed && initialized==terminated);
    failInitialize=true;
    { AudioCapture capture; assert(!capture.start(error)); }
    assert(initialized==terminated+1); // Failed init must not be terminated.
    std::cout<<"PASS: audio lifecycle, concurrent instances, callback delivery, fallback rate, failed initialization/open/start, cleanup\n";
}
