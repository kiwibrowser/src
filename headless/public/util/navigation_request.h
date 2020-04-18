// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_PUBLIC_UTIL_NAVIGATION_REQUEST_H_
#define HEADLESS_PUBLIC_UTIL_NAVIGATION_REQUEST_H_

#include "base/macros.h"

namespace headless {

// While the actual details of the navigation processing are left undefined,
// it's anticipated implementations will use Network.requestIntercepted event.
class NavigationRequest {
 public:
  NavigationRequest() {}
  virtual ~NavigationRequest() {}

  // Called on the IO thread to ask the implementation to start processing the
  // navigation request. The NavigationRequest will be deleted immediately after
  // The |done_callback| can be called from any thread.
  virtual void StartProcessing(base::Closure done_callback) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(NavigationRequest);
};

}  // namespace headless

#endif  // HEADLESS_PUBLIC_UTIL_NAVIGATION_REQUEST_H_
