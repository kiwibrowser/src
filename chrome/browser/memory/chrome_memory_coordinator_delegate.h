// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEMORY_CHROME_MEMORY_COORDINATOR_DELEGATE_H_
#define CHROME_BROWSER_MEMORY_CHROME_MEMORY_COORDINATOR_DELEGATE_H_

#include "base/memory/weak_ptr.h"
#include "content/public/browser/memory_coordinator_delegate.h"

namespace memory {

// A MemoryCoordinatorDelegate which uses TabManager internally.
class ChromeMemoryCoordinatorDelegate
    : public content::MemoryCoordinatorDelegate {
 public:
  static std::unique_ptr<content::MemoryCoordinatorDelegate> Create();

  ~ChromeMemoryCoordinatorDelegate() override;

  // MemoryCoordinatorDelegate implementation.
  void DiscardTab(bool skip_unload_handlers) override;

 private:
  ChromeMemoryCoordinatorDelegate();

  DISALLOW_COPY_AND_ASSIGN(ChromeMemoryCoordinatorDelegate);
};

}  // namespace memory

#endif  // CHROME_BROWSER_MEMORY_CHROME_MEMORY_COORDINATOR_DELEGATE_H_
