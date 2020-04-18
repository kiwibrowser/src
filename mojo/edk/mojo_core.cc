// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/time/time.h"
#include "mojo/edk/embedder/entrypoints.h"
#include "mojo/edk/system/core.h"
#include "mojo/public/c/system/core.h"
#include "mojo/public/c/system/thunks.h"

extern "C" {

namespace {

MojoResult InitializeImpl(const struct MojoInitializeOptions* options) {
  mojo::edk::InitializeCore();
  return MOJO_RESULT_OK;
}

MojoSystemThunks g_thunks = {0};

}  // namespace

#if defined(WIN32)
#define EXPORT_FROM_MOJO_CORE __declspec(dllexport)
#else
#define EXPORT_FROM_MOJO_CORE __attribute__((visibility("default")))
#endif

EXPORT_FROM_MOJO_CORE void MojoGetSystemThunks(MojoSystemThunks* thunks) {
  if (!g_thunks.size) {
    g_thunks = mojo::edk::GetSystemThunks();
    g_thunks.Initialize = InitializeImpl;
  }

  // Since this is the first version of the library, no valid system API
  // implementation can request fewer thunks than we have available.
  CHECK_GE(thunks->size, g_thunks.size);
  memcpy(thunks, &g_thunks, g_thunks.size);
}

}  // extern "C"
