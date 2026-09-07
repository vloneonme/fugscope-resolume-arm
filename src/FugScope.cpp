// SPDX-License-Identifier: GPL-3.0-only
// fugScopeGL concept/algorithms: (c) 2015 Alex May, www.bigfug.com.
// FFGL 2 / core-profile port, 2026. Unofficial; see NOTICE.md.
#include "FugScope.h"
#include "ffgl/FFGLLog.h"
#include <cstdio>

static CFFGLPluginInfo PluginInfo(PluginFactory<FugScope>, "FSAR", "FugScope ARM",
    2, 2, 1, 0, FF_SOURCE, "Audio waveform source. Unofficial fugScopeGL port for Apple Silicon.",
    "Based on fugScopeGL by Alex May. GPL-3.0. Modern port 2026.");

namespace {
constexpr const char* vertexShader = R"(#version 410 core
layout(location=0) in vec2 position;
void main() { gl_Position = vec4(position, 0.0, 1.0); }
)";
constexpr const char* fragmentShader = R"(#version 410 core
out vec4 fragColor;
void main() { fragColor = vec4(1.0); }
)";
// Preserve every GL binding/state changed by this plugin, including distinct
// draw/read framebuffers. No deprecated glBegin, client arrays, or wide lines.
struct GLState {
    GLint program, vao, buffer, framebuffer, viewport[4];
    GLfloat clear[4]; GLboolean mask[4];
    static constexpr GLenum caps[] = {GL_BLEND, GL_DEPTH_TEST, GL_CULL_FACE,
        GL_SCISSOR_TEST, GL_STENCIL_TEST, GL_RASTERIZER_DISCARD, GL_COLOR_LOGIC_OP};
    std::array<GLboolean, 7> enabled{};
    GLState() {
        glGetIntegerv(GL_CURRENT_PROGRAM, &program);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &buffer);
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &framebuffer);
        glGetIntegerv(GL_VIEWPORT, viewport);
        glGetFloatv(GL_COLOR_CLEAR_VALUE, clear);
        glGetBooleanv(GL_COLOR_WRITEMASK, mask);
        for (unsigned i=0; i<enabled.size(); ++i) enabled[i] = glIsEnabled(caps[i]);
    }
    ~GLState() {
        glUseProgram(program); glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
        glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
        glClearColor(clear[0], clear[1], clear[2], clear[3]);
        glColorMask(mask[0], mask[1], mask[2], mask[3]);
        for (unsigned i=0; i<enabled.size(); ++i) {
            if (enabled[i]) glEnable(caps[i]); else glDisable(caps[i]);
        }
    }
};
}

FugScope::FugScope() {
    SetMinInputs(0); SetMaxInputs(0);
    SetOptionParamInfo(0, "Config", 7, 0.f);
    for (unsigned i=0; i<7; ++i) SetParamElementInfo(0, i, scope::ModeNames[i], float(i));
    SetOptionParamInfo(1, "Arrange", 3, 0.f);
    for (unsigned i=0; i<3; ++i) SetParamElementInfo(1, i, scope::ArrangeNames[i], float(i));
    SetParamInfo(2, "Scale", FF_TYPE_STANDARD, .1f);
    SetParamInfo(3, "Width", FF_TYPE_STANDARD, 0.f);
    SetParamInfo(4, "Test Signal", FF_TYPE_BOOLEAN, false);
    // Construction is also used by the SDK for metadata: do not open audio here.
}
FFResult FugScope::InitGL(const FFGLViewportStruct* vp) {
    if (!vp || !vp->width || !vp->height) return FF_FAIL;
    GLState saved;
    if (!shader_.Compile(vertexShader, fragmentShader)) return FF_FAIL;
    glGenVertexArrays(1, &vao_); glGenBuffers(1, &vbo_);
    if (!vao_ || !vbo_) { DeInitGL(); return FF_FAIL; }
    glBindVertexArray(vao_); glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(scope::Vertex), nullptr);
    return CFFGLPlugin::InitGL(vp);
}
FFResult FugScope::DeInitGL() {
    audio_.stop(); audioAttempted_ = false;
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (vao_) glDeleteVertexArrays(1, &vao_);
    vbo_ = vao_ = 0;
    shader_.FreeGLResources();
    return FF_SUCCESS;
}
FFResult FugScope::ProcessOpenGL(ProcessOpenGLStruct* pGL) {
    if (!pGL || !vao_ || !shader_.IsReady() || !currentViewport.width || !currentViewport.height)
        return FF_FAIL;
    // Bound allocations for corrupted/unreasonable host dimensions.
    if (currentViewport.width > 32768 || currentViewport.height > 32768) return FF_FAIL;
    const auto now = Clock::now();
    if (params_[4] >= .5f) {
        if (audioAttempted_) { audio_.stop(); audioAttempted_ = false; }
        scope::demo(frame_, std::chrono::duration<double>(now-epoch_).count());
    } else {
        if (!audioAttempted_) {
            frame_.fill(0.f);
            std::string error;
            if (!audio_.start(error)) FFGLLog::LogToHost(("FugScope ARM audio: " + error).c_str());
            audioAttempted_ = true;
        }
        if (audio_.latest(frame_)) lastAudio_ = now;
        else if (now-lastAudio_ > std::chrono::milliseconds(250)) frame_.fill(0.f);
    }
    scope::resample(frame_, currentViewport.width, scope::Arrange(int(params_[1])), params_[2]*10.f, wave_);
    scope::mesh(wave_, currentViewport.height, scope::Mode(int(params_[0])), 1.f+params_[3]*49.f, vertices_);
    GLState saved;
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, pGL->HostFBO);
    glViewport(currentViewport.x, currentViewport.y, currentViewport.width, currentViewport.height);
    for (auto cap : GLState::caps) glDisable(cap);
    // Restrict clearing to the supplied viewport, including non-zero offsets.
    GLint oldScissor[4]; glGetIntegerv(GL_SCISSOR_BOX, oldScissor);
    glEnable(GL_SCISSOR_TEST);
    glScissor(currentViewport.x, currentViewport.y, currentViewport.width, currentViewport.height);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClearColor(0.f, 0.f, 0.f, 0.f); glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(shader_.GetGLID());
    glBindVertexArray(vao_); glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(vertices_.size()*sizeof(scope::Vertex)), vertices_.data(), GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, GLsizei(vertices_.size()));
    glScissor(oldScissor[0], oldScissor[1], oldScissor[2], oldScissor[3]);
    return FF_SUCCESS;
}
FFResult FugScope::SetFloatParameter(unsigned index, float value) {
    if (index >= params_.size() || !std::isfinite(value)) return FF_FAIL;
    if (index < 2) params_[index] = std::round(std::clamp(value, 0.f, index == 0 ? 6.f : 2.f));
    else params_[index] = std::clamp(value, 0.f, 1.f);
    return FF_SUCCESS;
}
float FugScope::GetFloatParameter(unsigned index) { return index < params_.size() ? params_[index] : 0.f; }
char* FugScope::GetParameterDisplay(unsigned index) {
    if (index >= params_.size()) return nullptr;
    if (index == 0) std::snprintf(display_, sizeof(display_), "%s", scope::ModeNames[int(params_[0])]);
    else if (index == 1) std::snprintf(display_, sizeof(display_), "%s", scope::ArrangeNames[int(params_[1])]);
    else if (index == 2) std::snprintf(display_, sizeof(display_), "%.2f", params_[2]*10.f);
    else if (index == 3) std::snprintf(display_, sizeof(display_), "%.1f px", 1.f+params_[3]*49.f);
    else std::snprintf(display_, sizeof(display_), "%s", params_[4] >= .5f ? "On" : "Off");
    return display_;
}
