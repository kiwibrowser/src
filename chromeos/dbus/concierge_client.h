// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_CONCIERGE_CLIENT_H_
#define CHROMEOS_DBUS_CONCIERGE_CLIENT_H_

#include "chromeos/chromeos_export.h"
#include "chromeos/dbus/concierge/service.pb.h"
#include "chromeos/dbus/dbus_client.h"
#include "chromeos/dbus/dbus_method_call_status.h"
#include "dbus/object_proxy.h"

namespace chromeos {

// ConciergeClient is used to communicate with Concierge, which is used to
// start and stop VMs.
class CHROMEOS_EXPORT ConciergeClient : public DBusClient {
 public:
  class Observer {
   public:
    // OnContainerStarted is signaled by Concierge after the long-running
    // container startup process has been completed and the container is ready.
    virtual void OnContainerStarted(
        const vm_tools::concierge::ContainerStartedSignal& signal) = 0;

    // OnContainerStartupFailed is signaled by Concierge after the long-running
    // container startup process's failure is detected. Note the signal protocol
    // buffer type is the same as in OnContainerStarted.
    virtual void OnContainerStartupFailed(
        const vm_tools::concierge::ContainerStartedSignal& signal) = 0;

   protected:
    virtual ~Observer() = default;
  };

  // Adds an observer.
  virtual void AddObserver(Observer* observer) = 0;

  // Removes an observer if added.
  virtual void RemoveObserver(Observer* observer) = 0;

  // IsContainerStartedSignalConnected must return true before StartContainer
  // is called.
  virtual bool IsContainerStartedSignalConnected() = 0;

  // IsContainerStartupFailedSignalConnected must return true before
  // StartContainer is called.
  virtual bool IsContainerStartupFailedSignalConnected() = 0;

  // Creates a disk image for the Termina VM.
  // |callback| is called after the method call finishes.
  virtual void CreateDiskImage(
      const vm_tools::concierge::CreateDiskImageRequest& request,
      DBusMethodCallback<vm_tools::concierge::CreateDiskImageResponse>
          callback) = 0;

  // Destroys a Termina VM and removes its disk image.
  // |callback| is called after the method call finishes.
  virtual void DestroyDiskImage(
      const vm_tools::concierge::DestroyDiskImageRequest& request,
      DBusMethodCallback<vm_tools::concierge::DestroyDiskImageResponse>
          callback) = 0;

  // Starts a Termina VM if there is not alread one running.
  // |callback| is called after the method call finishes.
  virtual void StartTerminaVm(
      const vm_tools::concierge::StartVmRequest& request,
      DBusMethodCallback<vm_tools::concierge::StartVmResponse> callback) = 0;

  // Stops the named Termina VM if it is running.
  // |callback| is called after the method call finishes.
  virtual void StopVm(
      const vm_tools::concierge::StopVmRequest& request,
      DBusMethodCallback<vm_tools::concierge::StopVmResponse> callback) = 0;

  // Starts a Container inside an existing Termina VM.
  // |callback| is called after the method call finishes.
  virtual void StartContainer(
      const vm_tools::concierge::StartContainerRequest& request,
      DBusMethodCallback<vm_tools::concierge::StartContainerResponse>
          callback) = 0;

  // Launches an application inside a running Container.
  // |callback| is called after the method call finishes.
  virtual void LaunchContainerApplication(
      const vm_tools::concierge::LaunchContainerApplicationRequest& request,
      DBusMethodCallback<
          vm_tools::concierge::LaunchContainerApplicationResponse>
          callback) = 0;

  // Gets application icons from inside a Container.
  // |callback| is called after the method call finishes.
  virtual void GetContainerAppIcons(
      const vm_tools::concierge::ContainerAppIconRequest& request,
      DBusMethodCallback<vm_tools::concierge::ContainerAppIconResponse>
          callback) = 0;

  // Registers |callback| to run when the Concierge service becomes available.
  // If the service is already available, or if connecting to the name-owner-
  // changed signal fails, |callback| will be run once asynchronously.
  // Otherwise, |callback| will be run once in the future after the service
  // becomes available.
  virtual void WaitForServiceToBeAvailable(
      dbus::ObjectProxy::WaitForServiceToBeAvailableCallback callback) = 0;

  // Gets SSH server public key of container and trusted SSH client private key
  // which can be used to connect to the container.
  // |callback| is called after the method call finishes.
  virtual void GetContainerSshKeys(
      const vm_tools::concierge::ContainerSshKeysRequest& request,
      DBusMethodCallback<vm_tools::concierge::ContainerSshKeysResponse>
          callback) = 0;

  // Creates an instance of ConciergeClient.
  static ConciergeClient* Create();

  ~ConciergeClient() override;

 protected:
  // Create() should be used instead.
  ConciergeClient();

 private:
  DISALLOW_COPY_AND_ASSIGN(ConciergeClient);
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_CONCIERGE_CLIENT_H_
