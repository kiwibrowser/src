// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gl/init/gl_initializer.h"

#include "base/base_paths.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/native_library.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_egl_api_implementation.h"
#include "ui/gl/gl_features.h"
#include "ui/gl/gl_gl_api_implementation.h"
#include "ui/gl/gl_implementation_osmesa.h"
#include "ui/gl/gl_osmesa_api_implementation.h"
#include "ui/gl/gl_surface_egl.h"

#if BUILDFLAG(USE_STATIC_ANGLE)
#include <EGL/egl.h>
#endif  // BUILDFLAG(USE_STATIC_ANGLE)

namespace gl {
namespace init {

namespace {

bool InitializeStaticEGLInternal() {
#if BUILDFLAG(USE_STATIC_ANGLE)
#pragma push_macro("eglGetProcAddress")
#undef eglGetProcAddress
  SetGLGetProcAddressProc(&eglGetProcAddress);
#pragma pop_macro("eglGetProcAddress")
#else   // BUILDFLAG(USE_STATIC_ANGLE)
  base::NativeLibrary gles_library = LoadLibraryAndPrintError("libGLESv2.so");
  if (!gles_library)
    return false;
  base::NativeLibrary egl_library = LoadLibraryAndPrintError("libEGL.so");
  if (!egl_library) {
    base::UnloadNativeLibrary(gles_library);
    return false;
  }

  GLGetProcAddressProc get_proc_address =
      reinterpret_cast<GLGetProcAddressProc>(
          base::GetFunctionPointerFromNativeLibrary(egl_library,
                                                    "eglGetProcAddress"));
  if (!get_proc_address) {
    LOG(ERROR) << "eglGetProcAddress not found.";
    base::UnloadNativeLibrary(egl_library);
    base::UnloadNativeLibrary(gles_library);
    return false;
  }

  SetGLGetProcAddressProc(get_proc_address);
  AddGLNativeLibrary(egl_library);
  AddGLNativeLibrary(gles_library);
#endif  // BUILDFLAG(USE_STATIC_ANGLE)
  SetGLImplementation(kGLImplementationEGLGLES2);

  InitializeStaticGLBindingsGL();
  InitializeStaticGLBindingsEGL();

  return true;
}

}  // namespace

bool InitializeGLOneOffPlatform() {
  switch (GetGLImplementation()) {
    case kGLImplementationEGLGLES2:
      if (!GLSurfaceEGL::InitializeOneOff(EGL_DEFAULT_DISPLAY)) {
        LOG(ERROR) << "GLSurfaceEGL::InitializeOneOff failed.";
        return false;
      }
      return true;
    default:
      return true;
  }
}

bool InitializeStaticGLBindings(GLImplementation implementation) {
  // Prevent reinitialization with a different implementation. Once the gpu
  // unit tests have initialized with kGLImplementationMock, we don't want to
  // later switch to another GL implementation.
  DCHECK_EQ(kGLImplementationNone, GetGLImplementation());

  switch (implementation) {
    case kGLImplementationEGLGLES2:
      return InitializeStaticEGLInternal();
    case kGLImplementationOSMesaGL:
      return InitializeStaticGLBindingsOSMesaGL();
    case kGLImplementationMockGL:
    case kGLImplementationStubGL:
      SetGLImplementation(implementation);
      InitializeStaticGLBindingsGL();
      return true;
    default:
      NOTREACHED();
  }

  return false;
}

void InitializeDebugGLBindings() {
  InitializeDebugGLBindingsEGL();
  InitializeDebugGLBindingsGL();
  InitializeDebugGLBindingsOSMESA();
}

void ShutdownGLPlatform() {
  GLSurfaceEGL::ShutdownOneOff();
  ClearBindingsEGL();
  ClearBindingsGL();
  ClearBindingsOSMESA();
}

}  // namespace init
}  // namespace gl
