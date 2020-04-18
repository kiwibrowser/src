// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_CROSTINI_CROSTINI_MANAGER_H_
#define CHROME_BROWSER_CHROMEOS_CROSTINI_CROSTINI_MANAGER_H_

#include <map>
#include <utility>

#include "base/files/file_path.h"
#include "base/memory/singleton.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "chromeos/dbus/concierge/service.pb.h"
#include "chromeos/dbus/concierge_client.h"

class Profile;

namespace crostini {

// Result types for CrostiniManager::StartTerminaVmCallback etc.
enum class ConciergeClientResult {
  SUCCESS,
  DBUS_ERROR,
  UNPARSEABLE_RESPONSE,
  CREATE_DISK_IMAGE_FAILED,
  VM_START_FAILED,
  VM_STOP_FAILED,
  DESTROY_DISK_IMAGE_FAILED,
  CLIENT_ERROR,
  DISK_TYPE_ERROR,
  CONTAINER_START_FAILED,
  LAUNCH_CONTAINER_APPLICATION_FAILED,
  UNKNOWN_ERROR,
};

// Return type when getting app icons from within a container.
struct Icon {
  std::string desktop_file_id;

  // Icon file content in PNG format.
  std::string content;
};

// CrostiniManager is a singleton which is used to check arguments for the
// ConciergeClient. ConciergeClient is dedicated to communication with the
// Concierge service and should remain as thin as possible.
class CrostiniManager : public chromeos::ConciergeClient::Observer {
 public:
  using ConciergeClientCallback =
      base::OnceCallback<void(ConciergeClientResult result)>;

  // The type of the callback for CrostiniManager::StartConcierge.
  using StartConciergeCallback =
      base::OnceCallback<void(bool is_service_available)>;
  // The type of the callback for CrostiniManager::StopConcierge.
  using StopConciergeCallback = StartConciergeCallback;

  // The type of the callback for CrostiniManager::StartTerminaVm.
  using StartTerminaVmCallback = ConciergeClientCallback;
  // The type of the callback for CrostiniManager::CreateDiskImage.
  using CreateDiskImageCallback =
      base::OnceCallback<void(ConciergeClientResult result,
                              const base::FilePath& disk_path)>;
  // The type of the callback for CrostiniManager::DestroyDiskImage.
  using DestroyDiskImageCallback = ConciergeClientCallback;
  // The type of the callback for CrostiniManager::StopVm.
  using StopVmCallback = ConciergeClientCallback;
  // The type of the callback for CrostiniManager::StartContainer.
  using StartContainerCallback = ConciergeClientCallback;
  // The type of the callback for CrostiniManager::LaunchContainerApplication.
  using LaunchContainerApplicationCallback = ConciergeClientCallback;
  // The type of the callback for CrostiniManager::GetContainerAppIcons.
  using GetContainerAppIconsCallback =
      base::OnceCallback<void(ConciergeClientResult result,
                              std::vector<Icon>& icons)>;
  // The type of the callback for CrostiniManager::GetContainerSshKeys.
  using GetContainerSshKeysCallback =
      base::OnceCallback<void(ConciergeClientResult result,
                              const std::string& container_public_key,
                              const std::string& host_private_key)>;
  // The type of the callback for CrostiniManager::RestartCrostini.
  using RestartCrostiniCallback = ConciergeClientCallback;
  // The type of the callback for CrostiniManager::RemoveCrostini.
  using RemoveCrostiniCallback = ConciergeClientCallback;

  // Observer class for the Crostini restart flow.
  class RestartObserver {
   public:
    virtual ~RestartObserver() {}
    virtual void OnComponentLoaded(ConciergeClientResult result) = 0;
    virtual void OnConciergeStarted(ConciergeClientResult result) = 0;
    virtual void OnDiskImageCreated(ConciergeClientResult result) = 0;
    virtual void OnVmStarted(ConciergeClientResult result) = 0;
  };

  // Checks if the cros-termina component is installed.
  static bool IsCrosTerminaInstalled();

  // Starts the Concierge service. |callback| is called after the method call
  // finishes.
  void StartConcierge(StartConciergeCallback callback);

  // Stops the Concierge service. |callback| is called after the method call
  // finishes.
  void StopConcierge(StopConciergeCallback callback);

  // Checks the arguments for creating a new Termina VM disk image. Creates a
  // disk image for a Termina VM via ConciergeClient::CreateDiskImage.
  // |callback| is called if the arguments are bad, or after the method call
  // finishes.
  void CreateDiskImage(
      // The cryptohome id for the user's encrypted storage.
      const std::string& cryptohome_id,
      // The path to the disk image, including the name of
      // the image itself. The image name should match the
      // name of the VM that it will be used for.
      const base::FilePath& disk_path,
      // The storage location for the disk image
      vm_tools::concierge::StorageLocation storage_location,
      CreateDiskImageCallback callback);

  // Checks the arguments for destroying a named Termina VM disk image.
  // Removes the named Termina VM via ConciergeClient::DestroyDiskImage.
  // |callback| is called if the arguments are bad, or after the method call
  // finishes.
  void DestroyDiskImage(
      // The cryptohome id for the user's encrypted storage.
      const std::string& cryptohome_id,
      // The path to the disk image, including the name of
      // the image itself.
      const base::FilePath& disk_path,
      // The storage location of the disk image
      vm_tools::concierge::StorageLocation storage_location,
      DestroyDiskImageCallback callback);

  // Checks the arguments for starting a Termina VM. Starts a Termina VM via
  // ConciergeClient::StartTerminaVm. |callback| is called if the arguments
  // are bad, or after the method call finishes.
  void StartTerminaVm(std::string owner_id,
                      // The human-readable name to be assigned to this VM.
                      std::string name,
                      // Path to the disk image on the host.
                      const base::FilePath& disk_path,
                      StartTerminaVmCallback callback);

  // Checks the arguments for stopping a Termina VM. Stops the Termina VM via
  // ConciergeClient::StopVm. |callback| is called if the arguments are bad,
  // or after the method call finishes.
  void StopVm(Profile* profile, std::string name, StopVmCallback callback);

  // Checks the arguments for starting a Container inside an existing Termina
  // VM. Starts the container via ConciergeClient::StartContainer. |callback|
  // is called if the arguments are bad, or after the method call finishes.
  void StartContainer(std::string vm_name,
                      std::string container_name,
                      std::string container_username,
                      std::string cryptohome_id,
                      StartContainerCallback callback);

  // Asynchronously launches an app as specified by its desktop file id.
  // |callback| is called with SUCCESS when the relevant process is started
  // or LAUNCH_CONTAINER_APPLICATION_FAILED if there was an error somewhere.
  void LaunchContainerApplication(Profile* profile,
                                  std::string vm_name,
                                  std::string container_name,
                                  std::string desktop_file_id,
                                  LaunchContainerApplicationCallback callback);

  // Asynchronously gets app icons as specified by their desktop file ids.
  // |callback| is called after the method call finishes.
  void GetContainerAppIcons(Profile* profile,
                            std::string vm_name,
                            std::string container_name,
                            std::vector<std::string> desktop_file_ids,
                            int icon_size,
                            int scale,
                            GetContainerAppIconsCallback callback);

  // Asynchronously gets SSH server public key of container and trusted SSH
  // client private key which can be used to connect to the container.
  // |callback| is called after the method call finishes.
  void GetContainerSshKeys(std::string vm_name,
                           std::string container_name,
                           std::string cryptohome_id,
                           GetContainerSshKeysCallback callback);

  // Launches the crosh-in-a-window that displays a shell in an already running
  // container on a VM.
  void LaunchContainerTerminal(Profile* profile,
                               const std::string& vm_name,
                               const std::string& container_name);

  using RestartId = int;
  static const RestartId kUninitializedRestartId = -1;
  // Runs all the steps required to restart the given crostini vm and container.
  // The optional |observer| tracks progress.
  RestartId RestartCrostini(Profile* profile,
                            std::string vm_name,
                            std::string container_name,
                            RestartCrostiniCallback callback,
                            RestartObserver* observer = nullptr);

  void AbortRestartCrostini(Profile* profile, RestartId id);

  // ConciergeClient::Observer:
  void OnContainerStarted(
      const vm_tools::concierge::ContainerStartedSignal& signal) override;
  void OnContainerStartupFailed(
      const vm_tools::concierge::ContainerStartedSignal& signal) override;

  void RemoveCrostini(Profile* profile,
                      std::string vm_name,
                      std::string container_name,
                      RemoveCrostiniCallback callback);

  // Returns the singleton instance of CrostiniManager.
  static CrostiniManager* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<CrostiniManager>;

  CrostiniManager();
  ~CrostiniManager() override;

  // Callback for ConciergeClient::CreateDiskImage. Called after the Concierge
  // service method finishes.
  void OnCreateDiskImage(
      CreateDiskImageCallback callback,
      base::Optional<vm_tools::concierge::CreateDiskImageResponse> response);

  // Callback for ConciergeClient::DestroyDiskImage. Called after the Concierge
  // service method finishes.
  void OnDestroyDiskImage(
      DestroyDiskImageCallback callback,
      base::Optional<vm_tools::concierge::DestroyDiskImageResponse> response);

  // Callback for ConciergeClient::StartTerminaVm. Called after the Concierge
  // service method finishes.
  void OnStartTerminaVm(
      StartTerminaVmCallback callback,
      base::Optional<vm_tools::concierge::StartVmResponse> response);

  // Callback for ConciergeClient::StopVm. Called after the Concierge
  // service method finishes.
  void OnStopVm(StopVmCallback callback,
                base::Optional<vm_tools::concierge::StopVmResponse> response);

  // Callback for CrostiniClient::StartConcierge. Called after the
  // DebugDaemon service method finishes.
  void OnStartConcierge(StartConciergeCallback callback, bool success);

  // Callback for CrostiniClient::StopConcierge. Called after the
  // DebugDaemon service method finishes.
  void OnStopConcierge(StopConciergeCallback callback, bool success);

  // Callback for CrostiniManager::StartContainer. Called after the Concierge
  // service finishes.
  void OnStartContainer(
      std::string owner_id,
      std::string vm_name,
      std::string container_name,
      StartContainerCallback callback,
      base::Optional<vm_tools::concierge::StartContainerResponse> response);

  // Callback for CrostiniManager::LaunchContainerApplication. We don't use
  // the result of this currently so it doesn't take a callback.
  void OnLaunchContainerApplication(
      LaunchContainerApplicationCallback callback,
      base::Optional<vm_tools::concierge::LaunchContainerApplicationResponse>
          response);

  // Callback for CrostiniManager::GetContainerAppIcons. Called after the
  // Concierge service finishes.
  void OnGetContainerAppIcons(
      GetContainerAppIconsCallback callback,
      base::Optional<vm_tools::concierge::ContainerAppIconResponse> response);

  // Callback for CrostiniManager::GetContainerSshKeys. Called after the
  // Concierge service finishes.
  void OnGetContainerSshKeys(
      GetContainerSshKeysCallback callback,
      base::Optional<vm_tools::concierge::ContainerSshKeysResponse> response);

  // Helper for CrostiniManager::CreateDiskImage. Separated so it can be run
  // off the main thread.
  void CreateDiskImageAfterSizeCheck(
      vm_tools::concierge::CreateDiskImageRequest request,
      CreateDiskImageCallback callback,
      int64_t free_disk_size);

  // Pending StartContainer callbacks are keyed by <owner_id, vm_name,
  // container_name> string tuples.
  std::multimap<std::tuple<std::string, std::string, std::string>,
                StartContainerCallback>
      start_container_callbacks_;

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate its weak pointers before any other members are destroyed.
  base::WeakPtrFactory<CrostiniManager> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(CrostiniManager);
};

}  // namespace crostini

#endif  // CHROME_BROWSER_CHROMEOS_CROSTINI_CROSTINI_MANAGER_H_
