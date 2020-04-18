// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_DISPLAY_MANAGER_APPLY_CONTENT_PROTECTION_TASK_H_
#define UI_DISPLAY_MANAGER_APPLY_CONTENT_PROTECTION_TASK_H_

#include <stddef.h>
#include <stdint.h>

#include <map>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "ui/display/manager/display_configurator.h"

namespace display {

class DisplayLayoutManager;
class NativeDisplayDelegate;

// In order to apply content protection the task executes in the following
// manner:
// 1) Run()
//   a) Query NativeDisplayDelegate for HDCP state on capable displays
//   b) OnHDCPStateUpdate() called for each display as response to (a)
// 2) ApplyProtections()
//   a) Compute preferred HDCP state for capable displays
//   b) Call into NativeDisplayDelegate to set HDCP state on capable displays
//   c) OnHDCPStateApplied() called for each display as reponse to (b)
// 3) Call |callback_| to signal end of task.
//
// Note, in steps 1a and 2a, if no HDCP capable displays are found or if errors
// are reported, the task finishes early and skips to step 3.
class DISPLAY_MANAGER_EXPORT ApplyContentProtectionTask {
 public:
  typedef base::Callback<void(bool)> ResponseCallback;

  ApplyContentProtectionTask(
      DisplayLayoutManager* layout_manager,
      NativeDisplayDelegate* native_display_delegate,
      const DisplayConfigurator::ContentProtections& requests,
      const ResponseCallback& callback);
  ~ApplyContentProtectionTask();

  void Run();

 private:
  // Callback to NatvieDisplayDelegate::GetHDCPState()
  void OnHDCPStateUpdate(int64_t display_id, bool success, HDCPState state);

  // Callback to NativeDisplayDelegate::SetHDCPState()
  void OnHDCPStateApplied(bool success);

  void ApplyProtections();

  uint32_t GetDesiredProtectionMask(int64_t display_id) const;

  DisplayLayoutManager* layout_manager_;  // Not owned.

  NativeDisplayDelegate* native_display_delegate_;  // Not owned.

  DisplayConfigurator::ContentProtections requests_;

  // Callback used to respond once the task finishes.
  ResponseCallback callback_;

  // Mapping between display IDs and the HDCP state returned by
  // NativeDisplayDelegate.
  std::map<int64_t, HDCPState> display_hdcp_state_map_;

  // Tracks the status of the NativeDisplayDelegate responses. This will be true
  // if all the queries were successful, false otherwise.
  bool query_status_;

  // Tracks the number of NativeDisplayDelegate requests sent but not answered
  // yet.
  size_t pending_requests_;

  base::WeakPtrFactory<ApplyContentProtectionTask> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ApplyContentProtectionTask);
};

}  // namespace display

#endif  // UI_DISPLAY_MANAGER_APPLY_CONTENT_PROTECTION_TASK_H_
