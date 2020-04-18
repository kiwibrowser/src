// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NACL_LOADER_NACL_MAIN_PLATFORM_DELEGATE_H_
#define COMPONENTS_NACL_LOADER_NACL_MAIN_PLATFORM_DELEGATE_H_

#include "base/macros.h"

namespace content {
struct MainFunctionParams;
}

class NaClMainPlatformDelegate {
 public:
  NaClMainPlatformDelegate() = default;

  // Initiate Lockdown.
  void EnableSandbox(const content::MainFunctionParams& parameters);

 private:
  DISALLOW_COPY_AND_ASSIGN(NaClMainPlatformDelegate);
};

#endif  // COMPONENTS_NACL_LOADER_NACL_MAIN_PLATFORM_DELEGATE_H_
