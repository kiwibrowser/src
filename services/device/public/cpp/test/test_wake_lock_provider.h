// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DEVICE_PUBLIC_CPP_TEST_TEST_WAKE_LOCK_PROVIDER_H_
#define SERVICES_DEVICE_PUBLIC_CPP_TEST_TEST_WAKE_LOCK_PROVIDER_H_

#include <set>
#include <string>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/device/public/mojom/wake_lock.mojom.h"
#include "services/device/public/mojom/wake_lock_provider.mojom.h"
#include "services/service_manager/public/cpp/service.h"

namespace device {

// TestWakeLockProvider provides a fake implementation of
// mojom::WakeLockProvider for use in unit tests.
class TestWakeLockProvider : public mojom::WakeLockProvider,
                             public service_manager::Service {
 public:
  TestWakeLockProvider();
  ~TestWakeLockProvider() override;

  void set_wake_lock_requested_callback(const base::RepeatingClosure& cb) {
    wake_lock_requested_callback_ = cb;
  }
  void set_wake_lock_canceled_callback(const base::RepeatingClosure& cb) {
    wake_lock_canceled_callback_ = cb;
  }

  // Returns the number of active (i.e. currently-requested) wake locks of type
  // |type|.
  int GetActiveWakeLocksOfType(mojom::WakeLockType type) const;

  // mojom::WakeLockProvider:
  void GetWakeLockContextForID(
      int context_id,
      mojo::InterfaceRequest<mojom::WakeLockContext> request) override;
  void GetWakeLockWithoutContext(mojom::WakeLockType type,
                                 mojom::WakeLockReason reason,
                                 const std::string& description,
                                 mojom::WakeLockRequest request) override;

  // service_manager::Service:
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override;

 private:
  class TestWakeLock;

  // Called by |wake_lock| when the lock is requested or canceled.
  void OnWakeLockRequested(TestWakeLock* wake_lock);
  void OnWakeLockCanceled(TestWakeLock* wake_lock);

  mojo::BindingSet<mojom::WakeLockProvider> bindings_;

  // Locks that have been passed to OnWakeLockRequested and haven't yet been
  // released.
  std::set<const TestWakeLock*> active_wake_locks_;

  // Callbacks to execute when wake locks are requested or canceled.
  base::RepeatingClosure wake_lock_requested_callback_;
  base::RepeatingClosure wake_lock_canceled_callback_;

  DISALLOW_COPY_AND_ASSIGN(TestWakeLockProvider);
};

}  // namespace device

#endif  // SERVICES_DEVICE_PUBLIC_CPP_TEST_TEST_WAKE_LOCK_PROVIDER_H_
