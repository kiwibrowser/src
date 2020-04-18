// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_MEMORY_COORDINATOR_DELEGATE_H_
#define CONTENT_PUBLIC_BROWSER_MEMORY_COORDINATOR_DELEGATE_H_

#include "content/common/content_export.h"

namespace content {

// A delegate class which is used by the memory coordinator.
class CONTENT_EXPORT MemoryCoordinatorDelegate {
 public:
  virtual ~MemoryCoordinatorDelegate() {}

  // Requests to discard one tab. An implementation should select a low priority
  // tab to discard. If there is no tab that can be discarded, this doesn't take
  // effect.
  virtual void DiscardTab(bool skip_unload_handlers) {}
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_MEMORY_COORDINATOR_DELEGATE_H_
