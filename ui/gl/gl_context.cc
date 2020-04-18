// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gl/gl_context.h"

#include <string>

#include "base/bind.h"
#include "base/cancelable_callback.h"
#include "base/command_line.h"
#include "base/debug/stack_trace.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_util.h"
#include "base/threading/thread_local.h"
#include "components/crash/core/common/crash_key.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_gl_api_implementation.h"
#include "ui/gl/gl_implementation.h"
#include "ui/gl/gl_surface.h"
#include "ui/gl/gl_switches.h"
#include "ui/gl/gl_version_info.h"
#include "ui/gl/gpu_timing.h"

namespace gl {

namespace {
base::LazyInstance<base::ThreadLocalPointer<GLContext>>::Leaky
    current_context_ = LAZY_INSTANCE_INITIALIZER;

base::LazyInstance<base::ThreadLocalPointer<GLContext>>::Leaky
    current_real_context_ = LAZY_INSTANCE_INITIALIZER;
}  // namespace

// static
base::subtle::Atomic32 GLContext::total_gl_contexts_ = 0;
// static
bool GLContext::switchable_gpus_supported_ = false;
// static
GpuPreference GLContext::forced_gpu_preference_ = GpuPreferenceNone;

GLContext::ScopedReleaseCurrent::ScopedReleaseCurrent() : canceled_(false) {}

GLContext::ScopedReleaseCurrent::~ScopedReleaseCurrent() {
  if (!canceled_ && GetCurrent()) {
    GetCurrent()->ReleaseCurrent(nullptr);
  }
}

void GLContext::ScopedReleaseCurrent::Cancel() {
  canceled_ = true;
}

GLContext::GLContext(GLShareGroup* share_group) : share_group_(share_group) {
  if (!share_group_.get())
    share_group_ = new gl::GLShareGroup();
  share_group_->AddContext(this);
  base::subtle::NoBarrier_AtomicIncrement(&total_gl_contexts_, 1);
}

GLContext::~GLContext() {
  share_group_->RemoveContext(this);
  if (GetCurrent() == this) {
    SetCurrent(nullptr);
    SetCurrentGL(nullptr);
  }
  base::subtle::Atomic32 after_value =
      base::subtle::NoBarrier_AtomicIncrement(&total_gl_contexts_, -1);
  DCHECK(after_value >= 0);
}

// static
int32_t GLContext::TotalGLContexts() {
  return static_cast<int32_t>(
      base::subtle::NoBarrier_Load(&total_gl_contexts_));
}

// static
bool GLContext::SwitchableGPUsSupported() {
  return switchable_gpus_supported_;
}

// static
void GLContext::SetSwitchableGPUsSupported() {
  DCHECK(!switchable_gpus_supported_);
  switchable_gpus_supported_ = true;
}

// static
void GLContext::SetForcedGpuPreference(GpuPreference gpu_preference) {
  DCHECK_EQ(GpuPreferenceNone, forced_gpu_preference_);
  forced_gpu_preference_ = gpu_preference;
}

// static
GpuPreference GLContext::AdjustGpuPreference(GpuPreference gpu_preference) {
  switch (forced_gpu_preference_) {
    case GpuPreferenceNone:
      return gpu_preference;
    case PreferIntegratedGpu:
    case PreferDiscreteGpu:
      return forced_gpu_preference_;
    default:
      NOTREACHED();
      return GpuPreferenceNone;
  }
}

GLApi* GLContext::CreateGLApi(DriverGL* driver) {
  real_gl_api_ = new RealGLApi;
  real_gl_api_->set_gl_workarounds(gl_workarounds_);
  real_gl_api_->SetDisabledExtensions(disabled_gl_extensions_);
  real_gl_api_->Initialize(driver);
  return real_gl_api_;
}

void GLContext::SetSafeToForceGpuSwitch() {
}

bool GLContext::ForceGpuSwitchIfNeeded() {
  return true;
}

void GLContext::SetUnbindFboOnMakeCurrent() {
  NOTIMPLEMENTED();
}

std::string GLContext::GetGLVersion() {
  DCHECK(IsCurrent(nullptr));
  DCHECK(gl_api_ != nullptr);
  const char* version =
      reinterpret_cast<const char*>(gl_api_->glGetStringFn(GL_VERSION));
  return std::string(version ? version : "");
}

std::string GLContext::GetGLRenderer() {
  DCHECK(IsCurrent(nullptr));
  DCHECK(gl_api_ != nullptr);
  const char* renderer =
      reinterpret_cast<const char*>(gl_api_->glGetStringFn(GL_RENDERER));
  return std::string(renderer ? renderer : "");
}

YUVToRGBConverter* GLContext::GetYUVToRGBConverter(
    const gfx::ColorSpace& color_space) {
  return nullptr;
}

CurrentGL* GLContext::GetCurrentGL() {
  if (!static_bindings_initialized_) {
    driver_gl_.reset(new DriverGL);
    driver_gl_->InitializeStaticBindings();

    gl_api_.reset(CreateGLApi(driver_gl_.get()));
    GLApi* final_api = gl_api_.get();

    if (base::CommandLine::ForCurrentProcess()->HasSwitch(
            switches::kEnableGPUServiceTracing)) {
      trace_gl_api_.reset(new TraceGLApi(final_api));
      final_api = trace_gl_api_.get();
    }

    if (GetDebugGLBindingsInitializedGL()) {
      debug_gl_api_.reset(new DebugGLApi(final_api));
      final_api = debug_gl_api_.get();
    }

    current_gl_.reset(new CurrentGL);
    current_gl_->Driver = driver_gl_.get();
    current_gl_->Api = final_api;
    current_gl_->Version = version_info_.get();

    static_bindings_initialized_ = true;
  }

  return current_gl_.get();
}

void GLContext::ReinitializeDynamicBindings() {
  DCHECK(IsCurrent(nullptr));
  dynamic_bindings_initialized_ = false;
  ResetExtensions();
  InitializeDynamicBindings();
}

void GLContext::ForceReleaseVirtuallyCurrent() {
  NOTREACHED();
}

bool GLContext::HasExtension(const char* name) {
  return gl::HasExtension(GetExtensions(), name);
}

const GLVersionInfo* GLContext::GetVersionInfo() {
  if (!version_info_) {
    version_info_ = GenerateGLVersionInfo();

    // current_gl_ may be null for virtual contexts
    if (current_gl_) {
      current_gl_->Version = version_info_.get();
    }
  }
  return version_info_.get();
}

GLShareGroup* GLContext::share_group() {
  return share_group_.get();
}

bool GLContext::LosesAllContextsOnContextLost() {
  switch (GetGLImplementation()) {
    case kGLImplementationDesktopGL:
      return false;
    case kGLImplementationEGLGLES2:
    case kGLImplementationSwiftShaderGL:
      return true;
    case kGLImplementationOSMesaGL:
    case kGLImplementationAppleGL:
      return false;
    case kGLImplementationMockGL:
    case kGLImplementationStubGL:
      return false;
    default:
      NOTREACHED();
      return true;
  }
}

GLContext* GLContext::GetCurrent() {
  return current_context_.Pointer()->Get();
}

GLContext* GLContext::GetRealCurrent() {
  return current_real_context_.Pointer()->Get();
}

GLContext* GLContext::GetRealCurrentForDebugging() {
  return GetRealCurrent();
}

std::unique_ptr<gl::GLVersionInfo> GLContext::GenerateGLVersionInfo() {
  return std::make_unique<GLVersionInfo>(
      GetGLVersion().c_str(), GetGLRenderer().c_str(), GetExtensions());
}

void GLContext::SetCurrent(GLSurface* surface) {
  current_context_.Pointer()->Set(surface ? this : nullptr);
  GLSurface::SetCurrent(surface);
  // Leave the real GL api current so that unit tests work correctly.
  // TODO(sievers): Remove this, but needs all gpu_unittest classes
  // to create and make current a context.
  if (!surface && GetGLImplementation() != kGLImplementationMockGL &&
      GetGLImplementation() != kGLImplementationStubGL) {
    // TODO(sunnyps): Remove after fixing crbug.com/724999.
    static crash_reporter::CrashKeyString<1024> crash_key(
        "gl-context-set-current-stack-trace");
    crash_reporter::SetCrashKeyStringToStackTrace(&crash_key,
                                                  base::debug::StackTrace());
    SetCurrentGL(nullptr);
  }
}

void GLContext::SetGLWorkarounds(const GLWorkarounds& workarounds) {
  DCHECK(!real_gl_api_);
  gl_workarounds_ = workarounds;
}

void GLContext::SetDisabledGLExtensions(
    const std::string& disabled_extensions) {
  DCHECK(!real_gl_api_);
  disabled_gl_extensions_ = disabled_extensions;
}

GLStateRestorer* GLContext::GetGLStateRestorer() {
  return state_restorer_.get();
}

void GLContext::SetGLStateRestorer(GLStateRestorer* state_restorer) {
  state_restorer_ = base::WrapUnique(state_restorer);
}

bool GLContext::WasAllocatedUsingRobustnessExtension() {
  return false;
}

void GLContext::InitializeDynamicBindings() {
  DCHECK(IsCurrent(nullptr));
  DCHECK(static_bindings_initialized_);
  if (!dynamic_bindings_initialized_) {
    if (real_gl_api_) {
      // This is called everytime DoRequestExtensionCHROMIUM() is called in
      // passthrough command buffer. So the underlying ANGLE driver will have
      // different GL extensions, therefore we need to clear the cache and
      // recompute on demand later.
      real_gl_api_->ClearCachedGLExtensions();
      real_gl_api_->set_version(GenerateGLVersionInfo());
    }

    driver_gl_->InitializeDynamicBindings(GetVersionInfo(), GetExtensions());
    dynamic_bindings_initialized_ = true;
  }
}

bool GLContext::MakeVirtuallyCurrent(
    GLContext* virtual_context, GLSurface* surface) {
  if (!ForceGpuSwitchIfNeeded())
    return false;
  bool switched_real_contexts = GLContext::GetRealCurrent() != this;
  GLSurface* current_surface = GLSurface::GetCurrent();
  if (switched_real_contexts || surface != current_surface) {
    // MakeCurrent 'lite' path that avoids potentially expensive MakeCurrent()
    // calls if the GLSurface uses the same underlying surface or renders to
    // an FBO.
    if (switched_real_contexts || !current_surface ||
        !virtual_context->IsCurrent(surface)) {
      if (!MakeCurrent(surface)) {
        return false;
      }
    }
  }

  DCHECK_EQ(this, GLContext::GetRealCurrent());
  DCHECK(IsCurrent(NULL));
  DCHECK(virtual_context->IsCurrent(surface));

  if (switched_real_contexts || virtual_context != current_virtual_context_) {
#if DCHECK_IS_ON()
    GLenum error = glGetError();
    // Accepting a context loss error here enables using debug mode to work on
    // context loss handling in virtual context mode.
    // There should be no other errors from the previous context leaking into
    // the new context.
    DCHECK(error == GL_NO_ERROR || error == GL_CONTEXT_LOST_KHR) <<
        "GL error was: " << error;
#endif

    // Set all state that is different from the real state
    if (virtual_context->GetGLStateRestorer()->IsInitialized()) {
      GLStateRestorer* virtual_state = virtual_context->GetGLStateRestorer();
      GLStateRestorer* current_state =
          current_virtual_context_
              ? current_virtual_context_->GetGLStateRestorer()
              : nullptr;
      if (current_state)
        current_state->PauseQueries();
      virtual_state->ResumeQueries();

      virtual_state->RestoreState(
          (current_state && !switched_real_contexts) ? current_state : NULL);
    }
    current_virtual_context_ = virtual_context;
  }

  virtual_context->SetCurrent(surface);
  if (!surface->OnMakeCurrent(virtual_context)) {
    LOG(ERROR) << "Could not make GLSurface current.";
    return false;
  }
  return true;
}

void GLContext::OnReleaseVirtuallyCurrent(GLContext* virtual_context) {
  if (current_virtual_context_ == virtual_context)
    current_virtual_context_ = nullptr;
}

void GLContext::BindGLApi() {
  SetCurrentGL(GetCurrentGL());
}

GLContextReal::GLContextReal(GLShareGroup* share_group)
    : GLContext(share_group) {}

scoped_refptr<GPUTimingClient> GLContextReal::CreateGPUTimingClient() {
  if (!gpu_timing_) {
    gpu_timing_.reset(GPUTiming::CreateGPUTiming(this));
  }
  return gpu_timing_->CreateGPUTimingClient();
}

const ExtensionSet& GLContextReal::GetExtensions() {
  DCHECK(IsCurrent(nullptr));
  if (!extensions_initialized_) {
    SetExtensionsFromString(GetGLExtensionsFromCurrentContext(gl_api()));
  }
  return extensions_;
}

GLContextReal::~GLContextReal() {
  if (GetRealCurrent() == this)
    current_real_context_.Pointer()->Set(nullptr);
}

void GLContextReal::SetCurrent(GLSurface* surface) {
  GLContext::SetCurrent(surface);
  current_real_context_.Pointer()->Set(surface ? this : nullptr);
}

scoped_refptr<GLContext> InitializeGLContext(scoped_refptr<GLContext> context,
                                             GLSurface* compatible_surface,
                                             const GLContextAttribs& attribs) {
  if (!context->Initialize(compatible_surface, attribs))
    return nullptr;
  return context;
}

void GLContextReal::SetExtensionsFromString(std::string extensions) {
  extensions_string_ = std::move(extensions);
  extensions_ = MakeExtensionSet(extensions_string_);
  extensions_initialized_ = true;
}

void GLContextReal::ResetExtensions() {
  extensions_.clear();
  extensions_string_.clear();
  extensions_initialized_ = false;
}

}  // namespace gl
