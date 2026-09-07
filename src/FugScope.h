// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "ffgl/FFGLPluginSDK.h"
#include "ffglex/FFGLShader.h"
#include "AudioCapture.h"
#include <chrono>

class FugScope final : public CFFGLPlugin {
public:
    FugScope();
    FFResult InitGL(const FFGLViewportStruct*) override;
    FFResult DeInitGL() override;
    FFResult ProcessOpenGL(ProcessOpenGLStruct*) override;
    FFResult SetFloatParameter(unsigned, float) override;
    float GetFloatParameter(unsigned) override;
    char* GetParameterDisplay(unsigned) override;
private:
    std::array<float, 5> params_{{0.f, 0.f, .1f, 0.f, 0.f}};
    AudioCapture audio_;
    bool audioAttempted_ = false;
    scope::Frame frame_{};
    std::vector<float> wave_;
    std::vector<scope::Vertex> vertices_;
    ffglex::FFGLShader shader_;
    GLuint vao_ = 0, vbo_ = 0;
    char display_[32]{};
    using Clock = std::chrono::steady_clock;
    Clock::time_point lastAudio_ = Clock::now(), epoch_ = Clock::now();
};
