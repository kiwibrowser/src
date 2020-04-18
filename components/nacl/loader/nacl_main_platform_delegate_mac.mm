// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/nacl/loader/nacl_main_platform_delegate.h"

#import <Cocoa/Cocoa.h>

#include "base/logging.h"
#include "components/nacl/common/nacl_switches.h"
#include "content/public/common/sandbox_init.h"
#include "services/service_manager/sandbox/sandbox_type.h"

void NaClMainPlatformDelegate::EnableSandbox(
    const content::MainFunctionParams& parameters) {
  CHECK(content::InitializeSandbox(service_manager::SANDBOX_TYPE_NACL_LOADER))
      << "Error initializing sandbox for " << switches::kNaClLoaderProcess;
}
