// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_DISPLAY_MANAGER_QUERY_CONTENT_PROTECTION_TASK_H_
#define UI_DISPLAY_MANAGER_QUERY_CONTENT_PROTECTION_TASK_H_

#include <stddef.h>
#include <stdint.h>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "ui/display/manager/display_manager_export.h"
#include "ui/display/types/display_constants.h"

namespace display {

class DisplayLayoutManager;
class NativeDisplayDelegate;

class DISPLAY_MANAGER_EXPORT QueryContentProtectionTask {
 public:
  struct Response {
    bool success = false;
    uint32_t link_mask = 0;
    uint32_t enabled = 0;
    uint32_t unfulfilled = 0;
  };

  typedef base::Callback<void(Response)> ResponseCallback;

  QueryContentProtectionTask(DisplayLayoutManager* layout_manager,
                             NativeDisplayDelegate* native_display_delegate,
                             int64_t display_id,
                             const ResponseCallback& callback);
  ~QueryContentProtectionTask();

  void Run();

 private:
  // Callback for NativeDisplayDelegate::GetHDCPState()
  void OnHDCPStateUpdate(bool success, HDCPState state);

  DisplayLayoutManager* layout_manager_;  // Not owned.

  NativeDisplayDelegate* native_display_delegate_;  // Not owned.

  // Display ID for the query.
  int64_t display_id_;

  // Called at the end of the query to signal completion.
  ResponseCallback callback_;

  Response response_;

  // Tracks the number of NativeDisplayDelegate requests sent but not answered
  // yet.
  size_t pending_requests_;

  base::WeakPtrFactory<QueryContentProtectionTask> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(QueryContentProtectionTask);
};

}  // namespace display

#endif  // UI_DISPLAY_MANAGER_QUERY_CONTENT_PROTECTION_TASK_H_
