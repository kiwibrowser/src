// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_TOAST_TOAST_MANAGER_H_
#define ASH_SYSTEM_TOAST_TOAST_MANAGER_H_

#include <memory>
#include <string>

#include "ash/ash_export.h"
#include "ash/system/toast/toast_data.h"
#include "ash/system/toast/toast_overlay.h"
#include "base/containers/circular_deque.h"
#include "base/memory/weak_ptr.h"

namespace ash {

// Class managing toast requests.
class ASH_EXPORT ToastManager : public ToastOverlay::Delegate {
 public:
  ToastManager();
  ~ToastManager() override;

  // Show a toast. If there are queued toasts, succeeding toasts are queued as
  // well, and are shown one by one.
  void Show(const ToastData& data);

  void Cancel(const std::string& id);

  // ToastOverlay::Delegate overrides:
  void OnClosed() override;

 private:
  friend class ToastManagerTest;

  void ShowLatest();
  void OnDurationPassed(int toast_number);

  ToastOverlay* GetCurrentOverlayForTesting() { return overlay_.get(); }
  int serial_for_testing() const { return serial_; }
  void ResetSerialForTesting() { serial_ = 0; }

  // ID of the toast which is currently shown. Empty if no toast is visible.
  std::string current_toast_id_;

  int serial_ = 0;
  base::circular_deque<ToastData> queue_;
  std::unique_ptr<ToastOverlay> overlay_;

  base::WeakPtrFactory<ToastManager> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ToastManager);
};

}  // namespace ash

#endif  // ASH_SYSTEM_TOAST_TOAST_MANAGER_H_
