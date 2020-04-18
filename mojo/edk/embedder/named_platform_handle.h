// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_EDK_EMBEDDER_NAMED_PLATFORM_HANDLE_H_
#define MOJO_EDK_EMBEDDER_NAMED_PLATFORM_HANDLE_H_

#include "build/build_config.h"

#if defined(OS_WIN)
#include "mojo/edk/embedder/named_platform_handle_win.h"
#elif defined(OS_POSIX) || defined(OS_FUCHSIA)
#include "mojo/edk/embedder/named_platform_handle_posix.h"
#endif

#endif  // MOJO_EDK_EMBEDDER_NAMED_PLATFORM_HANDLE_H_
