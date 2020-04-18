// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/discardable_shared_memory_manager.h"

#include "content/browser/browser_main_loop.h"

namespace content {

discardable_memory::DiscardableSharedMemoryManager*
GetDiscardableSharedMemoryManager() {
  return content::BrowserMainLoop::GetInstance()
      ->discardable_shared_memory_manager();
}

}  // namespace content
