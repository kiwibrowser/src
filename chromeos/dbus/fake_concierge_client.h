// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_FAKE_CONCIERGE_CLIENT_H_
#define CHROMEOS_DBUS_FAKE_CONCIERGE_CLIENT_H_

#include "base/observer_list.h"
#include "chromeos/dbus/concierge_client.h"

namespace chromeos {

// FakeConciergeClient is a light mock of ConciergeClient used for testing.
class CHROMEOS_EXPORT FakeConciergeClient : public ConciergeClient {
 public:
  FakeConciergeClient();
  ~FakeConciergeClient() override;

  // Adds an observer.
  void AddObserver(Observer* observer) override;

  // Removes an observer if added.
  void RemoveObserver(Observer* observer) override;

  // IsContainerStartedSignalConnected must return true before StartContainer
  // is called.
  bool IsContainerStartedSignalConnected() override;

  // IsContainerStartupFailedSignalConnected must return true before
  // StartContainer is called.
  bool IsContainerStartupFailedSignalConnected() override;

  // Fake version of the method that creates a disk image for the Termina VM.
  // Sets create_disk_image_called. |callback| is called after the method
  // call finishes.
  void CreateDiskImage(
      const vm_tools::concierge::CreateDiskImageRequest& request,
      DBusMethodCallback<vm_tools::concierge::CreateDiskImageResponse> callback)
      override;

  // Fake version of the method that destroys a Termina VM and removes its disk
  // image. Sets destroy_disk_image_called. |callback| is called after the
  // method call finishes.
  void DestroyDiskImage(
      const vm_tools::concierge::DestroyDiskImageRequest& request,
      DBusMethodCallback<vm_tools::concierge::DestroyDiskImageResponse>
          callback) override;

  // Fake version of the method that starts a Termina VM. Sets
  // start_termina_vm_called. |callback| is called after the method call
  // finishes.
  void StartTerminaVm(const vm_tools::concierge::StartVmRequest& request,
                      DBusMethodCallback<vm_tools::concierge::StartVmResponse>
                          callback) override;

  // Fake version of the method that stops the named Termina VM if it is
  // running. Sets stop_vm_called. |callback| is called after the method
  // call finishes.
  void StopVm(const vm_tools::concierge::StopVmRequest& request,
              DBusMethodCallback<vm_tools::concierge::StopVmResponse> callback)
      override;

  // Fake version of the method that starts a Container inside an existing
  // Termina VM. |callback| is called after the method call finishes.
  void StartContainer(
      const vm_tools::concierge::StartContainerRequest& request,
      DBusMethodCallback<vm_tools::concierge::StartContainerResponse> callback)
      override;

  // Fake version of the method that launches an application inside a running
  // Container. |callback| is called after the method call finishes.
  void LaunchContainerApplication(
      const vm_tools::concierge::LaunchContainerApplicationRequest& request,
      DBusMethodCallback<
          vm_tools::concierge::LaunchContainerApplicationResponse> callback)
      override;

  // Fake version of the method that gets application icons from inside a
  // Container. |callback| is called after the method call finishes.
  void GetContainerAppIcons(
      const vm_tools::concierge::ContainerAppIconRequest& request,
      DBusMethodCallback<vm_tools::concierge::ContainerAppIconResponse>
          callback) override;

  // Fake version of the method that waits for the Concierge service to be
  // availble.  |callback| is called after the method call finishes.
  void WaitForServiceToBeAvailable(
      dbus::ObjectProxy::WaitForServiceToBeAvailableCallback callback) override;

  // Fake version of the method that fetches ssh key information.
  // |callback| is called after the method call finishes.
  void GetContainerSshKeys(
      const vm_tools::concierge::ContainerSshKeysRequest& request,
      DBusMethodCallback<vm_tools::concierge::ContainerSshKeysResponse>
          callback) override;

  // Indicates whether CreateDiskImage has been called
  bool create_disk_image_called() const { return create_disk_image_called_; }
  // Indicates whether DestroyDiskImage has been called
  bool destroy_disk_image_called() const { return destroy_disk_image_called_; }
  // Indicates whether StartTerminaVm has been called
  bool start_termina_vm_called() const { return start_termina_vm_called_; }
  // Indicates whether StopVm has been called
  bool stop_vm_called() const { return stop_vm_called_; }
  // Indicates whether StartContainer has been called
  bool start_container_called() const { return start_container_called_; }
  // Set ContainerStartedSignalConnected state
  void set_container_started_signal_connected(bool connected) {
    is_container_started_signal_connected_ = connected;
  }
  // Set ContainerStartedSignalConnected state
  void set_container_startup_failed_signal_connected(bool connected) {
    is_container_startup_failed_signal_connected_ = connected;
  }

 protected:
  void Init(dbus::Bus* bus) override {}

 private:
  bool create_disk_image_called_ = false;
  bool destroy_disk_image_called_ = false;
  bool start_termina_vm_called_ = false;
  bool stop_vm_called_ = false;
  bool start_container_called_ = false;
  bool is_container_started_signal_connected_ = true;
  bool is_container_startup_failed_signal_connected_ = true;
  base::ObserverList<Observer> observer_list_;

  DISALLOW_COPY_AND_ASSIGN(FakeConciergeClient);
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_FAKE_CONCIERGE_CLIENT_H_
