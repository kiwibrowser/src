// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/crostini/crostini_manager.h"

#include <string>
#include <vector>

#include "base/bind.h"
#include "base/no_destructor.h"
#include "base/strings/stringprintf.h"
#include "base/sys_info.h"
#include "base/task_scheduler/post_task.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/crostini/crostini_remover.h"
#include "chrome/browser/chromeos/crostini/crostini_util.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/component_updater/cros_component_installer_chromeos.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/extensions/application_launch.h"
#include "chromeos/dbus/concierge_client.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/debug_daemon_client.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "content/public/browser/browser_thread.h"
#include "dbus/message.h"
#include "extensions/browser/extension_registry.h"
#include "net/base/escape.h"

namespace crostini {

namespace {

constexpr int64_t kMinimumDiskSize = 1ll * 1024 * 1024 * 1024;  // 1 GiB
constexpr base::FilePath::CharType kHomeDirectory[] =
    FILE_PATH_LITERAL("/home");

chromeos::ConciergeClient* GetConciergeClient() {
  return chromeos::DBusThreadManager::Get()->GetConciergeClient();
}

class CrostiniRestarter;

class CrostiniRestarterService : public KeyedService {
 public:
  CrostiniRestarterService() = default;
  ~CrostiniRestarterService() override = default;

  CrostiniManager::RestartId Register(
      std::string vm_name,
      std::string crypothome_id,
      std::string container_name,
      std::string container_username,
      CrostiniManager::RestartCrostiniCallback callback,
      CrostiniManager::RestartObserver* observer);

  void RunPendingCallbacks(CrostiniRestarter* restarter,
                           ConciergeClientResult result);

  // Aborts restart_id. A "next" restarter with the same  <vm_name,
  // container_name> will run, if there is one.
  void Abort(CrostiniManager::RestartId restart_id);

 private:
  void ErasePending(CrostiniRestarter* restarter);

  std::map<CrostiniManager::RestartId, scoped_refptr<CrostiniRestarter>>
      restarter_map_;

  // Restarts by <vm_name, container_name>. Only one restarter flow is actually
  // running. Other restarters will just have their callback called when the
  // running restarter completes.
  std::multimap<std::pair<std::string, std::string>, CrostiniManager::RestartId>
      pending_map_;
};

class CrostiniRestarterServiceFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static CrostiniRestarterService* GetForProfile(Profile* profile) {
    return static_cast<CrostiniRestarterService*>(
        GetInstance()->GetServiceForBrowserContext(profile, true));
  }
  static CrostiniRestarterServiceFactory* GetInstance() {
    static base::NoDestructor<CrostiniRestarterServiceFactory> factory;
    return factory.get();
  }

 private:
  friend class base::NoDestructor<CrostiniRestarterServiceFactory>;

  CrostiniRestarterServiceFactory()
      : BrowserContextKeyedServiceFactory(
            "CrostiniRestarterService",
            BrowserContextDependencyManager::GetInstance()) {}
  ~CrostiniRestarterServiceFactory() override = default;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override {
    return new CrostiniRestarterService();
  }
};

class CrostiniRestarter : public base::RefCountedThreadSafe<CrostiniRestarter> {
 public:
  CrostiniRestarter(CrostiniRestarterService* restarter_service,
                    std::string vm_name,
                    std::string cryptohome_id,
                    std::string container_name,
                    std::string container_username,
                    CrostiniManager::RestartCrostiniCallback callback)
      : vm_name_(std::move(vm_name)),
        cryptohome_id_(std::move(cryptohome_id)),
        container_name_(std::move(container_name)),
        container_username_(std::move(container_username)),
        callback_(std::move(callback)),
        restart_id_(next_restart_id_++),
        restarter_service_(restarter_service) {}

  void Restart() {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    if (is_aborted_)
      return;

    auto* cros_component_manager =
        g_browser_process->platform_part()->cros_component_manager();
    if (cros_component_manager) {
      cros_component_manager->Load(
          "cros-termina",
          component_updater::CrOSComponentManager::MountPolicy::kMount,
          base::BindOnce(&CrostiniRestarter::InstallImageLoaderFinished,
                         base::WrapRefCounted(this)));
    } else {
      // Running in test. We still PostTask to prevent races between observers
      // aborting.
      content::BrowserThread::PostTask(
          content::BrowserThread::UI, FROM_HERE,
          base::BindOnce(
              &CrostiniRestarter::InstallImageLoaderFinishedOnUIThread,
              base::WrapRefCounted(this),
              component_updater::CrOSComponentManager::Error::NONE,
              base::FilePath()));
    }
  }

  void AddObserver(CrostiniManager::RestartObserver* observer) {
    observer_list_.AddObserver(observer);
  }

  void RunCallback(ConciergeClientResult result) {
    std::move(callback_).Run(result);
  }

  void Abort() {
    is_aborted_ = true;
    observer_list_.Clear();
  }

  CrostiniManager::RestartId restart_id() const { return restart_id_; }
  std::string vm_name() const { return vm_name_; }
  std::string container_name() const { return container_name_; }

 private:
  friend class base::RefCountedThreadSafe<CrostiniRestarter>;

  ~CrostiniRestarter() {
    if (callback_) {
      LOG(ERROR) << "Destroying without having called the callback.";
    }
  }

  void FinishRestart(ConciergeClientResult result) {
    restarter_service_->RunPendingCallbacks(this, result);
  }

  static void InstallImageLoaderFinished(
      scoped_refptr<CrostiniRestarter> restarter,
      component_updater::CrOSComponentManager::Error error,
      const base::FilePath& result) {
    DCHECK(!content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
    if (restarter->is_aborted_)
      return;
    content::BrowserThread::PostTask(
        content::BrowserThread::UI, FROM_HERE,
        base::BindOnce(&CrostiniRestarter::InstallImageLoaderFinishedOnUIThread,
                       restarter, error, result));
  }

  void InstallImageLoaderFinishedOnUIThread(
      component_updater::CrOSComponentManager::Error error,
      const base::FilePath& result) {
    ConciergeClientResult client_result =
        error == component_updater::CrOSComponentManager::Error::NONE
            ? ConciergeClientResult::SUCCESS
            : ConciergeClientResult::CONTAINER_START_FAILED;
    // Tell observers.
    for (auto& observer : observer_list_) {
      observer.OnComponentLoaded(client_result);
    }
    if (is_aborted_)
      return;
    if (client_result != ConciergeClientResult::SUCCESS) {
      LOG(ERROR)
          << "Failed to install the cros-termina component with error code: "
          << static_cast<int>(error);
      FinishRestart(client_result);
      return;
    }
    CrostiniManager::GetInstance()->StartConcierge(
        base::BindOnce(&CrostiniRestarter::ConciergeStarted, this));
  }

  void ConciergeStarted(bool is_started) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    ConciergeClientResult client_result =
        is_started ? ConciergeClientResult::SUCCESS
                   : ConciergeClientResult::CONTAINER_START_FAILED;
    // Tell observers.
    for (auto& observer : observer_list_) {
      observer.OnConciergeStarted(client_result);
    }
    if (is_aborted_)
      return;
    if (!is_started) {
      LOG(ERROR) << "Failed to start Concierge service.";
      FinishRestart(client_result);
      return;
    }
    CrostiniManager::GetInstance()->CreateDiskImage(
        cryptohome_id_, base::FilePath(vm_name_),
        vm_tools::concierge::StorageLocation::STORAGE_CRYPTOHOME_ROOT,
        base::BindOnce(&CrostiniRestarter::CreateDiskImageFinished, this));
  }

  void CreateDiskImageFinished(ConciergeClientResult result,
                               const base::FilePath& result_path) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    // Tell observers.
    for (auto& observer : observer_list_) {
      observer.OnDiskImageCreated(result);
    }
    if (is_aborted_)
      return;
    if (result != ConciergeClientResult::SUCCESS) {
      LOG(ERROR) << "Failed to create disk image.";
      FinishRestart(result);
      return;
    }
    CrostiniManager::GetInstance()->StartTerminaVm(
        cryptohome_id_, vm_name_, result_path,
        base::BindOnce(&CrostiniRestarter::StartTerminaVmFinished, this));
  }

  void StartTerminaVmFinished(ConciergeClientResult result) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    // Tell observers.
    for (auto& observer : observer_list_) {
      observer.OnVmStarted(result);
    }
    if (is_aborted_)
      return;
    if (result != ConciergeClientResult::SUCCESS) {
      LOG(ERROR) << "Failed to Start Termina VM.";
      FinishRestart(result);
      return;
    }
    CrostiniManager::GetInstance()->StartContainer(
        vm_name_, container_name_, container_username_, cryptohome_id_,
        base::BindOnce(&CrostiniRestarter::StartContainerFinished, this));
  }

  void StartContainerFinished(ConciergeClientResult result) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    if (result != ConciergeClientResult::SUCCESS) {
      LOG(ERROR) << "Failed to start container.";
    }
    if (is_aborted_)
      return;
    FinishRestart(result);
  }

  std::string vm_name_;
  std::string cryptohome_id_;
  std::string container_name_;
  std::string container_username_;
  CrostiniManager::RestartCrostiniCallback callback_;
  base::ObserverList<CrostiniManager::RestartObserver> observer_list_;
  CrostiniManager::RestartId restart_id_;
  CrostiniRestarterService* restarter_service_;
  bool is_aborted_ = false;

  static CrostiniManager::RestartId next_restart_id_;
};

CrostiniManager::RestartId CrostiniRestarter::next_restart_id_ = 0;

CrostiniManager::RestartId CrostiniRestarterService::Register(
    std::string vm_name,
    std::string cryptohome_id,
    std::string container_name,
    std::string container_username,
    CrostiniManager::RestartCrostiniCallback callback,
    CrostiniManager::RestartObserver* observer) {
  auto restarter = base::MakeRefCounted<CrostiniRestarter>(
      this, std::move(vm_name), std::move(cryptohome_id),
      std::move(container_name), std::move(container_username),
      std::move(callback));
  if (observer)
    restarter->AddObserver(observer);
  auto key = std::make_pair(restarter->vm_name(), restarter->container_name());
  pending_map_.emplace(key, restarter->restart_id());
  restarter_map_[restarter->restart_id()] = restarter;
  if (pending_map_.count(key) > 1) {
    VLOG(1) << "Already restarting vm " << vm_name << ", container "
            << container_name;
  } else {
    restarter->Restart();
  }
  return restarter->restart_id();
}

void CrostiniRestarterService::RunPendingCallbacks(
    CrostiniRestarter* restarter,
    ConciergeClientResult result) {
  auto key = std::make_pair(restarter->vm_name(), restarter->container_name());
  auto range = pending_map_.equal_range(key);
  std::vector<scoped_refptr<CrostiniRestarter>> pending_restarters;
  // Erase first, because restarter->RunCallback() may modify our maps.
  for (auto it = range.first; it != range.second; ++it) {
    CrostiniManager::RestartId restart_id = it->second;
    pending_restarters.emplace_back(restarter_map_[restart_id]);
    restarter_map_.erase(restart_id);
  }
  pending_map_.erase(range.first, range.second);
  for (const auto& pending_restarter : pending_restarters) {
    pending_restarter->RunCallback(result);
  }
}

void CrostiniRestarterService::Abort(CrostiniManager::RestartId restart_id) {
  auto it = restarter_map_.find(restart_id);
  DCHECK(it != restarter_map_.end())
      << "Aborting a restarter that already finished";

  it->second->Abort();
  ErasePending(it->second.get());
  // Erasing |it| also invalidates |it|, so make a key from |it| now.
  auto key =
      std::make_pair(it->second->vm_name(), it->second->container_name());
  restarter_map_.erase(it);
  // Kick off the "next" (in no order) pending Restart() if any.
  auto pending_it = pending_map_.find(key);
  if (pending_it != pending_map_.end()) {
    auto restarter = restarter_map_[pending_it->second];
    restarter->Restart();
  }
}

void CrostiniRestarterService::ErasePending(CrostiniRestarter* restarter) {
  // Erase from pending_map_
  auto key = std::make_pair(restarter->vm_name(), restarter->container_name());
  auto range = pending_map_.equal_range(key);
  for (auto it = range.first; it != range.second; ++it) {
    if (it->second == restarter->restart_id()) {
      pending_map_.erase(it);
      return;
    }
  }
  NOTREACHED();
}

}  // namespace

// static
CrostiniManager* CrostiniManager::GetInstance() {
  return base::Singleton<CrostiniManager>::get();
}

CrostiniManager::CrostiniManager() : weak_ptr_factory_(this) {
  // ConciergeClient and its observer_list_ will be destroyed together.
  // We add, but don't need to remove the observer. (Doing so would force a
  // "destroyed before" dependency on the owner of ConciergeClient).
  GetConciergeClient()->AddObserver(this);
}

CrostiniManager::~CrostiniManager() {}

// static
bool CrostiniManager::IsCrosTerminaInstalled() {
  return !g_browser_process->platform_part()
              ->cros_component_manager()
              ->GetCompatiblePath("cros-termina")
              .empty();
}

void CrostiniManager::StartConcierge(StartConciergeCallback callback) {
  VLOG(1) << "Starting Concierge service";
  chromeos::DBusThreadManager::Get()->GetDebugDaemonClient()->StartConcierge(
      base::BindOnce(&CrostiniManager::OnStartConcierge,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void CrostiniManager::OnStartConcierge(StartConciergeCallback callback,
                                       bool success) {
  if (!success) {
    LOG(ERROR) << "Failed to start Concierge service";
    std::move(callback).Run(success);
    return;
  }
  VLOG(1) << "Concierge service started";
  VLOG(1) << "Waiting for Concierge to announce availability.";

  GetConciergeClient()->WaitForServiceToBeAvailable(std::move(callback));
}

void CrostiniManager::StopConcierge(StopConciergeCallback callback) {
  VLOG(1) << "Stopping Concierge service";
  chromeos::DBusThreadManager::Get()->GetDebugDaemonClient()->StopConcierge(
      base::BindOnce(&CrostiniManager::OnStopConcierge,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void CrostiniManager::OnStopConcierge(StopConciergeCallback callback,
                                      bool success) {
  if (!success) {
    LOG(ERROR) << "Failed to stop Concierge service";
  } else {
    VLOG(1) << "Concierge service stopped";
  }
  std::move(callback).Run(success);
}

void CrostiniManager::CreateDiskImage(
    const std::string& cryptohome_id,
    const base::FilePath& disk_path,
    vm_tools::concierge::StorageLocation storage_location,
    CreateDiskImageCallback callback) {
  if (cryptohome_id.empty()) {
    LOG(ERROR) << "Cryptohome id cannot be empty";
    std::move(callback).Run(ConciergeClientResult::CLIENT_ERROR,
                            base::FilePath());
    return;
  }

  std::string disk_path_string = disk_path.AsUTF8Unsafe();
  if (disk_path_string.empty()) {
    LOG(ERROR) << "Disk path cannot be empty";
    std::move(callback).Run(ConciergeClientResult::CLIENT_ERROR,
                            base::FilePath());
    return;
  }

  vm_tools::concierge::CreateDiskImageRequest request;
  request.set_cryptohome_id(std::move(cryptohome_id));
  request.set_disk_path(std::move(disk_path_string));
  // The type of disk image to be created.
  request.set_image_type(vm_tools::concierge::DISK_IMAGE_QCOW2);

  if (storage_location != vm_tools::concierge::STORAGE_CRYPTOHOME_ROOT &&
      storage_location != vm_tools::concierge::STORAGE_CRYPTOHOME_DOWNLOADS) {
    LOG(ERROR) << "'" << storage_location
               << "' is not a valid storage location";
    std::move(callback).Run(ConciergeClientResult::CLIENT_ERROR,
                            base::FilePath());
    return;
  }
  request.set_storage_location(storage_location);

  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&base::SysInfo::AmountOfFreeDiskSpace,
                     base::FilePath(kHomeDirectory)),
      base::BindOnce(&CrostiniManager::CreateDiskImageAfterSizeCheck,
                     weak_ptr_factory_.GetWeakPtr(), std::move(request),
                     std::move(callback)));
}

void CrostiniManager::CreateDiskImageAfterSizeCheck(
    vm_tools::concierge::CreateDiskImageRequest request,
    CreateDiskImageCallback callback,
    int64_t free_disk_size) {
  int64_t disk_size = (free_disk_size * 9) / 10;
  // Skip disk size check on dev box or trybots because
  // base::SysInfo::AmountOfFreeDiskSpace returns zero in testing.
  if (disk_size < kMinimumDiskSize && base::SysInfo::IsRunningOnChromeOS()) {
    LOG(ERROR) << "Insufficient disk available. Need to free "
               << kMinimumDiskSize - disk_size << " bytes";
    std::move(callback).Run(ConciergeClientResult::CLIENT_ERROR,
                            base::FilePath());
    return;
  }
  // The logical size of the new disk image, in bytes.
  request.set_disk_size(std::move(disk_size));

  GetConciergeClient()->CreateDiskImage(
      std::move(request),
      base::BindOnce(&CrostiniManager::OnCreateDiskImage,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void CrostiniManager::DestroyDiskImage(
    const std::string& cryptohome_id,
    const base::FilePath& disk_path,
    vm_tools::concierge::StorageLocation storage_location,
    DestroyDiskImageCallback callback) {
  if (cryptohome_id.empty()) {
    LOG(ERROR) << "Cryptohome id cannot be empty";
    std::move(callback).Run(ConciergeClientResult::CLIENT_ERROR);
    return;
  }

  std::string disk_path_string = disk_path.AsUTF8Unsafe();
  if (disk_path_string.empty()) {
    LOG(ERROR) << "Disk path cannot be empty";
    std::move(callback).Run(ConciergeClientResult::CLIENT_ERROR);
    return;
  }

  vm_tools::concierge::DestroyDiskImageRequest request;
  request.set_cryptohome_id(std::move(cryptohome_id));
  request.set_disk_path(std::move(disk_path_string));

  if (storage_location != vm_tools::concierge::STORAGE_CRYPTOHOME_ROOT &&
      storage_location != vm_tools::concierge::STORAGE_CRYPTOHOME_DOWNLOADS) {
    LOG(ERROR) << "'" << storage_location
               << "' is not a valid storage location";
    std::move(callback).Run(ConciergeClientResult::CLIENT_ERROR);
    return;
  }
  request.set_storage_location(storage_location);

  GetConciergeClient()->DestroyDiskImage(
      std::move(request),
      base::BindOnce(&CrostiniManager::OnDestroyDiskImage,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void CrostiniManager::StartTerminaVm(std::string owner_id,
                                     std::string name,
                                     const base::FilePath& disk_path,
                                     StartTerminaVmCallback callback) {
  if (owner_id.empty()) {
    LOG(ERROR) << "owner_id is required";
    std::move(callback).Run(ConciergeClientResult::CLIENT_ERROR);
    return;
  }

  if (name.empty()) {
    LOG(ERROR) << "name is required";
    std::move(callback).Run(ConciergeClientResult::CLIENT_ERROR);
    return;
  }

  std::string disk_path_string = disk_path.AsUTF8Unsafe();
  if (disk_path_string.empty()) {
    LOG(ERROR) << "Disk path cannot be empty";
    std::move(callback).Run(ConciergeClientResult::CLIENT_ERROR);
    return;
  }

  vm_tools::concierge::StartVmRequest request;
  request.set_name(std::move(name));
  request.set_start_termina(true);
  request.set_owner_id(std::move(owner_id));

  vm_tools::concierge::DiskImage* disk_image = request.add_disks();
  disk_image->set_path(std::move(disk_path_string));
  disk_image->set_image_type(vm_tools::concierge::DISK_IMAGE_QCOW2);
  disk_image->set_writable(true);
  disk_image->set_do_mount(false);

  GetConciergeClient()->StartTerminaVm(
      request,
      base::BindOnce(&CrostiniManager::OnStartTerminaVm,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void CrostiniManager::StopVm(Profile* profile,
                             std::string name,
                             StopVmCallback callback) {
  if (name.empty()) {
    LOG(ERROR) << "name is required";
    std::move(callback).Run(ConciergeClientResult::CLIENT_ERROR);
    return;
  }

  vm_tools::concierge::StopVmRequest request;
  request.set_owner_id(CryptohomeIdForProfile(profile));
  request.set_name(std::move(name));

  GetConciergeClient()->StopVm(
      std::move(request),
      base::BindOnce(&CrostiniManager::OnStopVm, weak_ptr_factory_.GetWeakPtr(),
                     std::move(callback)));
}

void CrostiniManager::StartContainer(std::string vm_name,
                                     std::string container_name,
                                     std::string container_username,
                                     std::string cryptohome_id,
                                     StartContainerCallback callback) {
  if (vm_name.empty()) {
    LOG(ERROR) << "vm_name is required";
    std::move(callback).Run(ConciergeClientResult::CLIENT_ERROR);
    return;
  }
  if (container_name.empty()) {
    LOG(ERROR) << "container_name is required";
    std::move(callback).Run(ConciergeClientResult::CLIENT_ERROR);
    return;
  }
  if (container_username.empty()) {
    LOG(ERROR) << "container_username is required";
    std::move(callback).Run(ConciergeClientResult::CLIENT_ERROR);
    return;
  }
  if (cryptohome_id.empty()) {
    LOG(ERROR) << "cryptohome_id is required";
    std::move(callback).Run(ConciergeClientResult::CLIENT_ERROR);
    return;
  }
  if (!GetConciergeClient()->IsContainerStartedSignalConnected()) {
    LOG(ERROR) << "Async call to StartContainer can't complete when signal "
                  "is not connected.";
    std::move(callback).Run(ConciergeClientResult::CLIENT_ERROR);
    return;
  }
  if (!GetConciergeClient()->IsContainerStartupFailedSignalConnected()) {
    LOG(ERROR) << "Async call to StartContainer can't complete when signal "
                  "is not connected.";
    std::move(callback).Run(ConciergeClientResult::CLIENT_ERROR);
    return;
  }
  vm_tools::concierge::StartContainerRequest request;
  request.set_vm_name(std::move(vm_name));
  request.set_container_name(std::move(container_name));
  request.set_container_username(std::move(container_username));
  request.set_cryptohome_id(std::move(cryptohome_id));

  GetConciergeClient()->StartContainer(
      std::move(request),
      base::BindOnce(&CrostiniManager::OnStartContainer,
                     weak_ptr_factory_.GetWeakPtr(), request.cryptohome_id(),
                     request.vm_name(), request.container_name(),
                     std::move(callback)));
}

void CrostiniManager::LaunchContainerApplication(
    Profile* profile,
    std::string vm_name,
    std::string container_name,
    std::string desktop_file_id,
    LaunchContainerApplicationCallback callback) {
  vm_tools::concierge::LaunchContainerApplicationRequest request;
  request.set_owner_id(CryptohomeIdForProfile(profile));
  request.set_vm_name(std::move(vm_name));
  request.set_container_name(std::move(container_name));
  request.set_desktop_file_id(std::move(desktop_file_id));

  GetConciergeClient()->LaunchContainerApplication(
      std::move(request),
      base::BindOnce(&CrostiniManager::OnLaunchContainerApplication,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void CrostiniManager::GetContainerAppIcons(
    Profile* profile,
    std::string vm_name,
    std::string container_name,
    std::vector<std::string> desktop_file_ids,
    int icon_size,
    int scale,
    GetContainerAppIconsCallback callback) {
  vm_tools::concierge::ContainerAppIconRequest request;
  request.set_owner_id(CryptohomeIdForProfile(profile));
  request.set_vm_name(std::move(vm_name));
  request.set_container_name(std::move(container_name));
  google::protobuf::RepeatedPtrField<std::string> ids(
      std::make_move_iterator(desktop_file_ids.begin()),
      std::make_move_iterator(desktop_file_ids.end()));
  request.mutable_desktop_file_ids()->Swap(&ids);
  request.set_size(icon_size);
  request.set_scale(scale);

  GetConciergeClient()->GetContainerAppIcons(
      std::move(request),
      base::BindOnce(&CrostiniManager::OnGetContainerAppIcons,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void CrostiniManager::GetContainerSshKeys(
    std::string vm_name,
    std::string container_name,
    std::string cryptohome_id,
    GetContainerSshKeysCallback callback) {
  vm_tools::concierge::ContainerSshKeysRequest request;
  request.set_vm_name(std::move(vm_name));
  request.set_container_name(std::move(container_name));
  request.set_cryptohome_id(std::move(cryptohome_id));

  GetConciergeClient()->GetContainerSshKeys(
      std::move(request),
      base::BindOnce(&CrostiniManager::OnGetContainerSshKeys,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void CrostiniManager::LaunchContainerTerminal(
    Profile* profile,
    const std::string& vm_name,
    const std::string& container_name) {
  std::string container_username = ContainerUserNameForProfile(profile);
  std::string vsh_crosh = base::StringPrintf(
      "chrome-extension://%s/html/crosh.html?command=vmshell",
      kCrostiniCroshBuiltinAppId);
  std::string vm_name_param = net::EscapeQueryParamValue(
      base::StringPrintf("--vm_name=%s", vm_name.c_str()), false);
  std::string lxd_dir =
      net::EscapeQueryParamValue("LXD_DIR=/mnt/stateful/lxd", false);
  std::string lxd_conf =
      net::EscapeQueryParamValue("LXD_CONF=/mnt/stateful/lxd_conf", false);
  const extensions::Extension* crosh_extension =
      extensions::ExtensionRegistry::Get(profile)->GetInstalledExtension(
          kCrostiniCroshBuiltinAppId);

  std::vector<base::StringPiece> pieces = {vsh_crosh,
                                           vm_name_param,
                                           "--",
                                           lxd_dir,
                                           lxd_conf,
                                           "run_container.sh",
                                           "--container_name",
                                           container_name,
                                           "--user",
                                           container_username,
                                           "--shell"};

  GURL vsh_in_crosh_url(base::JoinString(pieces, "&args[]="));

  AppLaunchParams launch_params(
      profile, crosh_extension, extensions::LAUNCH_CONTAINER_WINDOW,
      WindowOpenDisposition::NEW_WINDOW, extensions::SOURCE_APP_LAUNCHER);
  launch_params.override_app_name =
      AppNameFromCrostiniAppId(kCrostiniTerminalId);

  OpenApplicationWindow(launch_params, vsh_in_crosh_url);
}

CrostiniManager::RestartId CrostiniManager::RestartCrostini(
    Profile* profile,
    std::string vm_name,
    std::string container_name,
    RestartCrostiniCallback callback,
    RestartObserver* observer) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  return CrostiniRestarterServiceFactory::GetForProfile(profile)->Register(
      std::move(vm_name), CryptohomeIdForProfile(profile),
      std::move(container_name), ContainerUserNameForProfile(profile),
      std::move(callback), observer);
}

void CrostiniManager::AbortRestartCrostini(
    Profile* profile,
    CrostiniManager::RestartId restart_id) {
  CrostiniRestarterServiceFactory::GetForProfile(profile)->Abort(restart_id);
}

void CrostiniManager::OnCreateDiskImage(
    CreateDiskImageCallback callback,
    base::Optional<vm_tools::concierge::CreateDiskImageResponse> reply) {
  if (!reply.has_value()) {
    LOG(ERROR) << "Failed to create disk image. Empty response.";
    std::move(callback).Run(ConciergeClientResult::CREATE_DISK_IMAGE_FAILED,
                            base::FilePath());
    return;
  }
  vm_tools::concierge::CreateDiskImageResponse response = reply.value();

  if (response.status() != vm_tools::concierge::DISK_STATUS_EXISTS &&
      response.status() != vm_tools::concierge::DISK_STATUS_CREATED) {
    LOG(ERROR) << "Failed to create disk image: " << response.failure_reason();
    std::move(callback).Run(ConciergeClientResult::CREATE_DISK_IMAGE_FAILED,
                            base::FilePath());
    return;
  }

  std::move(callback).Run(ConciergeClientResult::SUCCESS,
                          base::FilePath(response.disk_path()));
}

void CrostiniManager::OnDestroyDiskImage(
    DestroyDiskImageCallback callback,
    base::Optional<vm_tools::concierge::DestroyDiskImageResponse> reply) {
  if (!reply.has_value()) {
    LOG(ERROR) << "Failed to destroy disk image. Empty response.";
    std::move(callback).Run(ConciergeClientResult::DESTROY_DISK_IMAGE_FAILED);
    return;
  }
  vm_tools::concierge::DestroyDiskImageResponse response =
      std::move(reply).value();

  if (response.status() != vm_tools::concierge::DISK_STATUS_DESTROYED &&
      response.status() != vm_tools::concierge::DISK_STATUS_DOES_NOT_EXIST) {
    LOG(ERROR) << "Failed to destroy disk image: " << response.failure_reason();
    std::move(callback).Run(ConciergeClientResult::DESTROY_DISK_IMAGE_FAILED);
    return;
  }

  std::move(callback).Run(ConciergeClientResult::SUCCESS);
}

void CrostiniManager::OnStartTerminaVm(
    StartTerminaVmCallback callback,
    base::Optional<vm_tools::concierge::StartVmResponse> reply) {
  if (!reply.has_value()) {
    LOG(ERROR) << "Failed to start termina vm. Empty response.";
    std::move(callback).Run(ConciergeClientResult::VM_START_FAILED);
    return;
  }
  vm_tools::concierge::StartVmResponse response = reply.value();

  if (!response.success()) {
    LOG(ERROR) << "Failed to start VM: " << response.failure_reason();
    std::move(callback).Run(ConciergeClientResult::VM_START_FAILED);
    return;
  }
  std::move(callback).Run(ConciergeClientResult::SUCCESS);
}

void CrostiniManager::OnStopVm(
    StopVmCallback callback,
    base::Optional<vm_tools::concierge::StopVmResponse> reply) {
  if (!reply.has_value()) {
    LOG(ERROR) << "Failed to stop termina vm. Empty response.";
    std::move(callback).Run(ConciergeClientResult::VM_STOP_FAILED);
    return;
  }
  vm_tools::concierge::StopVmResponse response = reply.value();

  if (!response.success()) {
    LOG(ERROR) << "Failed to stop VM: " << response.failure_reason();
    // TODO(rjwright): Change the service so that "Requested VM does not
    // exist" is not an error. "Requested VM does not exist" means that there
    // is a disk image for the VM but it is not running, either because it has
    // not been started or it has already been stopped. There's no need for
    // this to be an error, and making it a success will save us having to
    // discriminate on failure_reason here.
    if (response.failure_reason() != "Requested VM does not exist") {
      std::move(callback).Run(ConciergeClientResult::VM_STOP_FAILED);
      return;
    }
  }

  std::move(callback).Run(ConciergeClientResult::SUCCESS);
}

void CrostiniManager::OnStartContainer(
    std::string owner_id,
    std::string vm_name,
    std::string container_name,
    StartContainerCallback callback,
    base::Optional<vm_tools::concierge::StartContainerResponse> reply) {
  if (!reply.has_value()) {
    LOG(ERROR) << "Failed to start container in vm. Empty response.";
    std::move(callback).Run(ConciergeClientResult::CONTAINER_START_FAILED);
    return;
  }
  vm_tools::concierge::StartContainerResponse response = reply.value();
  if (response.status() == vm_tools::concierge::CONTAINER_STATUS_STARTING) {
    // The callback will be called when we receive the ContainerStated signal.
    start_container_callbacks_.emplace(
        std::make_tuple(owner_id, vm_name, container_name),
        std::move(callback));
    return;
  }
  if (response.status() != vm_tools::concierge::CONTAINER_STATUS_RUNNING) {
    LOG(ERROR) << "Failed to start container: " << response.failure_reason();
    std::move(callback).Run(ConciergeClientResult::CONTAINER_START_FAILED);
    return;
  }
  std::move(callback).Run(ConciergeClientResult::SUCCESS);
}

void CrostiniManager::OnContainerStarted(
    const vm_tools::concierge::ContainerStartedSignal& signal) {
  // Find the callbacks to call, then erase them from the map.
  std::string owner_id = signal.owner_id();
  // TODO(nverne): remove this check once Concierge always fills in owner_id.
  if (owner_id.empty()) {
    owner_id = CryptohomeIdForProfile(ProfileManager::GetPrimaryUserProfile());
  }
  auto range = start_container_callbacks_.equal_range(
      std::make_tuple(owner_id, signal.vm_name(), signal.container_name()));
  for (auto it = range.first; it != range.second; ++it) {
    std::move(it->second).Run(ConciergeClientResult::SUCCESS);
  }
  start_container_callbacks_.erase(range.first, range.second);
}

void CrostiniManager::OnContainerStartupFailed(
    const vm_tools::concierge::ContainerStartedSignal& signal) {
  // Find the callbacks to call, then erase them from the map.
  std::string owner_id = signal.owner_id();
  // TODO(nverne): remove this check once Concierge always fills in owner_id.
  if (owner_id.empty()) {
    owner_id = CryptohomeIdForProfile(ProfileManager::GetPrimaryUserProfile());
  }
  auto range = start_container_callbacks_.equal_range(
      std::make_tuple(owner_id, signal.vm_name(), signal.container_name()));
  for (auto it = range.first; it != range.second; ++it) {
    std::move(it->second).Run(ConciergeClientResult::CONTAINER_START_FAILED);
  }
  start_container_callbacks_.erase(range.first, range.second);
}

void CrostiniManager::OnLaunchContainerApplication(
    LaunchContainerApplicationCallback callback,
    base::Optional<vm_tools::concierge::LaunchContainerApplicationResponse>
        reply) {
  if (!reply.has_value()) {
    LOG(ERROR) << "Failed to launch application. Empty response.";
    std::move(callback).Run(
        ConciergeClientResult::LAUNCH_CONTAINER_APPLICATION_FAILED);
    return;
  }
  vm_tools::concierge::LaunchContainerApplicationResponse response =
      reply.value();

  if (!response.success()) {
    LOG(ERROR) << "Failed to launch application: " << response.failure_reason();
    std::move(callback).Run(
        ConciergeClientResult::LAUNCH_CONTAINER_APPLICATION_FAILED);
    return;
  }
  std::move(callback).Run(ConciergeClientResult::SUCCESS);
}

void CrostiniManager::OnGetContainerAppIcons(
    GetContainerAppIconsCallback callback,
    base::Optional<vm_tools::concierge::ContainerAppIconResponse> reply) {
  std::vector<Icon> icons;
  if (!reply.has_value()) {
    LOG(ERROR) << "Failed to get container application icons. Empty response.";
    std::move(callback).Run(ConciergeClientResult::DBUS_ERROR, icons);
    return;
  }
  vm_tools::concierge::ContainerAppIconResponse response = reply.value();
  for (auto& icon : *response.mutable_icons()) {
    icons.emplace_back(
        Icon{.desktop_file_id = std::move(*icon.mutable_desktop_file_id()),
             .content = std::move(*icon.mutable_icon())});
  }
  std::move(callback).Run(ConciergeClientResult::SUCCESS, icons);
}

void CrostiniManager::OnGetContainerSshKeys(
    GetContainerSshKeysCallback callback,
    base::Optional<vm_tools::concierge::ContainerSshKeysResponse> reply) {
  if (!reply.has_value()) {
    LOG(ERROR) << "Failed to get ssh keys. Empty response.";
    std::move(callback).Run(ConciergeClientResult::DBUS_ERROR, "", "");
    return;
  }
  vm_tools::concierge::ContainerSshKeysResponse response = reply.value();
  std::move(callback).Run(ConciergeClientResult::SUCCESS,
                          response.container_public_key(),
                          response.host_private_key());
}

void CrostiniManager::RemoveCrostini(Profile* profile,
                                     std::string vm_name,
                                     std::string container_name,
                                     RemoveCrostiniCallback callback) {
  auto crostini_remover = base::MakeRefCounted<CrostiniRemover>(
      profile, std::move(vm_name), std::move(container_name),
      std::move(callback));
  crostini_remover->RemoveCrostini();
}

}  // namespace crostini
