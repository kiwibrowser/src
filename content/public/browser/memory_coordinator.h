// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_MEMORY_COORDINATOR_H_
#define CONTENT_PUBLIC_BROWSER_MEMORY_COORDINATOR_H_

#include "base/memory/memory_coordinator_client.h"
#include "base/process/process_handle.h"
#include "content/common/content_export.h"

namespace content {

// The interface that represents the browser side of MemoryCoordinator.
// MemoryCoordinator determines memory state of each process (both browser
// and renderers) and dispatches state change notifications to its clients.
// See comments on MemoryCoordinatorClient for details.
class CONTENT_EXPORT MemoryCoordinator {
 public:
  virtual ~MemoryCoordinator() {}

  static MemoryCoordinator* GetInstance();

  // Returns the memory state of the given process handle.
  virtual base::MemoryState GetStateForProcess(base::ProcessHandle handle) = 0;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_MEMORY_COORDINATOR_H_
