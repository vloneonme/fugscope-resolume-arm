// SPDX-License-Identifier: GPL-3.0-only
#include "ScopeCore.h"
#include <cassert>
#include <iostream>
#include <limits>
#include <thread>

int main() {
    scope::Frame frame{};
    std::vector<float> wave;
    std::vector<scope::Vertex> vertices;
    frame.fill(-.7f); frame[100] = -.2f;
    scope::resample(frame, 1, scope::Arrange::Normal, 1, wave);
    assert(std::fabs(wave[0]+.2f) < 1e-6f); // Must retain signed max semantics.
    frame.fill(0); frame[700] = .8f;
    scope::resample(frame, 2, scope::Arrange::Normal, 2, wave);
    assert(wave[0] == 0 && std::fabs(wave[1]-1.6f)<1e-6f);
    scope::demo(frame, 0);
    for (unsigned w : {0u, 1u, 2u, 3u, 127u, 128u, 1023u, 1024u, 1920u, 3840u}) {
        for (int arrangement = 0; arrangement < 3; ++arrangement) {
            scope::resample(frame, w, scope::Arrange(arrangement), 1, wave);
            assert(wave.size() == w);
            if (arrangement) for (unsigned i=0; i<w; ++i) assert(wave[i] == wave[w-1-i]);
            for (int mode=0; mode<7; ++mode) for (float thickness : {1.f, 5.f, 50.f}) {
                scope::mesh(wave, 1080, scope::Mode(mode), thickness, vertices);
                assert(vertices.size()%3 == 0);
                assert(w == 0 ? vertices.empty() : !vertices.empty());
                for (auto v : vertices) assert(std::isfinite(v.x) && std::isfinite(v.y));
            }
        }
    }
    for (unsigned i=0;i<1024;++i) frame[i]=float(i);
    scope::resample(frame,4,scope::Arrange::Mirror1,1,wave);
    assert(wave[0] == 511 && wave[1] == 1023 && wave[2] == 1023 && wave[3] == 511);
    scope::resample(frame,4,scope::Arrange::Mirror2,1,wave);
    assert(wave[0] == 1023 && wave[1] == 511 && wave[2] == 511 && wave[3] == 1023);
    frame.fill(std::numeric_limits<float>::quiet_NaN());
    scope::resample(frame,10,scope::Arrange::Normal,1,wave);
    for (float v:wave) assert(v==0);
    scope::FrameQueue queue;
    scope::FrameAssembler assembler;
    std::array<float, 2500> input{}; input.fill(.25f);
    assembler.append(input.data(), 2500, queue);
    assert(queue.latest(frame)); for (float v:frame) assert(v==.25f);
    assert(!queue.latest(frame));
    assembler.append(nullptr, 572, queue);
    assert(queue.latest(frame));
    for (unsigned i=0;i<1024;++i) assert(frame[i] == (i<452 ? .25f : 0.f));
    // A full queue must retain ownership and resume after the consumer drains it.
    unsigned accepted=0; frame.fill(.75f);
    for(unsigned i=0;i<20;++i) accepted += queue.push(frame);
    assert(accepted==7 && queue.latest(frame) && queue.push(frame));
    queue.latest(frame);
    // Detect torn frames while producer/consumer wrap the queue many times.
    std::atomic<bool> done{false};
    std::thread producer([&] {
        scope::Frame value{};
        for (int i=1;i<=20000;++i) { value.fill(float(i)); while(!queue.push(value)) std::this_thread::yield(); }
        done.store(true, std::memory_order_release);
    });
    float previous=0;
    while (!done.load(std::memory_order_acquire)) {
        if (queue.latest(frame)) {
            assert(frame[0]>=previous); previous=frame[0];
            for(float v:frame) assert(v==frame[0]);
        }
    }
    producer.join();
    if(queue.latest(frame)) for(float v:frame) assert(v==frame[0]);
    std::cout << "PASS: modes, signed peak resampling, mirrors, edge sizes, NaN, callback chunks, queue concurrency\n";
}
