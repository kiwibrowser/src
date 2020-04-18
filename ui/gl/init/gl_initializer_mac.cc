// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gl/init/gl_initializer.h"

#include <OpenGL/CGLRenderers.h>

#include <vector>

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/mac/bundle_locations.h"
#include "base/mac/foundation_util.h"
#include "base/native_library.h"
#include "base/path_service.h"
#include "base/threading/thread_restrictions.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/gl_features.h"
#include "ui/gl/gl_gl_api_implementation.h"
#include "ui/gl/gl_implementation.h"
#include "ui/gl/gl_osmesa_api_implementation.h"
#include "ui/gl/gl_surface.h"
#include "ui/gl/gpu_switching_manager.h"

#if BUILDFLAG(USE_EGL_ON_MAC)
#include "ui/gl/gl_egl_api_implementation.h"
#include "ui/gl/gl_surface_egl.h"
#endif  // BUILDFLAG(USE_EGL_ON_MAC)

namespace gl {
namespace init {

namespace {

const char kOpenGLFrameworkPath[] =
    "/System/Library/Frameworks/OpenGL.framework/Versions/Current/OpenGL";

bool InitializeOneOffForSandbox() {
  static bool initialized = false;
  if (initialized)
    return true;

  // This is called from the sandbox warmup code on Mac OS X.
  // GPU-related stuff is very slow without this, probably because
  // the sandbox prevents loading graphics drivers or some such.
  std::vector<CGLPixelFormatAttribute> attribs;
  if (GLContext::SwitchableGPUsSupported()) {
    // Avoid switching to the discrete GPU just for this pixel
    // format selection.
    attribs.push_back(kCGLPFAAllowOfflineRenderers);
  }
  if (GetGLImplementation() == kGLImplementationAppleGL) {
    attribs.push_back(kCGLPFARendererID);
    attribs.push_back(
        static_cast<CGLPixelFormatAttribute>(kCGLRendererGenericFloatID));
  }
  attribs.push_back(static_cast<CGLPixelFormatAttribute>(0));

  CGLPixelFormatObj format;
  GLint num_pixel_formats;
  if (CGLChoosePixelFormat(&attribs.front(), &format, &num_pixel_formats) !=
      kCGLNoError) {
    LOG(ERROR) << "Error choosing pixel format.";
    return false;
  }
  if (!format) {
    LOG(ERROR) << "format == 0.";
    return false;
  }
  CGLReleasePixelFormat(format);
  DCHECK_NE(num_pixel_formats, 0);
  initialized = true;
  return true;
}

bool InitializeStaticOSMesaInternal() {
  // osmesa.so is located in the build directory. This code path is only
  // valid in a developer build environment.
  base::FilePath exe_path;
  if (!base::PathService::Get(base::FILE_EXE, &exe_path)) {
    LOG(ERROR) << "PathService::Get failed.";
    return false;
  }
  base::FilePath bundle_path = base::mac::GetAppBundlePath(exe_path);
  // Some unit test targets depend on osmesa but aren't built as app
  // bundles. In that case, the .so is next to the executable.
  if (bundle_path.empty())
    bundle_path = exe_path;
  base::FilePath build_dir_path = bundle_path.DirName();
  base::FilePath osmesa_path = build_dir_path.Append("osmesa.so");

  // When using OSMesa, just use OSMesaGetProcAddress to find entry points.
  base::NativeLibrary library = base::LoadNativeLibrary(osmesa_path, NULL);
  if (!library) {
    LOG(ERROR) << "osmesa.so not found at " << osmesa_path.value();
    return false;
  }

  GLGetProcAddressProc get_proc_address =
      reinterpret_cast<GLGetProcAddressProc>(
          base::GetFunctionPointerFromNativeLibrary(library,
                                                    "OSMesaGetProcAddress"));
  if (!get_proc_address) {
    LOG(ERROR) << "OSMesaGetProcAddress not found.";
    base::UnloadNativeLibrary(library);
    return false;
  }

  SetGLGetProcAddressProc(get_proc_address);
  AddGLNativeLibrary(library);
  SetGLImplementation(kGLImplementationOSMesaGL);

  InitializeStaticGLBindingsGL();
  InitializeStaticGLBindingsOSMESA();

  return true;
}

bool InitializeStaticCGLInternal(GLImplementation implementation) {
  base::NativeLibrary library =
      base::LoadNativeLibrary(base::FilePath(kOpenGLFrameworkPath), nullptr);
  if (!library) {
    LOG(ERROR) << "OpenGL framework not found";
    return false;
  }

  AddGLNativeLibrary(library);
  SetGLImplementation(implementation);

  InitializeStaticGLBindingsGL();
  return true;
}

#if BUILDFLAG(USE_EGL_ON_MAC)
const char kGLESv2ANGLELibraryName[] = "libGLESv2.dylib";
const char kEGLANGLELibraryName[] = "libEGL.dylib";

const char kGLESv2SwiftShaderLibraryName[] = "libswiftshader_libGLESv2.dylib";
const char kEGLSwiftShaderLibraryName[] = "libswiftshader_libEGL.dylib";

bool InitializeStaticEGLInternal(GLImplementation implementation) {
  // Some unit test targets depend on Angle/SwiftShader but aren't built
  // as app bundles. In that case, the .dylib is next to the executable.
  base::FilePath base_dir;
  if (base::mac::AmIBundled()) {
    base_dir =
        base::mac::FrameworkBundlePath().Append("Versions/Current/Libraries/");
  } else {
    if (!base::PathService::Get(base::FILE_EXE, &base_dir)) {
      LOG(ERROR) << "PathService::Get failed.";
      return false;
    }
    base_dir = base_dir.DirName();
  }

  base::FilePath glesv2_path;
  base::FilePath egl_path;
  if (implementation == kGLImplementationSwiftShaderGL) {
#if BUILDFLAG(ENABLE_SWIFTSHADER)
    glesv2_path = base_dir.Append(kGLESv2SwiftShaderLibraryName);
    egl_path = base_dir.Append(kEGLSwiftShaderLibraryName);
#else
    return false;
#endif
  } else {
    glesv2_path = base_dir.Append(kGLESv2ANGLELibraryName);
    egl_path = base_dir.Append(kEGLANGLELibraryName);
  }

  base::NativeLibrary gles_library = LoadLibraryAndPrintError(glesv2_path);
  if (!gles_library) {
    return false;
  }

  base::NativeLibrary egl_library = LoadLibraryAndPrintError(egl_path);
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
  // FIXME: SwiftShader must load symbols from libGLESv2 before libEGL on MacOS
  // currently
  AddGLNativeLibrary(gles_library);
  AddGLNativeLibrary(egl_library);
  SetGLImplementation(implementation);

  InitializeStaticGLBindingsGL();
  InitializeStaticGLBindingsEGL();

  return true;
}
#endif  // BUILDFLAG(USE_EGL_ON_MAC)

}  // namespace

bool InitializeGLOneOffPlatform() {
  switch (GetGLImplementation()) {
    case kGLImplementationDesktopGL:
    case kGLImplementationDesktopGLCoreProfile:
    case kGLImplementationAppleGL:
      if (!InitializeOneOffForSandbox()) {
        LOG(ERROR) << "GLSurfaceCGL::InitializeOneOff failed.";
        return false;
      }
      return true;
#if BUILDFLAG(USE_EGL_ON_MAC)
    case kGLImplementationEGLGLES2:
    case kGLImplementationSwiftShaderGL:
      if (!GLSurfaceEGL::InitializeOneOff(0)) {
        LOG(ERROR) << "GLSurfaceEGL::InitializeOneOff failed.";
        return false;
      }
      return true;
#endif  // BUILDFLAG(USE_EGL_ON_MAC)
    default:
      return true;
  }
}

bool InitializeStaticGLBindings(GLImplementation implementation) {
  // Prevent reinitialization with a different implementation. Once the gpu
  // unit tests have initialized with kGLImplementationMock, we don't want to
  // later switch to another GL implementation.
  DCHECK_EQ(kGLImplementationNone, GetGLImplementation());

  // Allow the main thread or another to initialize these bindings
  // after instituting restrictions on I/O. Going forward they will
  // likely be used in the browser process on most platforms. The
  // one-time initialization cost is small, between 2 and 5 ms.
  base::ThreadRestrictions::ScopedAllowIO allow_io;

  switch (implementation) {
    case kGLImplementationOSMesaGL:
      return InitializeStaticOSMesaInternal();
    case kGLImplementationDesktopGL:
    case kGLImplementationDesktopGLCoreProfile:
    case kGLImplementationAppleGL:
      return InitializeStaticCGLInternal(implementation);
#if BUILDFLAG(USE_EGL_ON_MAC)
    case kGLImplementationEGLGLES2:
    case kGLImplementationSwiftShaderGL:
      return InitializeStaticEGLInternal(implementation);
#endif  // BUILDFLAG(USE_EGL_ON_MAC)
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
  InitializeDebugGLBindingsGL();
  InitializeDebugGLBindingsOSMESA();
}

void ShutdownGLPlatform() {
  ClearBindingsGL();
  ClearBindingsOSMESA();
}

}  // namespace init
}  // namespace gl
