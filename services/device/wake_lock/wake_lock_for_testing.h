// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DEVICE_WAKE_LOCK_WAKE_LOCK_FOR_TESTING_H_
#define SERVICES_DEVICE_WAKE_LOCK_WAKE_LOCK_FOR_TESTING_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/device/public/mojom/wake_lock.mojom.h"
#include "services/device/public/mojom/wake_lock_context.mojom.h"
#include "services/device/wake_lock/wake_lock.h"
#include "ui/gfx/native_widget_types.h"

namespace device {

class WakeLockForTesting : public WakeLock {
 public:
  WakeLockForTesting(
      mojom::WakeLockRequest request,
      mojom::WakeLockType type,
      mojom::WakeLockReason reason,
      const std::string& description,
      int context_id,
      WakeLockContextCallback native_view_getter,
      scoped_refptr<base::SingleThreadTaskRunner> file_task_runner);
  ~WakeLockForTesting() override;

  void HasWakeLockForTests(HasWakeLockForTestsCallback callback) override;

 private:
  void UpdateWakeLock() override;
  void CreateWakeLock() override;
  void RemoveWakeLock() override;
  void SwapWakeLock() override;

  bool has_wake_lock_;

  DISALLOW_COPY_AND_ASSIGN(WakeLockForTesting);
};

}  // namespace device

#endif  // SERVICES_DEVICE_WAKE_LOCK_WAKE_LOCK_FOR_TESTING_H_
