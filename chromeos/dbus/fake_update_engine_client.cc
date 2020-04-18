// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/dbus/fake_update_engine_client.h"

#include "base/bind.h"
#include "base/threading/thread_task_runner_handle.h"

namespace chromeos {

FakeUpdateEngineClient::FakeUpdateEngineClient()
    : update_check_result_(UpdateEngineClient::UPDATE_RESULT_SUCCESS),
      can_rollback_stub_result_(false),
      reboot_after_update_call_count_(0),
      request_update_check_call_count_(0),
      rollback_call_count_(0),
      can_rollback_call_count_(0) {}

FakeUpdateEngineClient::~FakeUpdateEngineClient() = default;

void FakeUpdateEngineClient::Init(dbus::Bus* bus) {
}

void FakeUpdateEngineClient::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void FakeUpdateEngineClient::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

bool FakeUpdateEngineClient::HasObserver(const Observer* observer) const {
  return observers_.HasObserver(observer);
}

void FakeUpdateEngineClient::RequestUpdateCheck(
    const UpdateCheckCallback& callback) {
  request_update_check_call_count_++;
  callback.Run(update_check_result_);
}

void FakeUpdateEngineClient::Rollback() {
  rollback_call_count_++;
}

void FakeUpdateEngineClient::CanRollbackCheck(
    const RollbackCheckCallback& callback) {
  can_rollback_call_count_++;
  callback.Run(can_rollback_stub_result_);
}

void FakeUpdateEngineClient::RebootAfterUpdate() {
  reboot_after_update_call_count_++;
}

UpdateEngineClient::Status FakeUpdateEngineClient::GetLastStatus() {
  if (status_queue_.empty())
    return default_status_;

  UpdateEngineClient::Status last_status = status_queue_.front();
  status_queue_.pop();
  return last_status;
}

void FakeUpdateEngineClient::NotifyObserversThatStatusChanged(
    const UpdateEngineClient::Status& status) {
  for (auto& observer : observers_)
    observer.UpdateStatusChanged(status);
}

void FakeUpdateEngineClient::
    NotifyUpdateOverCellularOneTimePermissionGranted() {
  for (auto& observer : observers_)
    observer.OnUpdateOverCellularOneTimePermissionGranted();
}

void FakeUpdateEngineClient::SetChannel(const std::string& target_channel,
                                        bool is_powerwash_allowed) {
}

void FakeUpdateEngineClient::GetChannel(bool get_current_channel,
                                        const GetChannelCallback& callback) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(callback, std::string()));
}

void FakeUpdateEngineClient::GetEolStatus(GetEolStatusCallback callback) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback),
                                update_engine::EndOfLifeStatus::kSupported));
}

void FakeUpdateEngineClient::SetUpdateOverCellularPermission(
    bool allowed,
    const base::Closure& callback) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE, callback);
}

void FakeUpdateEngineClient::SetUpdateOverCellularOneTimePermission(
    const std::string& target_version,
    int64_t target_size,
    const UpdateOverCellularOneTimePermissionCallback& callback) {
  callback.Run(true);
}

void FakeUpdateEngineClient::set_default_status(
    const UpdateEngineClient::Status& status) {
  default_status_ = status;
}

void FakeUpdateEngineClient::set_update_check_result(
    const UpdateEngineClient::UpdateCheckResult& result) {
  update_check_result_ = result;
}

}  // namespace chromeos
