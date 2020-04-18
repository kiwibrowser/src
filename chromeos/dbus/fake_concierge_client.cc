// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/dbus/fake_concierge_client.h"

#include "base/threading/thread_task_runner_handle.h"

namespace chromeos {

FakeConciergeClient::FakeConciergeClient() {}
FakeConciergeClient::~FakeConciergeClient() = default;

// ConciergeClient override.
void FakeConciergeClient::AddObserver(Observer* observer) {
  observer_list_.AddObserver(observer);
}

// ConciergeClient override.
void FakeConciergeClient::RemoveObserver(Observer* observer) {
  observer_list_.RemoveObserver(observer);
}

// ConciergeClient override.
bool FakeConciergeClient::IsContainerStartedSignalConnected() {
  return is_container_started_signal_connected_;
}

// ConciergeClient override.
bool FakeConciergeClient::IsContainerStartupFailedSignalConnected() {
  return is_container_startup_failed_signal_connected_;
}

void FakeConciergeClient::CreateDiskImage(
    const vm_tools::concierge::CreateDiskImageRequest& request,
    DBusMethodCallback<vm_tools::concierge::CreateDiskImageResponse> callback) {
  create_disk_image_called_ = true;
  vm_tools::concierge::CreateDiskImageResponse response;
  response.set_status(vm_tools::concierge::DISK_STATUS_CREATED);
  response.set_disk_path("foo");
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), std::move(response)));
}

void FakeConciergeClient::DestroyDiskImage(
    const vm_tools::concierge::DestroyDiskImageRequest& request,
    DBusMethodCallback<vm_tools::concierge::DestroyDiskImageResponse>
        callback) {
  destroy_disk_image_called_ = true;
  vm_tools::concierge::DestroyDiskImageResponse response;
  response.set_status(vm_tools::concierge::DISK_STATUS_DESTROYED);
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), std::move(response)));
}

void FakeConciergeClient::StartTerminaVm(
    const vm_tools::concierge::StartVmRequest& request,
    DBusMethodCallback<vm_tools::concierge::StartVmResponse> callback) {
  start_termina_vm_called_ = true;
  vm_tools::concierge::StartVmResponse response;
  response.set_success(true);
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), std::move(response)));
}

void FakeConciergeClient::StopVm(
    const vm_tools::concierge::StopVmRequest& request,
    DBusMethodCallback<vm_tools::concierge::StopVmResponse> callback) {
  stop_vm_called_ = true;
  vm_tools::concierge::StopVmResponse response;
  response.set_success(true);
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), std::move(response)));
}

void FakeConciergeClient::StartContainer(
    const vm_tools::concierge::StartContainerRequest& request,
    DBusMethodCallback<vm_tools::concierge::StartContainerResponse> callback) {
  start_container_called_ = true;
  vm_tools::concierge::StartContainerResponse response;
  response.set_status(vm_tools::concierge::CONTAINER_STATUS_RUNNING);
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), std::move(response)));
}

void FakeConciergeClient::LaunchContainerApplication(
    const vm_tools::concierge::LaunchContainerApplicationRequest& request,
    DBusMethodCallback<vm_tools::concierge::LaunchContainerApplicationResponse>
        callback) {
  vm_tools::concierge::LaunchContainerApplicationResponse response;
  response.set_success(true);
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), std::move(response)));
}

void FakeConciergeClient::GetContainerAppIcons(
    const vm_tools::concierge::ContainerAppIconRequest& request,
    DBusMethodCallback<vm_tools::concierge::ContainerAppIconResponse>
        callback) {
  vm_tools::concierge::ContainerAppIconResponse response;
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), std::move(response)));
}

void FakeConciergeClient::WaitForServiceToBeAvailable(
    dbus::ObjectProxy::WaitForServiceToBeAvailableCallback callback) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), true));
}

void FakeConciergeClient::GetContainerSshKeys(
    const vm_tools::concierge::ContainerSshKeysRequest& request,
    DBusMethodCallback<vm_tools::concierge::ContainerSshKeysResponse>
        callback) {
  vm_tools::concierge::ContainerSshKeysResponse response;
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), std::move(response)));
}

}  // namespace chromeos
