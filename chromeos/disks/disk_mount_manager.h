// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DISKS_DISK_MOUNT_MANAGER_H_
#define CHROMEOS_DISKS_DISK_MOUNT_MANAGER_H_

#include <stdint.h>

#include <map>
#include <memory>

#include "base/callback_forward.h"
#include "base/files/file_path.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/dbus/cros_disks_client.h"

namespace chromeos {
namespace disks {

// Condition of mounted filesystem.
enum MountCondition {
  MOUNT_CONDITION_NONE,
  MOUNT_CONDITION_UNKNOWN_FILESYSTEM,
  MOUNT_CONDITION_UNSUPPORTED_FILESYSTEM,
};

// This class handles the interaction with cros-disks.
// Other classes can add themselves as observers.
class CHROMEOS_EXPORT DiskMountManager {
 public:
  // Event types passed to the observers.
  enum DiskEvent {
    DISK_ADDED,
    DISK_REMOVED,
    DISK_CHANGED,
  };

  enum DeviceEvent {
    DEVICE_ADDED,
    DEVICE_REMOVED,
    DEVICE_SCANNED,
  };

  enum MountEvent {
    MOUNTING,
    UNMOUNTING,
  };

  enum FormatEvent {
    FORMAT_STARTED,
    FORMAT_COMPLETED
  };

  enum RenameEvent { RENAME_STARTED, RENAME_COMPLETED };

  // Used to house an instance of each found mount device. Created from
  // non-virtual mount devices only - see IsAutoMountable().
  class Disk {
   public:
    Disk(const std::string& device_path,
         // The path to the mount point of this device. Empty if not mounted.
         // (e.g. /media/removable/VOLUME)
         const std::string& mount_path,
         // Whether the device is mounted in read-only mode by the policy.
         // Valid only when the device mounted and mount_path_ is non-empty.
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
         const std::string& base_mount_path);
    Disk(const Disk& other);
    ~Disk();

    // The path of the device, used by devicekit-disks.
    // (e.g. /sys/devices/pci0000:00/.../8:0:0:0/block/sdb/sdb1)
    const std::string& device_path() const { return device_path_; }

    // The path to the mount point of this device. Will be empty if not mounted.
    // (e.g. /media/removable/VOLUME)
    const std::string& mount_path() const { return mount_path_; }

    // The path of the device according to the udev system.
    // (e.g. /sys/devices/pci0000:00/.../8:0:0:0/block/sdb/sdb1)
    const std::string& system_path() const { return system_path_; }

    // The path of the device according to filesystem.
    // (e.g. /dev/sdb)
    const std::string& file_path() const { return file_path_; }

    // Device's label.
    const std::string& device_label() const { return device_label_; }

    void set_device_label(const std::string& device_label) {
      device_label_ = device_label;
    }

    // If disk is a parent, then its label, else parents label.
    // (e.g. "TransMemory")
    const std::string& drive_label() const { return drive_label_; }

    // Vendor ID of the device (e.g. "18d1").
    const std::string& vendor_id() const { return vendor_id_; }

    // Vendor name of the device (e.g. "Google Inc.").
    const std::string& vendor_name() const { return vendor_name_; }

    // Product ID of the device (e.g. "4e11").
    const std::string& product_id() const { return product_id_; }

    // Product name of the device (e.g. "Nexus One").
    const std::string& product_name() const { return product_name_; }

    // Returns the file system uuid string.
    const std::string& fs_uuid() const { return fs_uuid_; }

    // Path of the system device this device's block is a part of.
    // (e.g. /sys/devices/pci0000:00/.../8:0:0:0/)
    const std::string& system_path_prefix() const {
      return system_path_prefix_;
    }

    // Device type.
    DeviceType device_type() const { return device_type_; }

    // Total size of the device in bytes.
    uint64_t total_size_in_bytes() const { return total_size_in_bytes_; }

    // Is the device is a parent device (i.e. sdb rather than sdb1).
    bool is_parent() const { return is_parent_; }

    // Whether the user can write to the device. True if read-only.
    bool is_read_only() const {
      return is_read_only_hardware_ || write_disabled_by_policy_;
    }

    // Is the device read only.
    bool is_read_only_hardware() const { return is_read_only_hardware_; }

    // Does the device contains media.
    bool has_media() const { return has_media_; }

    // Is the device on the boot device.
    bool on_boot_device() const { return on_boot_device_; }

    // Is the device on the removable device.
    bool on_removable_device() const { return on_removable_device_; }

    // Shoud the device be shown in the UI, or automounted.
    bool is_hidden() const { return is_hidden_; }

    void set_write_disabled_by_policy(bool disable) {
      write_disabled_by_policy_ = disable;
    }

    void clear_mount_path() { mount_path_.clear(); }

    bool is_mounted() const { return !mount_path_.empty(); }

    const std::string& file_system_type() const { return file_system_type_; }

    void set_file_system_type(const std::string& file_system_type) {
      file_system_type_ = file_system_type;
    }
    // Name of the first mount path of the disk.
    const std::string& base_mount_path() const { return base_mount_path_; }

    void SetMountPath(const std::string& mount_path);

    bool IsAutoMountable() const;

    bool IsStatefulPartition() const;

   private:
    std::string device_path_;
    std::string mount_path_;
    bool write_disabled_by_policy_;
    std::string system_path_;
    std::string file_path_;
    std::string device_label_;
    std::string drive_label_;
    std::string vendor_id_;
    std::string vendor_name_;
    std::string product_id_;
    std::string product_name_;
    std::string fs_uuid_;
    std::string system_path_prefix_;
    DeviceType device_type_;
    uint64_t total_size_in_bytes_;
    bool is_parent_;
    bool is_read_only_hardware_;
    bool has_media_;
    bool on_boot_device_;
    bool on_removable_device_;
    bool is_hidden_;
    std::string file_system_type_;
    std::string base_mount_path_;
  };
  typedef std::map<std::string, std::unique_ptr<Disk>> DiskMap;

  // A struct to store information about mount point.
  struct MountPointInfo {
    // Device's path.
    std::string source_path;
    // Mounted path.
    std::string mount_path;
    // Type of mount.
    MountType mount_type;
    // Condition of mount.
    MountCondition mount_condition;

    MountPointInfo(const std::string& source,
                   const std::string& mount,
                   const MountType type,
                   MountCondition condition)
        : source_path(source),
          mount_path(mount),
          mount_type(type),
          mount_condition(condition) {
    }
  };

  // MountPointMap key is mount_path.
  typedef std::map<std::string, MountPointInfo> MountPointMap;

  // A callback function type which is called after UnmountDeviceRecursively
  // finishes.
  typedef base::Callback<void(bool)> UnmountDeviceRecursivelyCallbackType;

  // A callback type for UnmountPath method.
  typedef base::Callback<void(MountError error_code)> UnmountPathCallback;

  // A callback type for EnsureMountInfoRefreshed method.
  typedef base::Callback<void(bool success)> EnsureMountInfoRefreshedCallback;

  // Implement this interface to be notified about disk/mount related events.
  class Observer {
   public:
    virtual ~Observer() {}

    // Called when auto-mountable disk mount status is changed.
    virtual void OnAutoMountableDiskEvent(DiskEvent event, const Disk& disk) {}
    // Called when fixed storage disk status is changed.
    virtual void OnBootDeviceDiskEvent(DiskEvent event, const Disk& disk) {}
    // Called when device status is changed.
    virtual void OnDeviceEvent(DeviceEvent event,
                               const std::string& device_path) {}
    // Called after a mount point has been mounted or unmounted.
    virtual void OnMountEvent(MountEvent event,
                              MountError error_code,
                              const MountPointInfo& mount_info) {}
    // Called on format process events.
    virtual void OnFormatEvent(FormatEvent event,
                               FormatError error_code,
                               const std::string& device_path) {}
    // Called on rename process events.
    virtual void OnRenameEvent(RenameEvent event,
                               RenameError error_code,
                               const std::string& device_path) {}
  };

  virtual ~DiskMountManager() {}

  // Adds an observer.
  virtual void AddObserver(Observer* observer) = 0;

  // Removes an observer.
  virtual void RemoveObserver(Observer* observer) = 0;

  // Gets the list of disks found.
  virtual const DiskMap& disks() const = 0;

  // Returns Disk object corresponding to |source_path| or NULL on failure.
  virtual const Disk* FindDiskBySourcePath(
      const std::string& source_path) const = 0;

  // Gets the list of mount points.
  virtual const MountPointMap& mount_points() const = 0;

  // Refreshes all the information about mounting if it is not yet done and
  // invokes |callback| when finished. If the information is already refreshed
  // and |force| is false, it just runs |callback| immediately.
  virtual void EnsureMountInfoRefreshed(
      const EnsureMountInfoRefreshedCallback& callback,
      bool force) = 0;

  // Mounts a device or an archive file.
  // |source_path| specifies either a device or an archive file path.
  // When |type|=MOUNT_TYPE_ARCHIVE, caller may set two optional arguments:
  // |source_format| and |mount_label|. See CrosDisksClient::Mount for detail.
  // |access_mode| specifies read-only or read-write mount mode for a device.
  // Note that the mount operation may fail. To find out the result, one should
  // observe DiskMountManager for |Observer::OnMountEvent| event, which will be
  // raised upon the mount operation completion.
  virtual void MountPath(const std::string& source_path,
                         const std::string& source_format,
                         const std::string& mount_label,
                         const std::vector<std::string>& mount_options,
                         MountType type,
                         MountAccessMode access_mode) = 0;

  // Unmounts a mounted disk.
  // |UnmountOptions| enum defined in chromeos/dbus/cros_disks_client.h.
  // When the method is complete, |callback| will be called and observers'
  // |OnMountEvent| will be raised.
  //
  // |callback| may be empty, in which case it gets ignored.
  virtual void UnmountPath(const std::string& mount_path,
                           UnmountOptions options,
                           const UnmountPathCallback& callback) = 0;

  // Remounts mounted removable devices to change the read-only mount option.
  // Devices that can be mounted only in its read-only mode will be ignored.
  virtual void RemountAllRemovableDrives(chromeos::MountAccessMode mode) = 0;

  // Formats Device given its mount path. Unmounts the device.
  // Example: mount_path: /media/VOLUME_LABEL
  virtual void FormatMountedDevice(const std::string& mount_path) = 0;

  // Renames Device given its mount path.
  // Example: mount_path: /media/VOLUME_LABEL
  //          volume_name: MYUSB
  virtual void RenameMountedDevice(const std::string& mount_path,
                                   const std::string& volume_name) = 0;

  // Unmounts device_path and all of its known children.
  virtual void UnmountDeviceRecursively(
      const std::string& device_path,
      const UnmountDeviceRecursivelyCallbackType& callback) = 0;

  // Used in tests to initialize the manager's disk and mount point sets.
  // Default implementation does noting. It just fails.
  virtual bool AddDiskForTest(std::unique_ptr<Disk> disk);
  virtual bool AddMountPointForTest(const MountPointInfo& mount_point);

  // Returns corresponding string to |type| like "unknown_filesystem".
  static std::string MountConditionToString(MountCondition type);

  // Returns corresponding string to |type|, like "sd", "usb".
  static std::string DeviceTypeToString(DeviceType type);

  // Creates the global DiskMountManager instance.
  static void Initialize();

  // Similar to Initialize(), but can inject an alternative
  // DiskMountManager such as MockDiskMountManager for testing.
  // The injected object will be owned by the internal pointer and deleted
  // by Shutdown().
  static void InitializeForTesting(DiskMountManager* disk_mount_manager);

  // Destroys the global DiskMountManager instance if it exists.
  static void Shutdown();

  // Returns a pointer to the global DiskMountManager instance.
  // Initialize() should already have been called.
  static DiskMountManager* GetInstance();
};

}  // namespace disks
}  // namespace chromeos

#endif  // CHROMEOS_DISKS_DISK_MOUNT_MANAGER_H_
