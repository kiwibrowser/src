// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_FAKE_UPDATE_ENGINE_CLIENT_H_
#define CHROMEOS_DBUS_FAKE_UPDATE_ENGINE_CLIENT_H_

#include <string>

#include "base/containers/queue.h"
#include "chromeos/dbus/update_engine_client.h"

namespace chromeos {

// A fake implementation of UpdateEngineClient. The user of this class can
// use set_update_engine_client_status() to set a fake last Status and
// GetLastStatus() returns the fake with no modification. Other methods do
// nothing.
class FakeUpdateEngineClient : public UpdateEngineClient {
 public:
  FakeUpdateEngineClient();
  ~FakeUpdateEngineClient() override;

  // UpdateEngineClient overrides
  void Init(dbus::Bus* bus) override;
  void AddObserver(Observer* observer) override;
  void RemoveObserver(Observer* observer) override;
  bool HasObserver(const Observer* observer) const override;
  void RequestUpdateCheck(const UpdateCheckCallback& callback) override;
  void RebootAfterUpdate() override;
  void Rollback() override;
  void CanRollbackCheck(const RollbackCheckCallback& callback) override;
  Status GetLastStatus() override;
  void SetChannel(const std::string& target_channel,
                  bool is_powerwash_allowed) override;
  void GetChannel(bool get_current_channel,
                  const GetChannelCallback& callback) override;
  void GetEolStatus(GetEolStatusCallback callback) override;
  void SetUpdateOverCellularPermission(bool allowed,
                                       const base::Closure& callback) override;
  void SetUpdateOverCellularOneTimePermission(
      const std::string& target_version,
      int64_t target_size,
      const UpdateOverCellularOneTimePermissionCallback& callback) override;
  // Pushes UpdateEngineClient::Status in the queue to test changing status.
  // GetLastStatus() returns the status set by this method in FIFO order.
  // See set_default_status().
  void PushLastStatus(const UpdateEngineClient::Status& status) {
    status_queue_.push(status);
  }

  // Sends status change notification.
  void NotifyObserversThatStatusChanged(
      const UpdateEngineClient::Status& status);

  // Notifies observers that the user's one time permission is granted.
  void NotifyUpdateOverCellularOneTimePermissionGranted();

  // Sets the default UpdateEngineClient::Status. GetLastStatus() returns the
  // value set here if |status_queue_| is empty.
  void set_default_status(const UpdateEngineClient::Status& status);

  // Sets a value returned by RequestUpdateCheck().
  void set_update_check_result(
      const UpdateEngineClient::UpdateCheckResult& result);

  void set_can_rollback_check_result(bool result) {
      can_rollback_stub_result_ = result;
  }

  // Returns how many times RebootAfterUpdate() is called.
  int reboot_after_update_call_count() const {
      return reboot_after_update_call_count_;
  }

  // Returns how many times RequestUpdateCheck() is called.
  int request_update_check_call_count() const {
    return request_update_check_call_count_;
  }

  // Returns how many times Rollback() is called.
  int rollback_call_count() const { return rollback_call_count_; }

  // Returns how many times Rollback() is called.
  int can_rollback_call_count() const { return can_rollback_call_count_; }

 private:
  base::ObserverList<Observer> observers_;
  base::queue<UpdateEngineClient::Status> status_queue_;
  UpdateEngineClient::Status default_status_;
  UpdateEngineClient::UpdateCheckResult update_check_result_;
  bool can_rollback_stub_result_;
  int reboot_after_update_call_count_;
  int request_update_check_call_count_;
  int rollback_call_count_;
  int can_rollback_call_count_;
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_FAKE_UPDATE_ENGINE_CLIENT_H_
