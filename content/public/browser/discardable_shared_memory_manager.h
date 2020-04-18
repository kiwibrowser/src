// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_DISCARDABLE_SHARED_MEMORY_MANAGER_H_
#define CONTENT_PUBLIC_BROWSER_DISCARDABLE_SHARED_MEMORY_MANAGER_H_

#include "content/common/content_export.h"

namespace discardable_memory {
class DiscardableSharedMemoryManager;
}

namespace content {

CONTENT_EXPORT discardable_memory::DiscardableSharedMemoryManager*
GetDiscardableSharedMemoryManager();

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_DISCARDABLE_SHARED_MEMORY_MANAGER_H_
