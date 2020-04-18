// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/public/cpp/test/test_wake_lock_provider.h"

#include "base/callback.h"
#include "base/logging.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/device/public/mojom/constants.mojom.h"

namespace device {

// TestWakeLock implements mojom::WakeLock on behalf of TestWakeLockProvider.
class TestWakeLockProvider::TestWakeLock : public mojom::WakeLock,
                                           public service_manager::Service {
 public:
  TestWakeLock(TestWakeLockProvider* provider, mojom::WakeLockType type)
      : provider_(provider), type_(type) {}
  ~TestWakeLock() override {
    if (active_)
      provider_->OnWakeLockCanceled(this);
  }

  mojom::WakeLockType type() const { return type_; }

  // mojom::WakeLock:
  void RequestWakeLock() override {
    if (!active_) {
      provider_->OnWakeLockRequested(this);
      active_ = true;
    }
  }
  void CancelWakeLock() override {
    if (active_) {
      provider_->OnWakeLockCanceled(this);
      active_ = false;
    }
  }
  void AddClient(mojom::WakeLockRequest request) override { NOTIMPLEMENTED(); }
  void ChangeType(mojom::WakeLockType type,
                  ChangeTypeCallback callback) override {
    NOTIMPLEMENTED();
  }
  void HasWakeLockForTests(HasWakeLockForTestsCallback callback) override {
    NOTIMPLEMENTED();
  }

 private:
  TestWakeLockProvider* provider_;  // Not owned.

  mojom::WakeLockType type_;

  // Set to true by RequestWakeLock and back to false by CancelWakeLock.
  bool active_ = false;

  DISALLOW_COPY_AND_ASSIGN(TestWakeLock);
};

TestWakeLockProvider::TestWakeLockProvider() = default;

TestWakeLockProvider::~TestWakeLockProvider() = default;

int TestWakeLockProvider::GetActiveWakeLocksOfType(
    mojom::WakeLockType type) const {
  int count = 0;
  for (const auto* lock : active_wake_locks_) {
    if (lock->type() == type)
      count++;
  }
  return count;
}

void TestWakeLockProvider::GetWakeLockContextForID(
    int context_id,
    mojo::InterfaceRequest<mojom::WakeLockContext> request) {
  // This method is only used on Android.
  NOTIMPLEMENTED();
}

void TestWakeLockProvider::GetWakeLockWithoutContext(
    mojom::WakeLockType type,
    mojom::WakeLockReason reason,
    const std::string& description,
    mojom::WakeLockRequest request) {
  auto wake_lock = std::make_unique<TestWakeLock>(this, type);
  mojo::MakeStrongBinding(std::move(wake_lock), std::move(request));
}

void TestWakeLockProvider::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  DCHECK_EQ(interface_name, mojom::WakeLockProvider::Name_);
  bindings_.AddBinding(
      this, mojom::WakeLockProviderRequest(std::move(interface_pipe)));
}

void TestWakeLockProvider::OnWakeLockRequested(TestWakeLock* wake_lock) {
  DCHECK(!active_wake_locks_.count(wake_lock));
  active_wake_locks_.insert(wake_lock);
  if (wake_lock_requested_callback_)
    wake_lock_requested_callback_.Run();
}

void TestWakeLockProvider::OnWakeLockCanceled(TestWakeLock* wake_lock) {
  DCHECK(active_wake_locks_.count(wake_lock));
  active_wake_locks_.erase(wake_lock);
  if (wake_lock_canceled_callback_)
    wake_lock_canceled_callback_.Run();
}

}  // namespace device
