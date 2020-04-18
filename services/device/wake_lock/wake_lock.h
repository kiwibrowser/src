// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DEVICE_WAKE_LOCK_WAKE_LOCK_H_
#define SERVICES_DEVICE_WAKE_LOCK_WAKE_LOCK_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "build/build_config.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/device/public/mojom/wake_lock.mojom.h"
#include "services/device/public/mojom/wake_lock_context.mojom.h"
#include "services/device/wake_lock/power_save_blocker/power_save_blocker.h"
#include "services/device/wake_lock/wake_lock_context.h"
#include "ui/gfx/native_widget_types.h"

namespace device {

class WakeLock : public mojom::WakeLock {
 public:
  WakeLock(mojom::WakeLockRequest request,
           mojom::WakeLockType type,
           mojom::WakeLockReason reason,
           const std::string& description,
           int context_id,
           WakeLockContextCallback native_view_getter,
           scoped_refptr<base::SingleThreadTaskRunner> file_task_runner);
  ~WakeLock() override;

  // WakeLockSevice implementation.
  void RequestWakeLock() override;
  void CancelWakeLock() override;
  void AddClient(mojom::WakeLockRequest request) override;
  void ChangeType(mojom::WakeLockType type,
                  ChangeTypeCallback callback) override;
  void HasWakeLockForTests(HasWakeLockForTestsCallback callback) override;

 protected:
  int num_lock_requests_;

 private:
  virtual void UpdateWakeLock();
  virtual void CreateWakeLock();
  virtual void RemoveWakeLock();
  virtual void SwapWakeLock();

  void OnConnectionError();

  mojom::WakeLockType type_;
  mojom::WakeLockReason reason_;
  std::unique_ptr<std::string> description_;

#if defined(OS_ANDROID)
  int context_id_;
  WakeLockContextCallback native_view_getter_;
#endif

  scoped_refptr<base::SequencedTaskRunner> main_task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> file_task_runner_;

  // The actual power save blocker for screen.
  std::unique_ptr<PowerSaveBlocker> wake_lock_;

  // Multiple clients that associate to the same WebContents share the same one
  // WakeLock instance. Two consecutive |RequestWakeLock| requests
  // from the same client should be coalesced as one request. Everytime a new
  // client is being added into the BindingSet, we create an unique_ptr<bool>
  // as its context, which records this client's request status.
  mojo::BindingSet<mojom::WakeLock, std::unique_ptr<bool>> binding_set_;

  DISALLOW_COPY_AND_ASSIGN(WakeLock);
};

}  // namespace device

#endif  // SERVICES_DEVICE_WAKE_LOCK_WAKE_LOCK_H_
