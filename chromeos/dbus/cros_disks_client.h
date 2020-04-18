// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_CROS_DISKS_CLIENT_H_
#define CHROMEOS_DBUS_CROS_DISKS_CLIENT_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/dbus/dbus_client.h"
#include "chromeos/dbus/dbus_client_implementation_type.h"
#include "chromeos/dbus/dbus_method_call_status.h"

namespace base {
class FilePath;
}

namespace dbus {
class Response;
}

// TODO(tbarzic): We should move these enums inside CrosDisksClient,
// to be clearer where they come from. Also, most of these are partially or
// completely duplicated in third_party/dbus/service_constants.h. We should
// probably use enums from service_contstants directly.
namespace chromeos {

// Enum describing types of mount used by cros-disks.
enum MountType {
  MOUNT_TYPE_INVALID,
  MOUNT_TYPE_DEVICE,
  MOUNT_TYPE_ARCHIVE,
  MOUNT_TYPE_NETWORK_STORAGE,
};

// Type of device.
enum DeviceType {
  DEVICE_TYPE_UNKNOWN,
  DEVICE_TYPE_USB,  // USB stick.
  DEVICE_TYPE_SD,  // SD card.
  DEVICE_TYPE_OPTICAL_DISC,  // e.g. Optical disc excluding DVD.
  DEVICE_TYPE_MOBILE,  // Storage on a mobile device (e.g. Android).
  DEVICE_TYPE_DVD,  // DVD.
};

// Mount error code used by cros-disks.
enum MountError {
  MOUNT_ERROR_NONE = 0,
  MOUNT_ERROR_UNKNOWN = 1,
  MOUNT_ERROR_INTERNAL = 2,
  MOUNT_ERROR_INVALID_ARGUMENT = 3,
  MOUNT_ERROR_INVALID_PATH = 4,
  MOUNT_ERROR_PATH_ALREADY_MOUNTED = 5,
  MOUNT_ERROR_PATH_NOT_MOUNTED = 6,
  MOUNT_ERROR_DIRECTORY_CREATION_FAILED = 7,
  MOUNT_ERROR_INVALID_MOUNT_OPTIONS = 8,
  MOUNT_ERROR_INVALID_UNMOUNT_OPTIONS = 9,
  MOUNT_ERROR_INSUFFICIENT_PERMISSIONS = 10,
  MOUNT_ERROR_MOUNT_PROGRAM_NOT_FOUND = 11,
  MOUNT_ERROR_MOUNT_PROGRAM_FAILED = 12,
  MOUNT_ERROR_INVALID_DEVICE_PATH = 100,
  MOUNT_ERROR_UNKNOWN_FILESYSTEM = 101,
  MOUNT_ERROR_UNSUPPORTED_FILESYSTEM = 102,
  MOUNT_ERROR_INVALID_ARCHIVE = 201,
  MOUNT_ERROR_NOT_AUTHENTICATED = 601,
  MOUNT_ERROR_PATH_UNMOUNTED = 901,
  // TODO(tbarzic): Add more error codes as they get added to cros-disks and
  // consider doing explicit translation from cros-disks error_types.
};

// Rename error reported by cros-disks.
enum RenameError {
  RENAME_ERROR_NONE,
  RENAME_ERROR_UNKNOWN,
  RENAME_ERROR_INTERNAL,
  RENAME_ERROR_INVALID_DEVICE_PATH,
  RENAME_ERROR_DEVICE_BEING_RENAMED,
  RENAME_ERROR_UNSUPPORTED_FILESYSTEM,
  RENAME_ERROR_RENAME_PROGRAM_NOT_FOUND,
  RENAME_ERROR_RENAME_PROGRAM_FAILED,
  RENAME_ERROR_DEVICE_NOT_ALLOWED,
  RENAME_ERROR_LONG_NAME,
  RENAME_ERROR_INVALID_CHARACTER,
};

// Format error reported by cros-disks.
enum FormatError {
  FORMAT_ERROR_NONE,
  FORMAT_ERROR_UNKNOWN,
  FORMAT_ERROR_INTERNAL,
  FORMAT_ERROR_INVALID_DEVICE_PATH,
  FORMAT_ERROR_DEVICE_BEING_FORMATTED,
  FORMAT_ERROR_UNSUPPORTED_FILESYSTEM,
  FORMAT_ERROR_FORMAT_PROGRAM_NOT_FOUND,
  FORMAT_ERROR_FORMAT_PROGRAM_FAILED,
  FORMAT_ERROR_DEVICE_NOT_ALLOWED,
};

// Event type each corresponding to a signal sent from cros-disks.
enum MountEventType {
  CROS_DISKS_DISK_ADDED,
  CROS_DISKS_DISK_REMOVED,
  CROS_DISKS_DISK_CHANGED,
  CROS_DISKS_DEVICE_ADDED,
  CROS_DISKS_DEVICE_REMOVED,
  CROS_DISKS_DEVICE_SCANNED,
};

// Additional unmount flags to be added to unmount request.
enum UnmountOptions {
  UNMOUNT_OPTIONS_NONE,
  UNMOUNT_OPTIONS_LAZY,  // Do lazy unmount.
};

// Mount option to control write permission to a device.
enum MountAccessMode {
  MOUNT_ACCESS_MODE_READ_WRITE,
  MOUNT_ACCESS_MODE_READ_ONLY,
};

// Whether to mount to a new path or remount a device already mounted.
enum RemountOption {
  // Mount a new device. If the device is already mounted, the mount status is
  // unchanged and the callback for MountCompleted will receive
  // MOUNT_ERROR_PATH_ALREADY_MOUNTED error code.
  REMOUNT_OPTION_MOUNT_NEW_DEVICE,
  // Remount a device that is already mounted. If the device is not mounted
  // yet, it will do nothing and the callback for MountCompleted will receive
  // MOUNT_ERROR_PATH_NOT_MOUNTED error code.
  REMOUNT_OPTION_REMOUNT_EXISTING_DEVICE,
};

// A class to represent information about a disk sent from cros-disks.
class CHROMEOS_EXPORT DiskInfo {
 public:
  DiskInfo(const std::string& device_path, dbus::Response* response);
  ~DiskInfo();

  // Device path. (e.g. /sys/devices/pci0000:00/.../8:0:0:0/block/sdb/sdb1)
  const std::string& device_path() const { return device_path_; }

  // Disk mount path. (e.g. /media/removable/VOLUME)
  const std::string& mount_path() const { return mount_path_; }

  // Disk system path given by udev.
  // (e.g. /sys/devices/pci0000:00/.../8:0:0:0/block/sdb/sdb1)
  const std::string& system_path() const { return system_path_; }

  // Is a drive or not. (i.e. true with /dev/sdb, false with /dev/sdb1)
  bool is_drive() const { return is_drive_; }

  // Does the disk have media content.
  bool has_media() const { return has_media_; }

  // Is the disk on device we booted the machine from.
  bool on_boot_device() const { return on_boot_device_; }

  // Is the disk on a removable device.
  bool on_removable_device() const { return on_removable_device_; }

  // Is the device read-only.
  bool is_read_only() const { return is_read_only_; }

  // Returns true if the device should be hidden from the file browser.
  bool is_hidden() const { return is_hidden_; }

  // Is the disk virtual.
  bool is_virtual() const { return is_virtual_; }

  // Disk file path (e.g. /dev/sdb).
  const std::string& file_path() const { return file_path_; }

  // Disk label.
  const std::string& label() const { return label_; }

  // Vendor ID of the device (e.g. "18d1").
  const std::string& vendor_id() const { return vendor_id_; }

  // Vendor name of the device (e.g. "Google Inc.").
  const std::string& vendor_name() const { return vendor_name_; }

  // Product ID of the device (e.g. "4e11").
  const std::string& product_id() const { return product_id_; }

  // Product name of the device (e.g. "Nexus One").
  const std::string& product_name() const { return product_name_; }

  // Disk model. (e.g. "TransMemory")
  const std::string& drive_label() const { return drive_model_; }

  // Device type. Not working well, yet.
  DeviceType device_type() const { return device_type_; }

  // Total size of the disk in bytes.
  uint64_t total_size_in_bytes() const { return total_size_in_bytes_; }

  // Returns file system uuid.
  const std::string& uuid() const { return uuid_; }

  // Returns file system type identifier.
  const std::string& file_system_type() const { return file_system_type_; }

 private:
  void InitializeFromResponse(dbus::Response* response);

  std::string device_path_;
  std::string mount_path_;
  std::string system_path_;
  bool is_drive_;
  bool has_media_;
  bool on_boot_device_;
  bool on_removable_device_;
  bool is_read_only_;
  bool is_hidden_;
  bool is_virtual_;

  std::string file_path_;
  std::string label_;
  std::string vendor_id_;
  std::string vendor_name_;
  std::string product_id_;
  std::string product_name_;
  std::string drive_model_;
  DeviceType device_type_;
  uint64_t total_size_in_bytes_;
  std::string uuid_;
  std::string file_system_type_;
};

// A struct to represent information about a mount point sent from cros-disks.
struct CHROMEOS_EXPORT MountEntry {
 public:
  MountEntry()
      : error_code_(MOUNT_ERROR_UNKNOWN), mount_type_(MOUNT_TYPE_INVALID) {
  }

  MountEntry(MountError error_code,
             const std::string& source_path,
             MountType mount_type,
             const std::string& mount_path)
      : error_code_(error_code),
        source_path_(source_path),
        mount_type_(mount_type),
        mount_path_(mount_path) {
  }

  MountError error_code() const { return error_code_; }
  const std::string& source_path() const { return source_path_; }
  MountType mount_type() const { return mount_type_; }
  const std::string& mount_path() const { return mount_path_; }

 private:
  MountError error_code_;
  std::string source_path_;
  MountType mount_type_;
  std::string mount_path_;
};

// A class to make the actual DBus calls for cros-disks service.
// This class only makes calls, result/error handling should be done
// by callbacks.
class CHROMEOS_EXPORT CrosDisksClient : public DBusClient {
 public:
  // A callback to handle the result of EnumerateAutoMountableDevices and
  // EnumerateDevices. The argument is the enumerated device paths.
  typedef base::Callback<void(const std::vector<std::string>& device_paths)>
      EnumerateDevicesCallback;

  // A callback to handle the result of EnumerateMountEntries.
  // The argument is the enumerated mount entries.
  typedef base::Callback<void(const std::vector<MountEntry>& entries)>
      EnumerateMountEntriesCallback;

  // A callback to handle the result of GetDeviceProperties.
  // The argument is the information about the specified device.
  typedef base::Callback<void(const DiskInfo& disk_info)>
      GetDevicePropertiesCallback;

  class Observer {
   public:
    // Called when a mount event signal is received.
    virtual void OnMountEvent(MountEventType event_type,
                              const std::string& device_path) = 0;

    // Called when a MountCompleted signal is received.
    virtual void OnMountCompleted(const MountEntry& entry) = 0;

    // Called when a FormatCompleted signal is received.
    virtual void OnFormatCompleted(FormatError error_code,
                                   const std::string& device_path) = 0;

    // Called when a RenameCompleted signal is received.
    virtual void OnRenameCompleted(RenameError error_code,
                                   const std::string& device_path) = 0;

   protected:
    virtual ~Observer() = default;
  };

  ~CrosDisksClient() override;

  // Registers the given |observer| to listen D-Bus signals.
  virtual void AddObserver(Observer* observer) = 0;

  // Unregisters the |observer| from this instance.
  virtual void RemoveObserver(Observer* observer) = 0;

  // Calls Mount method.  On method call completion, |callback| is called with
  // |true| on success, or with |false| otherwise.
  // When mounting an archive, caller may set two optional arguments:
  // - The |source_format| argument passes the file extension (with the leading
  //   dot, for example ".zip"). If |source_format| is empty then the source
  //   format is auto-detected.
  // - The |mount_label| argument passes an optional mount label to be used as
  //   the directory name of the mount point. If |mount_label| is empty, the
  //   mount label will be based on the |source_path|.
  virtual void Mount(const std::string& source_path,
                     const std::string& source_format,
                     const std::string& mount_label,
                     const std::vector<std::string>& mount_options,
                     MountAccessMode access_mode,
                     RemountOption remount,
                     VoidDBusMethodCallback callback) = 0;

  // Calls Unmount method.  On method call completion, |callback| is called
  // with |true| on success, or with |false| otherwise.
  virtual void Unmount(const std::string& device_path,
                       UnmountOptions options,
                       VoidDBusMethodCallback callback) = 0;

  // Calls EnumerateAutoMountableDevices method.  |callback| is called after the
  // method call succeeds, otherwise, |error_callback| is called.
  virtual void EnumerateAutoMountableDevices(
      const EnumerateDevicesCallback& callback,
      const base::Closure& error_callback) = 0;

  // Calls EnumerateDevices method.  |callback| is called after the
  // method call succeeds, otherwise, |error_callback| is called.
  virtual void EnumerateDevices(const EnumerateDevicesCallback& callback,
                                const base::Closure& error_callback) = 0;

  // Calls EnumerateMountEntries.  |callback| is called after the
  // method call succeeds, otherwise, |error_callback| is called.
  virtual void EnumerateMountEntries(
      const EnumerateMountEntriesCallback& callback,
      const base::Closure& error_callback) = 0;

  // Calls Format method. On completion, |callback| is called, with |true| on
  // success, or with |false| otherwise.
  virtual void Format(const std::string& device_path,
                      const std::string& filesystem,
                      VoidDBusMethodCallback callback) = 0;

  // Calls Rename method. On completion, |callback| is called, with |true| on
  // success, or with |false| otherwise.
  virtual void Rename(const std::string& device_path,
                      const std::string& volume_name,
                      VoidDBusMethodCallback callback) = 0;

  // Calls GetDeviceProperties method.  |callback| is called after the method
  // call succeeds, otherwise, |error_callback| is called.
  virtual void GetDeviceProperties(const std::string& device_path,
                                   const GetDevicePropertiesCallback& callback,
                                   const base::Closure& error_callback) = 0;

  // Factory function, creates a new instance and returns ownership.
  // For normal usage, access the singleton via DBusThreadManager::Get().
  static CrosDisksClient* Create(DBusClientImplementationType type);

  // Returns the path of the mount point for archive files.
  static base::FilePath GetArchiveMountPoint();

  // Returns the path of the mount point for removable disks.
  static base::FilePath GetRemovableDiskMountPoint();

  // Composes a list of mount options.
  static std::vector<std::string> ComposeMountOptions(
      const std::vector<std::string>& options,
      const std::string& mount_label,
      MountAccessMode access_mode,
      RemountOption remount);

 protected:
  // Create() should be used instead.
  CrosDisksClient();

 private:
  DISALLOW_COPY_AND_ASSIGN(CrosDisksClient);
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_CROS_DISKS_CLIENT_H_
