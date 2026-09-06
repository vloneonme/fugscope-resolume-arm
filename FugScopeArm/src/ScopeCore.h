// SPDX-License-Identifier: GPL-3.0-only
// Derived from fugScopeGL by Alex May (c) 2015. Modern port, 2026.
#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <vector>

namespace scope {
constexpr std::size_t FrameSize = 1024;
using Frame = std::array<float, FrameSize>;
enum class Mode { Line, Points, Fill, PointsMirror, LineMirror, FillBottom, FillTop };
enum class Arrange { Normal, Mirror1, Mirror2 };
constexpr const char* ModeNames[] = {"Line", "Points", "Fill", "Points Mirror", "Line Mirror", "Fill Bottom", "Fill Top"};
constexpr const char* ArrangeNames[] = {"Normal", "Mirror1", "Mirror2"};
inline float finite(float v) { return std::isfinite(v) ? v : 0.f; }

// One audio producer / one renderer. Never overwrite a slot owned by the reader.
// Full queues drop incoming frames; neither thread waits or allocates memory.
class FrameQueue {
    static constexpr unsigned Slots = 8;
    std::array<Frame, Slots> frames_{};
    alignas(64) std::atomic<unsigned> write_{0};
    alignas(64) std::atomic<unsigned> read_{0};
public:
    static_assert(std::atomic<unsigned>::is_always_lock_free, "Audio callback needs lock-free indices");
    bool push(const Frame& frame) noexcept {
        const auto w = write_.load(std::memory_order_relaxed);
        const auto next = (w + 1) % Slots;
        if (next == read_.load(std::memory_order_acquire)) return false;
        frames_[w] = frame;
        write_.store(next, std::memory_order_release);
        return true;
    }
    bool latest(Frame& frame) noexcept {
        // Bound work even if a producer runs continuously during this call.
        const auto end = write_.load(std::memory_order_acquire);
        auto r = read_.load(std::memory_order_relaxed);
        if (r == end) return false;
        while (r != end) {
            frame = frames_[r];
            r = (r + 1) % Slots;
            read_.store(r, std::memory_order_release);
        }
        return true;
    }
};

// Handles callback sizes both smaller and larger than 1024, and null input.
class FrameAssembler {
    Frame pending_{};
    std::size_t used_ = 0;
public:
    void append(const float* data, std::size_t count, FrameQueue& queue) noexcept {
        for (std::size_t i = 0; i < count; ++i) {
            pending_[used_++] = data ? finite(data[i]) : 0.f;
            if (used_ == FrameSize) { queue.push(pending_); used_ = 0; }
        }
    }
};

// Preserve the original signed MAX reduction (not max(abs(sample))).
inline float bucket(const Frame& frame, std::size_t i, std::size_t count) {
    const auto first = std::min(FrameSize - 1, i * FrameSize / count);
    const auto end = std::min(FrameSize, std::max(first + 1, (i + 1) * FrameSize / count));
    float v = finite(frame[first]);
    for (auto j = first + 1; j < end; ++j) v = std::max(v, finite(frame[j]));
    return v;
}
inline void resample(const Frame& frame, std::size_t width, Arrange arrange, float scale,
                     std::vector<float>& result) {
    result.resize(width);
    if (!width) return;
    scale = std::clamp(finite(scale), 0.f, 10.f);
    if (arrange == Arrange::Normal) {
        for (std::size_t i = 0; i < width; ++i) result[i] = bucket(frame, i, width) * scale;
    } else {
        const auto half = (width + 1) / 2;
        for (std::size_t i = 0; i < half; ++i) {
            const auto index = arrange == Arrange::Mirror1 ? i : half - 1 - i;
            result[i] = result[width - 1 - i] = bucket(frame, index, half) * scale;
        }
    }
}

struct Vertex { float x, y; };
inline void quad(std::vector<Vertex>& out, Vertex a, Vertex b, Vertex c, Vertex d) {
    out.insert(out.end(), {a, b, c, a, c, d});
}
inline void mesh(const std::vector<float>& wave, unsigned height, Mode mode, float thickness,
                 std::vector<Vertex>& out) {
    out.clear();
    if (wave.empty() || !height) return;
    const auto w = wave.size();
    const float px = 2.f / float(w), py = 2.f / float(height);
    thickness = std::clamp(finite(thickness), 1.f, 50.f);
    const float rx = thickness * px * .5f, ry = thickness * py * .5f;
    auto x = [w](std::size_t i) { return w == 1 ? 0.f : -1.f + 2.f * float(i) / float(w - 1); };
    out.reserve(w * 12);
    auto dot = [&](float xx, float yy) {
        quad(out, {xx-rx, yy-ry}, {xx+rx, yy-ry}, {xx+rx, yy+ry}, {xx-rx, yy+ry});
    };
    auto segment = [&](float x1, float y1, float x2, float y2) {
        // Triangles give a reliable 1..50 pixel width in Apple's core profile.
        const float dx = (x2-x1) / px, dy = (y2-y1) / py;
        const float length = std::hypot(dx, dy);
        if (length < 1e-6f) { dot(x1, y1); return; }
        const float nx = -dy / length * rx, ny = dx / length * ry;
        quad(out, {x1+nx,y1+ny}, {x1-nx,y1-ny}, {x2-nx,y2-ny}, {x2+nx,y2+ny});
    };
    const bool points = mode == Mode::Points || mode == Mode::PointsMirror;
    const bool mirrored = mode == Mode::PointsMirror || mode == Mode::LineMirror;
    const bool fill = mode == Mode::Fill || mode == Mode::FillBottom || mode == Mode::FillTop;
    if (fill) {
        auto bounds = [mode](float y) -> Vertex {
            if (mode == Mode::Fill) return {-std::fabs(y), std::fabs(y)};
            if (mode == Mode::FillBottom) return {-1.f, y};
            return {y, 1.f};
        };
        for (std::size_t i = 0; i < (w > 1 ? w-1 : 1); ++i) {
            const auto a = bounds(wave[i]), b = bounds(wave[w > 1 ? i+1 : i]);
            const float left = w > 1 ? x(i) : -1.f, right = w > 1 ? x(i+1) : 1.f;
            quad(out, {left,a.x}, {right,b.x}, {right,b.y}, {left,a.y});
        }
    } else {
        for (int side = 0; side < (mirrored ? 2 : 1); ++side) {
            auto y = [&](std::size_t i) { return mirrored ? (side ? -1.f : 1.f)*std::fabs(wave[i]) : wave[i]; };
            for (std::size_t i = 0; i < w; ++i) {
                if (points || w == 1) dot(x(i), y(i));
                else if (i + 1 < w) segment(x(i), y(i), x(i+1), y(i+1));
            }
        }
    }
}
inline void demo(Frame& frame, double seconds) {
    const double phase = std::fmod(seconds, 1000.0) * 2.0;
    for (std::size_t i = 0; i < FrameSize; ++i) {
        const double t = double(i) / double(FrameSize);
        frame[i] = float(.48 * std::sin(6.283185307179586 * 3.0 * t + phase)
                       + .17 * std::sin(6.283185307179586 * 11.0 * t - phase));
    }
}
} // namespace scope
