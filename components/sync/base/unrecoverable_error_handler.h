// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_BASE_UNRECOVERABLE_ERROR_HANDLER_H_
#define COMPONENTS_SYNC_BASE_UNRECOVERABLE_ERROR_HANDLER_H_

#include <string>

#include "base/location.h"

namespace syncer {

class UnrecoverableErrorHandler {
 public:
  // Call this when normal operation detects that the chrome model and the
  // syncer model are inconsistent, or similar.  The ProfileSyncService will
  // try to avoid doing any work to avoid crashing or corrupting things
  // further, and will report an error status if queried.
  virtual void OnUnrecoverableError(const base::Location& from_here,
                                    const std::string& message) = 0;
  virtual ~UnrecoverableErrorHandler() {}
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_BASE_UNRECOVERABLE_ERROR_HANDLER_H_
