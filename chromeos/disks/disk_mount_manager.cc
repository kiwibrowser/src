// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/disks/disk_mount_manager.h"

#include <stddef.h>
#include <stdint.h>

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/strings/string_util.h"
#include "chromeos/dbus/cros_disks_client.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/disks/suspend_unmount_manager.h"

namespace chromeos {
namespace disks {

namespace {

constexpr char kDefaultFormattedDeviceName[] = "UNTITLED";
constexpr char kDefaultFormatVFAT[] = "vfat";
constexpr char kDeviceNotFound[] = "Device could not be found";
constexpr char kStatefulPartition[] = "/mnt/stateful_partition";
DiskMountManager* g_disk_mount_manager = NULL;

// The DiskMountManager implementation.
class DiskMountManagerImpl : public DiskMountManager,
                             public CrosDisksClient::Observer {
 public:
  DiskMountManagerImpl() :
    already_refreshed_(false),
    weak_ptr_factory_(this) {
    DBusThreadManager* dbus_thread_manager = DBusThreadManager::Get();
    cros_disks_client_ = dbus_thread_manager->GetCrosDisksClient();
    PowerManagerClient* power_manager_client =
        dbus_thread_manager->GetPowerManagerClient();
    suspend_unmount_manager_.reset(
        new SuspendUnmountManager(this, power_manager_client));
    cros_disks_client_->AddObserver(this);
  }

  ~DiskMountManagerImpl() override { cros_disks_client_->RemoveObserver(this); }

  // DiskMountManager override.
  void AddObserver(DiskMountManager::Observer* observer) override {
    observers_.AddObserver(observer);
  }

  // DiskMountManager override.
  void RemoveObserver(DiskMountManager::Observer* observer) override {
    observers_.RemoveObserver(observer);
  }

  // DiskMountManager override.
  void MountPath(const std::string& source_path,
                 const std::string& source_format,
                 const std::string& mount_label,
                 const std::vector<std::string>& mount_options,
                 MountType type,
                 MountAccessMode access_mode) override {
    // Hidden and non-existent devices should not be mounted.
    if (type == MOUNT_TYPE_DEVICE) {
      DiskMap::const_iterator it = disks_.find(source_path);
      if (it == disks_.end() || it->second->is_hidden()) {
        OnMountCompleted(MountEntry(MOUNT_ERROR_INTERNAL, source_path, type,
                                    ""));
        return;
      }
    }
    cros_disks_client_->Mount(
        source_path, source_format, mount_label, mount_options, access_mode,
        REMOUNT_OPTION_MOUNT_NEW_DEVICE,
        base::BindOnce(&DiskMountManagerImpl::OnMount,
                       weak_ptr_factory_.GetWeakPtr(), source_path, type));

    // Record the access mode option passed to CrosDisks.
    // This is needed because CrosDisks service methods doesn't return the info
    // via DBus.
    access_modes_.insert(std::make_pair(source_path, access_mode));
  }

  // DiskMountManager override.
  void UnmountPath(const std::string& mount_path,
                   UnmountOptions options,
                   const UnmountPathCallback& callback) override {
    UnmountChildMounts(mount_path);
    cros_disks_client_->Unmount(
        mount_path, options,
        base::BindOnce(&DiskMountManagerImpl::OnUnmountPath,
                       weak_ptr_factory_.GetWeakPtr(), callback, mount_path));
  }

  void RemountAllRemovableDrives(MountAccessMode mode) override {
    // TODO(yamaguchi): Retry for tentative remount failures. crbug.com/661455
    for (const auto& device_path_and_disk : disks_) {
      const Disk& disk = *device_path_and_disk.second;
      if (disk.is_read_only_hardware()) {
        // Read-only devices can be mounted in RO mode only. No need to remount.
        continue;
      }
      if (!disk.is_mounted()) {
        continue;
      }
      RemountRemovableDrive(disk, mode);
    }
  }

  // DiskMountManager override.
  void FormatMountedDevice(const std::string& mount_path) override {
    MountPointMap::const_iterator mount_point = mount_points_.find(mount_path);
    if (mount_point == mount_points_.end()) {
      LOG(ERROR) << "Mount point with path \"" << mount_path << "\" not found.";
      OnFormatCompleted(FORMAT_ERROR_UNKNOWN, mount_path);
      return;
    }

    std::string device_path = mount_point->second.source_path;
    DiskMap::const_iterator disk = disks_.find(device_path);
    if (disk == disks_.end()) {
      LOG(ERROR) << "Device with path \"" << device_path << "\" not found.";
      OnFormatCompleted(FORMAT_ERROR_UNKNOWN, device_path);
      return;
    }
    if (disk->second->is_read_only()) {
      LOG(ERROR) << "Mount point with path \"" << mount_path
                 << "\" is read-only.";
      OnFormatCompleted(FORMAT_ERROR_DEVICE_NOT_ALLOWED, mount_path);
      return;
    }

    UnmountPath(disk->second->mount_path(),
                UNMOUNT_OPTIONS_NONE,
                base::Bind(&DiskMountManagerImpl::OnUnmountPathForFormat,
                           weak_ptr_factory_.GetWeakPtr(),
                           device_path));
  }

  void RenameMountedDevice(const std::string& mount_path,
                           const std::string& volume_name) override {
    MountPointMap::const_iterator mount_point = mount_points_.find(mount_path);
    if (mount_point == mount_points_.end()) {
      LOG(ERROR) << "Mount point with path '" << mount_path << "' not found.";
      OnRenameCompleted(RENAME_ERROR_UNKNOWN, mount_path);
      return;
    }

    std::string device_path = mount_point->second.source_path;
    DiskMap::const_iterator iter = disks_.find(device_path);
    if (iter == disks_.end()) {
      LOG(ERROR) << "Device with path '" << device_path << "' not found.";
      OnRenameCompleted(RENAME_ERROR_UNKNOWN, device_path);
      return;
    }
    if (iter->second->is_read_only()) {
      LOG(ERROR) << "Mount point with path '" << mount_path
                 << "' is read-only.";
      OnRenameCompleted(RENAME_ERROR_DEVICE_NOT_ALLOWED, mount_path);
      return;
    }

    UnmountPath(
        iter->second->mount_path(), UNMOUNT_OPTIONS_NONE,
        base::Bind(&DiskMountManagerImpl::OnUnmountPathForRename,
                   weak_ptr_factory_.GetWeakPtr(), device_path, volume_name));
  }

  // DiskMountManager override.
  void UnmountDeviceRecursively(
      const std::string& device_path,
      const UnmountDeviceRecursivelyCallbackType& callback) override {
    std::vector<std::string> devices_to_unmount;

    // Get list of all devices to unmount.
    int device_path_len = device_path.length();
    for (DiskMap::iterator it = disks_.begin(); it != disks_.end(); ++it) {
      if (!it->second->mount_path().empty() &&
          strncmp(device_path.c_str(), it->second->device_path().c_str(),
                  device_path_len) == 0) {
        devices_to_unmount.push_back(it->second->mount_path());
      }
    }

    // We should detect at least original device.
    if (devices_to_unmount.empty()) {
      if (disks_.find(device_path) == disks_.end()) {
        LOG(WARNING) << "Unmount recursive request failed for device "
                     << device_path << ", with error: " << kDeviceNotFound;
        callback.Run(false);
        return;
      }

      // Nothing to unmount.
      callback.Run(true);
      return;
    }

    // We will send the same callback data object to all Unmount calls and use
    // it to synchronize callbacks.
    // Note: this implementation has a potential memory leak issue. For
    // example if this instance is destructed before all the callbacks for
    // Unmount are invoked, the memory pointed by |cb_data| will be leaked.
    // It is because the UnmountDeviceRecursivelyCallbackData keeps how
    // many times OnUnmountDeviceRecursively callback is called and when
    // all the callbacks are called, |cb_data| will be deleted in the method.
    // However destructing the instance before all callback invocations will
    // cancel all pending callbacks, so that the |cb_data| would never be
    // deleted.
    // Fortunately, in the real scenario, the instance will be destructed
    // only for ShutDown. So, probably the memory would rarely be leaked.
    // TODO(hidehiko): Fix the issue.
    UnmountDeviceRecursivelyCallbackData* cb_data =
        new UnmountDeviceRecursivelyCallbackData(
            callback, devices_to_unmount.size());
    for (size_t i = 0; i < devices_to_unmount.size(); ++i) {
      cros_disks_client_->Unmount(
          devices_to_unmount[i], UNMOUNT_OPTIONS_NONE,
          base::BindOnce(&DiskMountManagerImpl::OnUnmountDeviceRecursively,
                         weak_ptr_factory_.GetWeakPtr(), cb_data,
                         devices_to_unmount[i]));
    }
  }

  // DiskMountManager override.
  void EnsureMountInfoRefreshed(
      const EnsureMountInfoRefreshedCallback& callback,
      bool force) override {
    if (!force && already_refreshed_) {
      callback.Run(true);
      return;
    }

    refresh_callbacks_.push_back(callback);
    if (refresh_callbacks_.size() == 1) {
      // If there's no in-flight refreshing task, start it.
      cros_disks_client_->EnumerateDevices(
          base::Bind(&DiskMountManagerImpl::RefreshAfterEnumerateDevices,
                     weak_ptr_factory_.GetWeakPtr()),
          base::Bind(&DiskMountManagerImpl::RefreshCompleted,
                     weak_ptr_factory_.GetWeakPtr(), false));
    }
  }

  // DiskMountManager override.
  const DiskMap& disks() const override { return disks_; }

  // DiskMountManager override.
  const Disk* FindDiskBySourcePath(
      const std::string& source_path) const override {
    DiskMap::const_iterator disk_it = disks_.find(source_path);
    return disk_it == disks_.end() ? NULL : disk_it->second.get();
  }

  // DiskMountManager override.
  const MountPointMap& mount_points() const override { return mount_points_; }

  // DiskMountManager override.
  bool AddDiskForTest(std::unique_ptr<Disk> disk) override {
    if (disks_.find(disk->device_path()) != disks_.end()) {
      LOG(ERROR) << "Attempt to add a duplicate disk";
      return false;
    }

    disks_.insert(std::make_pair(disk->device_path(), std::move(disk)));
    return true;
  }

  // DiskMountManager override.
  // Corresponding disk should be added to the manager before this is called.
  bool AddMountPointForTest(const MountPointInfo& mount_point) override {
    if (mount_points_.find(mount_point.mount_path) != mount_points_.end()) {
      LOG(ERROR) << "Attempt to add a duplicate mount point";
      return false;
    }
    if (mount_point.mount_type == chromeos::MOUNT_TYPE_DEVICE &&
        disks_.find(mount_point.source_path) == disks_.end()) {
      LOG(ERROR) << "Device mount points must have a disk entry.";
      return false;
    }

    mount_points_.insert(std::make_pair(mount_point.mount_path, mount_point));
    return true;
  }

 private:
  // A struct to represent information about a format changes.
  struct FormatChange {
    // new file system type
    std::string file_system_type;
    // New volume name
    std::string volume_name;
  };

  // Stores new volume name and file system type for a device on which
  // formatting is invoked on, so that OnFormatCompleted can set it back to
  // |disks_|. The key is a device_path and the value is a FormatChange.
  std::map<std::string, FormatChange> pending_format_changes_;

  // Stores new volume name for a device on which renaming is invoked on, so
  // that OnRenameCompleted can set it back to |disks_|. The key is a
  // device_path and the value is new volume_name.
  std::map<std::string, std::string> pending_rename_changes_;

  struct UnmountDeviceRecursivelyCallbackData {
    UnmountDeviceRecursivelyCallbackData(
        const UnmountDeviceRecursivelyCallbackType& in_callback,
        int in_num_pending_callbacks)
        : callback(in_callback),
          num_pending_callbacks(in_num_pending_callbacks) {
    }

    const UnmountDeviceRecursivelyCallbackType callback;
    size_t num_pending_callbacks;
  };

  // Called on D-Bus CrosDisksClient::Mount() is done.
  void OnMount(const std::string& source_path, MountType type, bool result) {
    // When succeeds, OnMountCompleted will be called by "MountCompleted",
    // signal instead. Do nothing now.
    if (result)
      return;

    OnMountCompleted(
        MountEntry(MOUNT_ERROR_INTERNAL, source_path, type, std::string()));
  }

  void RemountRemovableDrive(const Disk& disk,
                             MountAccessMode access_mode) {
    const std::string& mount_path = disk.mount_path();
    MountPointMap::const_iterator mount_point = mount_points_.find(mount_path);
    if (mount_point == mount_points_.end()) {
      // Not in mount_points_. This happens when the mount_points ans disks_ are
      // inconsistent.
      LOG(ERROR) << "Mount point with path \"" << mount_path << "\" not found.";
      OnMountCompleted(
          MountEntry(MOUNT_ERROR_PATH_NOT_MOUNTED, disk.device_path(),
                     MOUNT_TYPE_DEVICE, mount_path));
      return;
    }
    const std::string& source_path = mount_point->second.source_path;

    // Update the access mode option passed to CrosDisks.
    // This is needed because CrosDisks service methods doesn't return the info
    // via DBus, and must be updated before issuing Mount command as it'll be
    // read by the handler of MountCompleted signal.
    access_modes_[source_path] = access_mode;

    cros_disks_client_->Mount(
        mount_point->second.source_path, std::string(), std::string(), {},
        access_mode, REMOUNT_OPTION_REMOUNT_EXISTING_DEVICE,
        base::BindOnce(&DiskMountManagerImpl::OnMount,
                       weak_ptr_factory_.GetWeakPtr(), source_path,
                       mount_point->second.mount_type));
  }

  // Unmounts all mount points whose source path is transitively parented by
  // |mount_path|.
  void UnmountChildMounts(const std::string& mount_path_in) {
    std::string mount_path = mount_path_in;
    // Let's make sure mount path has trailing slash.
    if (mount_path.back() != '/')
      mount_path += '/';

    for (MountPointMap::iterator it = mount_points_.begin();
         it != mount_points_.end();
         ++it) {
      if (base::StartsWith(it->second.source_path, mount_path,
                           base::CompareCase::SENSITIVE)) {
        // TODO(tbarzic): Handle the case where this fails.
        UnmountPath(it->second.mount_path,
                    UNMOUNT_OPTIONS_NONE,
                    UnmountPathCallback());
      }
    }
  }

  // Callback for UnmountDeviceRecursively.
  void OnUnmountDeviceRecursively(UnmountDeviceRecursivelyCallbackData* cb_data,
                                  const std::string& mount_path,
                                  bool success) {
    if (success) {
      // Do standard processing for Unmount event.
      OnUnmountPath(UnmountPathCallback(), mount_path, true /* success */);
      VLOG(1) << mount_path <<  " unmounted.";
    }
    // This is safe as long as all callbacks are called on the same thread as
    // UnmountDeviceRecursively.
    cb_data->num_pending_callbacks--;

    if (cb_data->num_pending_callbacks == 0) {
      // This code has a problem that the |success| status used here is for the
      // last "unmount" callback, but not whether all unmounting is succeeded.
      // TODO(hidehiko): Fix the issue.
      cb_data->callback.Run(success);
      delete cb_data;
    }
  }

  // CrosDisksClient::Observer override.
  void OnMountCompleted(const MountEntry& entry) override {
    MountCondition mount_condition = MOUNT_CONDITION_NONE;
    if (entry.mount_type() == MOUNT_TYPE_DEVICE) {
      if (entry.error_code() == MOUNT_ERROR_UNKNOWN_FILESYSTEM) {
        mount_condition = MOUNT_CONDITION_UNKNOWN_FILESYSTEM;
      }
      if (entry.error_code() == MOUNT_ERROR_UNSUPPORTED_FILESYSTEM) {
        mount_condition = MOUNT_CONDITION_UNSUPPORTED_FILESYSTEM;
      }
    }
    const MountPointInfo mount_info(entry.source_path(),
                                    entry.mount_path(),
                                    entry.mount_type(),
                                    mount_condition);

    // If the device is corrupted but it's still possible to format it, it will
    // be fake mounted.
    if ((entry.error_code() == MOUNT_ERROR_NONE ||
         mount_info.mount_condition) &&
        mount_points_.find(mount_info.mount_path) == mount_points_.end()) {
      mount_points_.insert(MountPointMap::value_type(mount_info.mount_path,
                                                     mount_info));
    }

    if ((entry.error_code() == MOUNT_ERROR_NONE ||
         mount_info.mount_condition) &&
        mount_info.mount_type == MOUNT_TYPE_DEVICE &&
        !mount_info.source_path.empty() &&
        !mount_info.mount_path.empty()) {
      DiskMap::iterator iter = disks_.find(mount_info.source_path);
      if (iter != disks_.end()) {  // disk might have been removed by now?
        Disk* disk = iter->second.get();
        DCHECK(disk);
        // Currently the MountCompleted signal doesn't tell whether the device
        // is mounted in read-only mode or not. Instead use the mount option
        // recorded by DiskMountManagerImpl::MountPath().
        // |source_path| should be same as |disk->device_path| because
        // |VolumeManager::OnDiskEvent()| passes the latter to cros-disks as a
        // source path when mounting a device.
        AccessModeMap::iterator it = access_modes_.find(entry.source_path());

        // Store whether the disk was mounted in read-only mode due to a policy.
        disk->set_write_disabled_by_policy(
            it != access_modes_.end() && !disk->is_read_only_hardware()
                && it->second == MOUNT_ACCESS_MODE_READ_ONLY);
        disk->SetMountPath(mount_info.mount_path);
      }
    }
    // Observers may read the values of disks_. So notify them after tweaking
    // values of disks_.
    NotifyMountStatusUpdate(MOUNTING, entry.error_code(), mount_info);
  }

  // Callback for UnmountPath.
  void OnUnmountPath(const UnmountPathCallback& callback,
                     const std::string& mount_path,
                     bool success) {
    MountPointMap::iterator mount_points_it = mount_points_.find(mount_path);
    if (mount_points_it == mount_points_.end()) {
      // The path was unmounted, but not as a result of this unmount request,
      // so return error.
      if (!callback.is_null())
        callback.Run(MOUNT_ERROR_INTERNAL);
      return;
    }

    NotifyMountStatusUpdate(
        UNMOUNTING,
        success ? MOUNT_ERROR_NONE : MOUNT_ERROR_INTERNAL,
        MountPointInfo(mount_points_it->second.source_path,
                       mount_points_it->second.mount_path,
                       mount_points_it->second.mount_type,
                       mount_points_it->second.mount_condition));

    std::string path(mount_points_it->second.source_path);
    if (success)
      mount_points_.erase(mount_points_it);

    DiskMap::iterator disk_iter = disks_.find(path);
    if (disk_iter != disks_.end()) {
      DCHECK(disk_iter->second);
      if (success)
        disk_iter->second->clear_mount_path();
    }

    if (!callback.is_null())
      callback.Run(success ? MOUNT_ERROR_NONE : MOUNT_ERROR_INTERNAL);
  }

  void OnUnmountPathForFormat(const std::string& device_path,
                              MountError error_code) {
    if (error_code == MOUNT_ERROR_NONE &&
        disks_.find(device_path) != disks_.end()) {
      FormatUnmountedDevice(device_path);
    } else {
      OnFormatCompleted(FORMAT_ERROR_UNKNOWN, device_path);
    }
  }

  // Starts device formatting.
  void FormatUnmountedDevice(const std::string& device_path) {
    DiskMap::const_iterator disk = disks_.find(device_path);
    DCHECK(disk != disks_.end() && disk->second->mount_path().empty());

    pending_format_changes_[device_path] = {kDefaultFormatVFAT,
                                            kDefaultFormattedDeviceName};

    cros_disks_client_->Format(
        device_path, kDefaultFormatVFAT,
        base::BindOnce(&DiskMountManagerImpl::OnFormatStarted,
                       weak_ptr_factory_.GetWeakPtr(), device_path));
  }

  // Callback for Format.
  void OnFormatStarted(const std::string& device_path, bool success) {
    if (!success) {
      OnFormatCompleted(FORMAT_ERROR_UNKNOWN, device_path);
      return;
    }

    NotifyFormatStatusUpdate(FORMAT_STARTED, FORMAT_ERROR_NONE, device_path);
  }

  // CrosDisksClient::Observer override.
  void OnFormatCompleted(FormatError error_code,
                         const std::string& device_path) override {
    auto iter = disks_.find(device_path);

    // disk might have been removed by now?
    if (iter != disks_.end()) {
      Disk* disk = iter->second.get();
      DCHECK(disk);

      auto pending_change = pending_format_changes_.find(device_path);
      if (pending_change != pending_format_changes_.end() &&
          error_code == FORMAT_ERROR_NONE) {
        disk->set_device_label(pending_change->second.volume_name);
        disk->set_file_system_type(pending_change->second.file_system_type);
      }
    }

    pending_format_changes_.erase(device_path);

    NotifyFormatStatusUpdate(FORMAT_COMPLETED, error_code, device_path);
  }

  void OnUnmountPathForRename(const std::string& device_path,
                              const std::string& volume_name,
                              MountError error_code) {
    if (error_code != MOUNT_ERROR_NONE ||
        disks_.find(device_path) == disks_.end()) {
      OnRenameCompleted(RENAME_ERROR_UNKNOWN, device_path);
      return;
    }

    RenameUnmountedDevice(device_path, volume_name);
  }

  // Start device renaming
  void RenameUnmountedDevice(const std::string& device_path,
                             const std::string& volume_name) {
    DiskMap::const_iterator disk = disks_.find(device_path);
    DCHECK(disk != disks_.end() && disk->second->mount_path().empty());

    pending_rename_changes_[device_path] = volume_name;
    cros_disks_client_->Rename(
        device_path, volume_name,
        base::BindOnce(&DiskMountManagerImpl::OnRenameStarted,
                       weak_ptr_factory_.GetWeakPtr(), device_path));
  }

  // Callback for Rename.
  void OnRenameStarted(const std::string& device_path, bool success) {
    if (!success) {
      OnRenameCompleted(RENAME_ERROR_UNKNOWN, device_path);
      return;
    }

    NotifyRenameStatusUpdate(RENAME_STARTED, RENAME_ERROR_NONE, device_path);
  }

  // CrosDisksClient::Observer override.
  void OnRenameCompleted(RenameError error_code,
                         const std::string& device_path) override {
    auto iter = disks_.find(device_path);

    // disk might have been removed by now?
    if (iter != disks_.end()) {
      Disk* disk = iter->second.get();
      DCHECK(disk);

      auto pending_change = pending_rename_changes_.find(device_path);
      if (pending_change != pending_rename_changes_.end() &&
          error_code == RENAME_ERROR_NONE)
        disk->set_device_label(pending_change->second);
    }

    pending_rename_changes_.erase(device_path);

    NotifyRenameStatusUpdate(RENAME_COMPLETED, error_code, device_path);
  }

  // Callback for GetDeviceProperties.
  void OnGetDeviceProperties(const DiskInfo& disk_info) {
    if (disk_info.is_virtual())
      return;

    DVLOG(1) << "Found disk " << disk_info.device_path();
    // Delete previous disk info for this path:
    bool is_new = true;
    std::string base_mount_path = std::string();
    DiskMap::iterator iter = disks_.find(disk_info.device_path());
    if (iter != disks_.end()) {
      base_mount_path = iter->second->base_mount_path();
      disks_.erase(iter);
      is_new = false;
    }

    // If the device was mounted by the instance, apply recorded parameter.
    // Otherwise, default to false.
    // Lookup by |device_path| which we pass to cros-disks when mounting a
    // device in |VolumeManager::OnDiskEvent()|.
    auto access_mode = access_modes_.find(disk_info.device_path());
    bool write_disabled_by_policy = access_mode != access_modes_.end()
        && access_mode->second == chromeos::MOUNT_ACCESS_MODE_READ_ONLY;
    // TODO(agawronska): Add constructor for Disk from DiskInfo. Introduce Disk
    // builder class for tests.
    Disk* disk = new Disk(
        disk_info.device_path(), disk_info.mount_path(),
        write_disabled_by_policy, disk_info.system_path(),
        disk_info.file_path(), disk_info.label(), disk_info.drive_label(),
        disk_info.vendor_id(), disk_info.vendor_name(), disk_info.product_id(),
        disk_info.product_name(), disk_info.uuid(),
        FindSystemPathPrefix(disk_info.system_path()), disk_info.device_type(),
        disk_info.total_size_in_bytes(), disk_info.is_drive(),
        disk_info.is_read_only(), disk_info.has_media(),
        disk_info.on_boot_device(), disk_info.on_removable_device(),
        disk_info.is_hidden(), disk_info.file_system_type(), base_mount_path);
    disks_.insert(
        std::make_pair(disk_info.device_path(), base::WrapUnique(disk)));
    NotifyDiskStatusUpdate(is_new ? DISK_ADDED : DISK_CHANGED, *disk);
  }

  // Part of EnsureMountInfoRefreshed(). Called after the list of devices are
  // enumerated.
  void RefreshAfterEnumerateDevices(const std::vector<std::string>& devices) {
    std::set<std::string> current_device_set(devices.begin(), devices.end());
    for (DiskMap::iterator iter = disks_.begin(); iter != disks_.end(); ) {
      if (current_device_set.find(iter->first) == current_device_set.end()) {
        disks_.erase(iter++);
      } else {
        ++iter;
      }
    }
    RefreshDeviceAtIndex(devices, 0);
  }

  // Part of EnsureMountInfoRefreshed(). Called for each device to refresh info.
  void RefreshDeviceAtIndex(const std::vector<std::string>& devices,
                            size_t index) {
    if (index == devices.size()) {
      // All devices info retrieved. Proceed to enumerate mount point info.
      cros_disks_client_->EnumerateMountEntries(
          base::Bind(&DiskMountManagerImpl::RefreshAfterEnumerateMountEntries,
                     weak_ptr_factory_.GetWeakPtr()),
          base::Bind(&DiskMountManagerImpl::RefreshCompleted,
                     weak_ptr_factory_.GetWeakPtr(), false));
      return;
    }

    cros_disks_client_->GetDeviceProperties(
        devices[index],
        base::Bind(&DiskMountManagerImpl::RefreshAfterGetDeviceProperties,
                   weak_ptr_factory_.GetWeakPtr(), devices, index + 1),
        base::Bind(&DiskMountManagerImpl::RefreshDeviceAtIndex,
                   weak_ptr_factory_.GetWeakPtr(), devices, index + 1));
  }

  // Part of EnsureMountInfoRefreshed().
  void RefreshAfterGetDeviceProperties(const std::vector<std::string>& devices,
                                       size_t next_index,
                                       const DiskInfo& disk_info) {
    OnGetDeviceProperties(disk_info);
    RefreshDeviceAtIndex(devices, next_index);
  }

  // Part of EnsureMountInfoRefreshed(). Called after mount entries are listed.
  void RefreshAfterEnumerateMountEntries(
      const std::vector<MountEntry>& entries) {
    for (size_t i = 0; i < entries.size(); ++i)
      OnMountCompleted(entries[i]);
    RefreshCompleted(true);
  }

  // Part of EnsureMountInfoRefreshed(). Called when the refreshing is done.
  void RefreshCompleted(bool success) {
    already_refreshed_ = true;
    for (size_t i = 0; i < refresh_callbacks_.size(); ++i)
      refresh_callbacks_[i].Run(success);
    refresh_callbacks_.clear();
  }

  // CrosDisksClient::Observer override.
  void OnMountEvent(MountEventType event,
                    const std::string& device_path_arg) override {
    // Take a copy of the argument so we can modify it below.
    std::string device_path = device_path_arg;
    switch (event) {
      case CROS_DISKS_DISK_ADDED: {
        cros_disks_client_->GetDeviceProperties(
            device_path,
            base::Bind(&DiskMountManagerImpl::OnGetDeviceProperties,
                       weak_ptr_factory_.GetWeakPtr()),
            base::DoNothing());
        break;
      }
      case CROS_DISKS_DISK_REMOVED: {
        // Search and remove disks that are no longer present.
        DiskMountManager::DiskMap::iterator iter = disks_.find(device_path);
        if (iter != disks_.end()) {
          Disk* disk = iter->second.get();
          NotifyDiskStatusUpdate(DISK_REMOVED, *disk);
          disks_.erase(iter);
        }
        break;
      }
      case CROS_DISKS_DEVICE_ADDED: {
        system_path_prefixes_.insert(device_path);
        NotifyDeviceStatusUpdate(DEVICE_ADDED, device_path);
        break;
      }
      case CROS_DISKS_DEVICE_REMOVED: {
        system_path_prefixes_.erase(device_path);
        NotifyDeviceStatusUpdate(DEVICE_REMOVED, device_path);
        break;
      }
      case CROS_DISKS_DEVICE_SCANNED: {
        NotifyDeviceStatusUpdate(DEVICE_SCANNED, device_path);
        break;
      }
      default: {
        LOG(ERROR) << "Unknown event: " << event;
      }
    }
  }

  // Notifies all observers about disk status update.
  void NotifyDiskStatusUpdate(DiskEvent event, const Disk& disk) {
    for (auto& observer : observers_) {
      disk.IsAutoMountable() ? observer.OnAutoMountableDiskEvent(event, disk)
                             : observer.OnBootDeviceDiskEvent(event, disk);
    }
  }

  // Notifies all observers about device status update.
  void NotifyDeviceStatusUpdate(DeviceEvent event,
                                const std::string& device_path) {
    for (auto& observer : observers_)
      observer.OnDeviceEvent(event, device_path);
  }

  // Notifies all observers about mount completion.
  void NotifyMountStatusUpdate(MountEvent event,
                               MountError error_code,
                               const MountPointInfo& mount_info) {
    for (auto& observer : observers_)
      observer.OnMountEvent(event, error_code, mount_info);
  }

  void NotifyFormatStatusUpdate(FormatEvent event,
                                FormatError error_code,
                                const std::string& device_path) {
    for (auto& observer : observers_)
      observer.OnFormatEvent(event, error_code, device_path);
  }

  void NotifyRenameStatusUpdate(RenameEvent event,
                                RenameError error_code,
                                const std::string& device_path) {
    for (auto& observer : observers_)
      observer.OnRenameEvent(event, error_code, device_path);
  }

  // Finds system path prefix from |system_path|.
  const std::string& FindSystemPathPrefix(const std::string& system_path) {
    if (system_path.empty())
      return base::EmptyString();
    for (SystemPathPrefixSet::const_iterator it = system_path_prefixes_.begin();
         it != system_path_prefixes_.end();
         ++it) {
      const std::string& prefix = *it;
      if (base::StartsWith(system_path, prefix, base::CompareCase::SENSITIVE))
        return prefix;
    }
    return base::EmptyString();
  }

  // Mount event change observers.
  base::ObserverList<DiskMountManager::Observer> observers_;

  CrosDisksClient* cros_disks_client_;

  // The list of disks found.
  DiskMountManager::DiskMap disks_;

  DiskMountManager::MountPointMap mount_points_;

  typedef std::set<std::string> SystemPathPrefixSet;
  SystemPathPrefixSet system_path_prefixes_;

  bool already_refreshed_;
  std::vector<EnsureMountInfoRefreshedCallback> refresh_callbacks_;

  std::unique_ptr<SuspendUnmountManager> suspend_unmount_manager_;

  // Whether the instance attempted to mount a device in read-only mode for
  // each source path.
  typedef std::map<std::string, chromeos::MountAccessMode> AccessModeMap;
  AccessModeMap access_modes_;

  base::WeakPtrFactory<DiskMountManagerImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(DiskMountManagerImpl);
};

}  // namespace

DiskMountManager::Disk::Disk(const std::string& device_path,
                             const std::string& mount_path,
                             bool write_disabled_by_policy,
                             const std::string& system_path,
                             const std::string& file_path,
                             const std::string& device_label,
                             const std::string& drive_label,
                             const std::string& vendor_id,
                             const std::string& vendor_name,
                             const std::string& product_id,
                             const std::string& product_name,
                             const std::string& fs_uuid,
                             const std::string& system_path_prefix,
                             DeviceType device_type,
                             uint64_t total_size_in_bytes,
                             bool is_parent,
                             bool is_read_only_hardware,
                             bool has_media,
                             bool on_boot_device,
                             bool on_removable_device,
                             bool is_hidden,
                             const std::string& file_system_type,
                             const std::string& base_mount_path)
    : device_path_(device_path),
      mount_path_(mount_path),
      write_disabled_by_policy_(write_disabled_by_policy),
      system_path_(system_path),
      file_path_(file_path),
      device_label_(device_label),
      drive_label_(drive_label),
      vendor_id_(vendor_id),
      vendor_name_(vendor_name),
      product_id_(product_id),
      product_name_(product_name),
      fs_uuid_(fs_uuid),
      system_path_prefix_(system_path_prefix),
      device_type_(device_type),
      total_size_in_bytes_(total_size_in_bytes),
      is_parent_(is_parent),
      is_read_only_hardware_(is_read_only_hardware),
      has_media_(has_media),
      on_boot_device_(on_boot_device),
      on_removable_device_(on_removable_device),
      is_hidden_(is_hidden),
      file_system_type_(file_system_type),
      base_mount_path_(base_mount_path) {}

DiskMountManager::Disk::Disk(const Disk& other) = default;

DiskMountManager::Disk::~Disk() = default;

void DiskMountManager::Disk::SetMountPath(const std::string& mount_path) {
  mount_path_ = mount_path;

  if (base_mount_path_.empty())
    base_mount_path_ = mount_path;
}

bool DiskMountManager::Disk::IsAutoMountable() const {
  // Disks are considered auto-mountable if they are:
  // 1. Non-virtual
  // 2. Not on boot device
  // Only the second condition is checked here, because Disks are created from
  // non-virtual mount devices only.
  return !on_boot_device_;
}

bool DiskMountManager::Disk::IsStatefulPartition() const {
  return mount_path_ == kStatefulPartition;
}

bool DiskMountManager::AddDiskForTest(std::unique_ptr<Disk> disk) {
  return false;
}

bool DiskMountManager::AddMountPointForTest(const MountPointInfo& mount_point) {
  return false;
}

// static
std::string DiskMountManager::MountConditionToString(MountCondition condition) {
  switch (condition) {
    case MOUNT_CONDITION_NONE:
      return "";
    case MOUNT_CONDITION_UNKNOWN_FILESYSTEM:
      return "unknown_filesystem";
    case MOUNT_CONDITION_UNSUPPORTED_FILESYSTEM:
      return "unsupported_filesystem";
    default:
      NOTREACHED();
  }
  return "";
}

// static
std::string DiskMountManager::DeviceTypeToString(DeviceType type) {
  switch (type) {
    case DEVICE_TYPE_USB:
      return "usb";
    case DEVICE_TYPE_SD:
      return "sd";
    case DEVICE_TYPE_OPTICAL_DISC:
      return "optical";
    case DEVICE_TYPE_MOBILE:
      return "mobile";
    default:
      return "unknown";
  }
}

// static
void DiskMountManager::Initialize() {
  if (g_disk_mount_manager) {
    LOG(WARNING) << "DiskMountManager was already initialized";
    return;
  }
  g_disk_mount_manager = new DiskMountManagerImpl();
  VLOG(1) << "DiskMountManager initialized";
}

// static
void DiskMountManager::InitializeForTesting(
    DiskMountManager* disk_mount_manager) {
  if (g_disk_mount_manager) {
    LOG(WARNING) << "DiskMountManager was already initialized";
    return;
  }
  g_disk_mount_manager = disk_mount_manager;
  VLOG(1) << "DiskMountManager initialized";
}

// static
void DiskMountManager::Shutdown() {
  if (!g_disk_mount_manager) {
    LOG(WARNING) << "DiskMountManager::Shutdown() called with NULL manager";
    return;
  }
  delete g_disk_mount_manager;
  g_disk_mount_manager = NULL;
  VLOG(1) << "DiskMountManager Shutdown completed";
}

// static
DiskMountManager* DiskMountManager::GetInstance() {
  return g_disk_mount_manager;
}

}  // namespace disks
}  // namespace chromeos
