// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_RESOURCE_CONTROLLER_H_
#define CONTENT_BROWSER_LOADER_RESOURCE_CONTROLLER_H_

#include "content/common/content_export.h"

namespace content {

// Used to either resume a deferred resource load or cancel a resource load at
// any time.  CancelAndIgnore is a variation of Cancel that also causes the
// requester of the resource to act like the request was never made.  By
// default, load is cancelled with ERR_ABORTED code. CancelWithError can be used
// to cancel load with any other error code.
class CONTENT_EXPORT ResourceController {
 public:
  virtual ~ResourceController() {}

  virtual void Cancel() = 0;
  virtual void CancelWithError(int error_code) = 0;

  // Resumes the request. May only be called if the request was previously
  // deferred. Guaranteed not to call back into the ResourceHandler, or destroy
  // it, synchronously.
  virtual void Resume() = 0;
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_RESOURCE_CONTROLLER_H_
