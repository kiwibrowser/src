// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_WAKE_LOCK_RESOURCE_THROTTLE_H_
#define CONTENT_BROWSER_LOADER_WAKE_LOCK_RESOURCE_THROTTLE_H_

#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/timer/timer.h"
#include "content/public/browser/resource_throttle.h"
#include "services/device/public/mojom/wake_lock.mojom.h"

namespace content {

// This ResourceThrottle holds wake lock until large upload request finishes.
class WakeLockResourceThrottle : public ResourceThrottle {
 public:
  WakeLockResourceThrottle(const std::string& host);
  ~WakeLockResourceThrottle() override;

  // ResourceThrottle overrides:
  void WillStartRequest(bool* defer) override;
  void WillProcessResponse(bool* defer) override;
  const char* GetNameForLogging() const override;

 private:
  void RequestWakeLock();

  const std::string host_;
  base::OneShotTimer timer_;

  // Destruction of wake_lock_ will trigger
  // WakeLock::OnConnectionError on the service side, so there is no
  // need to call CancelWakeLock() in the destructor.
  device::mojom::WakeLockPtr wake_lock_;

  DISALLOW_COPY_AND_ASSIGN(WakeLockResourceThrottle);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_WAKE_LOCK_RESOURCE_THROTTLE_H_
