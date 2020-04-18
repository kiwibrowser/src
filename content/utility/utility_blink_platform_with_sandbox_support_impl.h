// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_UTILITY_UTILITY_BLINK_PLATFORM_WITH_SANDBOX_SUPPORT_IMPL_H_
#define CONTENT_UTILITY_UTILITY_BLINK_PLATFORM_WITH_SANDBOX_SUPPORT_IMPL_H_

#include <memory>

#include "base/macros.h"
#include "build/build_config.h"
#include "content/utility/utility_blink_platform_impl.h"

namespace blink {
class WebSandboxSupport;
}

namespace content {

// This class extends from UtilityBlinkPlatformImpl with added blink web
// sandbox support.
class UtilityBlinkPlatformWithSandboxSupportImpl
    : public UtilityBlinkPlatformImpl {
 public:
  UtilityBlinkPlatformWithSandboxSupportImpl();
  ~UtilityBlinkPlatformWithSandboxSupportImpl() override;

  // BlinkPlatformImpl
  blink::WebSandboxSupport* GetSandboxSupport() override;

 private:
#if defined(OS_POSIX) && !defined(OS_ANDROID) && !defined(OS_FUCHSIA)
  class SandboxSupport;
  std::unique_ptr<SandboxSupport> sandbox_support_;
#endif

  DISALLOW_COPY_AND_ASSIGN(UtilityBlinkPlatformWithSandboxSupportImpl);
};

}  // namespace content

#endif  // CONTENT_UTILITY_UTILITY_BLINK_PLATFORM_WITH_SANDBOX_SUPPORT_IMPL_H_
