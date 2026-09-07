// SPDX-License-Identifier: GPL-3.0-only
// Linux-only offscreen FFGL host: verifies real GLSL/ABI, not macOS integration.
#include "ffgl/FFGL.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <cassert>
#include <cstring>
#include <dlfcn.h>
#include <fstream>
#include <iostream>
#include <vector>

int main(int argc, char** argv) {
    assert(argc == 3);
    auto display = eglGetPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, nullptr);
    EGLint major, minor; assert(eglInitialize(display, &major, &minor));
    assert(eglBindAPI(EGL_OPENGL_API));
    const EGLint configAttrs[] = {EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8, EGL_NONE};
    EGLConfig config; EGLint count;
    assert(eglChooseConfig(display, configAttrs, &config, 1, &count) && count);
    const EGLint surfaceAttrs[] = {EGL_WIDTH, 672, EGL_HEIGHT, 392, EGL_NONE};
    auto surface = eglCreatePbufferSurface(display, config, surfaceAttrs); assert(surface != EGL_NO_SURFACE);
    const EGLint contextAttrs[] = {EGL_CONTEXT_MAJOR_VERSION, 4, EGL_CONTEXT_MINOR_VERSION, 1,
        EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT, EGL_NONE};
    auto context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttrs); assert(context != EGL_NO_CONTEXT);
    assert(eglMakeCurrent(display, surface, surface, context));
    glewExperimental = GL_TRUE; glewInit(); // GLEW's GLX check can fail on EGL; function loading still succeeds.
    assert(glGenVertexArrays && glCreateShader);
    while(glGetError() != GL_NO_ERROR) {}
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << "; OpenGL: " << glGetString(GL_VERSION) << '\n';
    void* module = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if(!module) { std::cerr << dlerror() << '\n'; return 1; }
    auto entry = reinterpret_cast<FF_Main_FuncPtr>(dlsym(module, "plugMain")); assert(entry);
    auto call = [&](unsigned opcode, FFMixed input = {}, void* instance = nullptr) { return entry(opcode,input,instance); };
    assert(call(FF_INITIALISE_V2).UIntValue == FF_SUCCESS);
    auto* info = static_cast<PluginInfoStruct*>(call(FF_GET_INFO).PointerValue);
    assert(info && info->APIMajorVersion == 2 && info->PluginType == FF_SOURCE);
    assert(std::memcmp(info->PluginUniqueID,"FSAR",4) == 0);
    assert(call(FF_GET_NUM_PARAMETERS).UIntValue == 5);
    FFMixed index{}; index.UIntValue=0;
    assert(call(FF_GET_NUM_PARAMETER_ELEMENTS,index).UIntValue == 7);
    index.UIntValue=1;
    assert(call(FF_GET_NUM_PARAMETER_ELEMENTS,index).UIntValue == 3);
    GLuint fbo, texture;
    glGenTextures(1,&texture); glBindTexture(GL_TEXTURE_2D,texture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,672,392,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
    glGenFramebuffers(1,&fbo); glBindFramebuffer(GL_FRAMEBUFFER,fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,texture,0);
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
    glBindTexture(GL_TEXTURE_2D,0);
    FFGLViewportStruct vp{8,9,641,361}; FFMixed pointer{}; pointer.PointerValue=&vp;
    auto instance = call(FF_INSTANTIATE_GL,pointer).PointerValue;
    assert(instance && instance != reinterpret_cast<void*>(uintptr_t(FF_FAIL)));
    auto set = [&](unsigned parameter, float value) {
        SetParameterStruct param{}; param.ParameterNumber=parameter;
        std::memcpy(&param.NewParameterValue.UIntValue,&value,4);
        FFMixed p{};p.PointerValue=&param;
        assert(call(FF_SET_PARAMETER,p,instance).UIntValue == FF_SUCCESS);
    };
    set(4,1.f); // Rendering tests use demo, never microphone capture.
    set(3,4.f/49.f);
    ProcessOpenGLStruct process{0,nullptr,fbo}; pointer.PointerValue=&process;
    std::vector<unsigned char> pixels(672*392*4);
    for(unsigned arrangement=0;arrangement<3;++arrangement) {
        set(1,float(arrangement));
        for(unsigned mode=0;mode<7;++mode) {
            set(0,float(mode));
            glBindFramebuffer(GL_FRAMEBUFFER,fbo);
            glDisable(GL_SCISSOR_TEST); glColorMask(1,1,1,1);
            glClearColor(.125f,.25f,.5f,1.f); glClear(GL_COLOR_BUFFER_BIT);
            // Non-default host state must survive the plugin call.
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER,0);
            glViewport(1,2,33,44); glEnable(GL_BLEND); glEnable(GL_DEPTH_TEST);
            glEnable(GL_CULL_FACE); glEnable(GL_SCISSOR_TEST); glScissor(2,3,4,5);
            glColorMask(0,1,0,1);
            assert(call(FF_PROCESS_OPENGL,pointer,instance).UIntValue==FF_SUCCESS);
            assert(glGetError()==GL_NO_ERROR);
            GLint restored[4]; glGetIntegerv(GL_VIEWPORT,restored);
            assert(restored[0]==1 && restored[1]==2 && restored[2]==33 && restored[3]==44);
            glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,restored); assert(restored[0]==0);
            glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,restored); assert(restored[0]==int(fbo));
            glGetIntegerv(GL_SCISSOR_BOX,restored); assert(restored[0]==2 && restored[1]==3 && restored[2]==4 && restored[3]==5);
            assert(glIsEnabled(GL_BLEND) && glIsEnabled(GL_DEPTH_TEST) && glIsEnabled(GL_CULL_FACE) && glIsEnabled(GL_SCISSOR_TEST));
            GLboolean mask[4];glGetBooleanv(GL_COLOR_WRITEMASK,mask);assert(!mask[0] && mask[1] && !mask[2] && mask[3]);
            glReadPixels(0,0,672,392,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
            assert(pixels[0]==32 && pixels[1]==64 && pixels[2]==128 && pixels[3]==255); // Outside vp unchanged.
            std::size_t lit=0, clear=0;
            for(unsigned y=vp.y;y<vp.y+vp.height;++y) for(unsigned x=vp.x;x<vp.x+vp.width;++x) {
                const auto p=&pixels[(y*672+x)*4];
                if(p[3]==255) { ++lit; assert(p[0]==255 && p[1]==255 && p[2]==255); }
                else { assert(p[0]==0 && p[1]==0 && p[2]==0 && p[3]==0); ++clear; }
            }
            assert(lit>500 && clear>500);
            std::cout << "PASS mode="<<mode<<" arrange="<<arrangement<<" visible="<<lit<<" transparent="<<clear<<'\n';
            const auto name=std::string(argv[2])+"/mode-"+std::to_string(mode)+"-arrange-"+std::to_string(arrangement)+".ppm";
            std::ofstream image(name,std::ios::binary);
            image<<"P6\n"<<vp.width<<" "<<vp.height<<"\n255\n";
            for(int y=int(vp.y+vp.height)-1;y>=int(vp.y);--y)
                for(unsigned x=vp.x;x<vp.x+vp.width;++x) image.write(reinterpret_cast<char*>(&pixels[(y*672+x)*4]),3);
        }
    }
    // Resize edge cases via the actual FFGL dispatcher.
    for(unsigned size : {1u,2u,3u,127u}) {
        vp={0,0,size,size};pointer.PointerValue=&vp;
        assert(call(FF_RESIZE,pointer,instance).UIntValue==FF_SUCCESS);
        pointer.PointerValue=&process;
        assert(call(FF_PROCESS_OPENGL,pointer,instance).UIntValue==FF_SUCCESS);
        assert(glGetError()==GL_NO_ERROR);
    }
    // This test build of PortAudio has no hardware backend. Failure must leave
    // a usable source and render silence, rather than reject the instance.
    set(4,0);pointer.PointerValue=&process;
    assert(call(FF_PROCESS_OPENGL,pointer,instance).UIntValue==FF_SUCCESS);
    assert(call(FF_DEINSTANTIATE_GL,{},instance).UIntValue==FF_SUCCESS);
    assert(call(FF_DEINITIALISE).UIntValue==FF_SUCCESS);
    glDeleteFramebuffers(1,&fbo);glDeleteTextures(1,&texture);
    dlclose(module);
    eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
    eglDestroyContext(display,context);eglDestroySurface(display,surface);eglTerminate(display);
    std::cout<<"PASS: FFGL metadata, 21 rendered combinations, viewport boundaries, GL state, resize, missing audio, teardown\n";
}
