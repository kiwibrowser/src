// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gl/gl_implementation_osmesa.h"

#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/native_library.h"
#include "base/path_service.h"
#include "build/build_config.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_gl_api_implementation.h"
#include "ui/gl/gl_implementation.h"
#include "ui/gl/gl_osmesa_api_implementation.h"

namespace gl {

bool InitializeStaticGLBindingsOSMesaGL() {
  base::FilePath module_path;

#if !defined(OS_FUCHSIA)
  // On all platforms except Fuchsia libosmesa.so is expected to be in the same
  // directory as the chrome binary. Pass full path to dlopen() to ensure we
  // load the right version.
  //
  // On Fuchsia libraries are normally in the lib directory. The loader service
  // will load libosmesa.so that belongs to the chrome package when dlopen() is
  // called with relative path.
  if (!base::PathService::Get(base::DIR_MODULE, &module_path)) {
    LOG(ERROR) << "PathService::Get failed.";
    return false;
  }
#endif  // !defined(OS_FUCHSIA)

#if defined(OS_WIN)
  base::FilePath library_path =
      module_path.Append(FILE_PATH_LITERAL("osmesa.dll"));
#else
  base::FilePath library_path =
      module_path.Append(FILE_PATH_LITERAL("libosmesa.so"));
#endif

  base::NativeLibrary library = LoadLibraryAndPrintError(library_path);
  if (!library)
    return false;

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

}  // namespace gl
