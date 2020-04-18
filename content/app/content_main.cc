// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/app/content_main.h"

#include "content/app/content_service_manager_main_delegate.h"
#include "services/service_manager/embedder/main.h"

namespace content {

int ContentMain(const ContentMainParams& params) {
  ContentServiceManagerMainDelegate delegate(params);
  service_manager::MainParams main_params(&delegate);
#if defined(OS_POSIX) && !defined(OS_ANDROID)
  main_params.argc = params.argc;
  main_params.argv = params.argv;
#endif
  return service_manager::Main(main_params);
}

}  // namespace content
