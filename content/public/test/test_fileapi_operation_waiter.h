// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_TEST_TEST_FILEAPI_OPERATION_WAITER_H_
#define CONTENT_PUBLIC_TEST_TEST_FILEAPI_OPERATION_WAITER_H_

#include <stdint.h>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "storage/browser/fileapi/file_observers.h"

namespace storage {
class FileSystemContext;
class FileSystemURL;
}

namespace content {

// Installs a temporary storage::FileUpdateObserver for use in browser tests
// that need to wait for a specific fileapi operation to complete.
class TestFileapiOperationWaiter : public storage::FileUpdateObserver {
 public:
  explicit TestFileapiOperationWaiter(storage::FileSystemContext* context);
  ~TestFileapiOperationWaiter() override;

  // Returns true if OnStartUpdate has occurred.
  bool did_start_update() { return did_start_update_; }

  // Wait for the next OnEndUpdate event.
  void WaitForEndUpdate();

  // FileUpdateObserver overrides.
  void OnStartUpdate(const storage::FileSystemURL& url) override;
  void OnUpdate(const storage::FileSystemURL& url, int64_t delta) override;
  void OnEndUpdate(const storage::FileSystemURL& url) override;

 private:
  bool did_start_update_ = false;
  base::RunLoop run_loop_;

  DISALLOW_COPY_AND_ASSIGN(TestFileapiOperationWaiter);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_TEST_TEST_FILEAPI_OPERATION_WAITER_H_
