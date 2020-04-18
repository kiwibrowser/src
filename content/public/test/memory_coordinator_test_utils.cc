// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/test/memory_coordinator_test_utils.h"

#include "base/memory/memory_coordinator_proxy.h"
#include "content/browser/memory/memory_coordinator_impl.h"
#include "content/public/common/content_features.h"

namespace content {

void SetUpMemoryCoordinatorProxyForTesting() {
  // Make sure that MemoryCoordinatorImpl is initialized.
  MemoryCoordinatorImpl::GetInstance();
}

}  // namespace content
