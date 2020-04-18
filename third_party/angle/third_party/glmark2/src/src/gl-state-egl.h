//
// Copyright © 2012 Linaro Limited
//
// This file is part of the glmark2 OpenGL (ES) 2.0 benchmark.
//
// glmark2 is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// glmark2 is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// glmark2.  If not, see <http://www.gnu.org/licenses/>.
//
// Authors:
//  Jesse Barker
//
#ifndef GLMARK2_GL_STATE_EGL_H_
#define GLMARK2_GL_STATE_EGL_H_

#include <vector>
#include <glad/egl.h>
#include "gl-state.h"
#include "gl-visual-config.h"
#include "shared-library.h"

class EglConfig
{
    EGLConfig handle_;
    // Color buffer attributes.
    EGLint bufferSize_;
    EGLint redSize_;
    EGLint greenSize_;
    EGLint blueSize_;
    EGLint luminanceSize_;
    EGLint alphaSize_;
    EGLint alphaMaskSize_;
    bool bindTexRGB_;
    bool bindTexRGBA_;
    EGLint bufferType_;
    // Base config attributes
    EGLint caveat_;
    EGLint configID_;
    EGLint conformant_;
    // Depth buffer
    EGLint depthSize_;
    // Framebuffer level
    EGLint level_;
    // Pbuffers
    EGLint pbufferWidth_;
    EGLint pbufferHeight_;
    EGLint pbufferPixels_;
    // Swap interval
    EGLint minSwapInterval_;
    EGLint maxSwapInterval_;
    // Native window system attributes.
    EGLint nativeID_;
    EGLint nativeType_;
    bool nativeRenderable_;
    // Multisample support
    EGLint sampleBuffers_;
    EGLint samples_;
    // Stencil buffer
    EGLint stencilSize_;
    EGLint surfaceType_;
    // Transparency
    EGLint xparentType_; // Should be RGB or NONE
    EGLint xparentRedValue_;
    EGLint xparentGreenValue_;
    EGLint xparentBlueValue_;

public:
    EglConfig() :
        handle_(0),
        bufferSize_(0),
        redSize_(0),
        greenSize_(0),
        blueSize_(0),
        luminanceSize_(0),
        alphaSize_(0),
        alphaMaskSize_(0),
        bindTexRGB_(false),
        bindTexRGBA_(false),
        bufferType_(EGL_RGB_BUFFER),
        caveat_(0),
        configID_(0),
        conformant_(0),
        depthSize_(0),
        level_(0),
        pbufferWidth_(0),
        pbufferHeight_(0),
        pbufferPixels_(0),
        minSwapInterval_(0),
        maxSwapInterval_(0),
        nativeID_(0),
        nativeType_(0),
        nativeRenderable_(false),
        sampleBuffers_(0),
        samples_(0),
        stencilSize_(0),
        surfaceType_(0),
        xparentType_(0),
        xparentRedValue_(0),
        xparentGreenValue_(0),
        xparentBlueValue_(0) {}
    EglConfig(EGLDisplay dpy, EGLConfig config);
    ~EglConfig() {}
    void print() const;
    static void print_header();
    bool isWindowConfig() const { return surfaceType_ & EGL_WINDOW_BIT; }
    EGLint configID() const { return configID_; }
};

class GLStateEGL : public GLState
{
    EGLNativeDisplayType native_display_;
    EGLNativeWindowType native_window_;
    EGLDisplay egl_display_;
    EGLConfig egl_config_;
    EGLContext egl_context_;
    EGLSurface egl_surface_;
    GLVisualConfig requested_visual_config_;
    EglConfig best_config_;
    SharedLibrary egl_lib_;
    SharedLibrary gl_lib_;
    bool gotValidDisplay();
    bool gotValidConfig();
    bool gotValidSurface();
    bool gotValidContext();
    void get_glvisualconfig(EGLConfig config, GLVisualConfig& visual_config);
    EGLConfig select_best_config(std::vector<EGLConfig>& configs);

    static GLADapiproc load_proc(const char* name, void* userptr);

public:
    GLStateEGL() :
        native_display_(0),
        native_window_(0),
        egl_display_(0),
        egl_config_(0),
        egl_context_(0),
        egl_surface_(0) {}
    ~GLStateEGL();

    bool init_display(void* native_display, GLVisualConfig& config_pref);
    bool init_surface(void* native_window);
    bool init_gl_extensions();
    bool valid();
    bool reset();
    void swap();
    // Performs a config search, returning a native visual ID on success
    bool gotNativeConfig(int& vid);
    void getVisualConfig(GLVisualConfig& vc);
};

#endif // GLMARK2_GL_STATE_EGL_H_
