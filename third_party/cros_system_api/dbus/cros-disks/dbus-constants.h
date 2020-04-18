// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_CROS_DISKS_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_CROS_DISKS_DBUS_CONSTANTS_H_

namespace cros_disks {
const char kCrosDisksInterface[] = "org.chromium.CrosDisks";
const char kCrosDisksServicePath[] = "/org/chromium/CrosDisks";
const char kCrosDisksServiceName[] = "org.chromium.CrosDisks";
const char kCrosDisksServiceError[] = "org.chromium.CrosDisks.Error";

// Methods.
const char kEnumerateAutoMountableDevices[] = "EnumerateAutoMountableDevices";
const char kEnumerateDevices[] = "EnumerateDevices";
const char kEnumerateMountEntries[] = "EnumerateMountEntries";
const char kFormat[] = "Format";
const char kGetDeviceProperties[] = "GetDeviceProperties";
const char kMount[] = "Mount";
const char kRename[] = "Rename";
const char kUnmount[] = "Unmount";

// Signals.
const char kDeviceAdded[] = "DeviceAdded";
const char kDeviceScanned[] = "DeviceScanned";
const char kDeviceRemoved[] = "DeviceRemoved";
const char kDiskAdded[] = "DiskAdded";
const char kDiskChanged[] = "DiskChanged";
const char kDiskRemoved[] = "DiskRemoved";
const char kFormatCompleted[] = "FormatCompleted";
const char kMountCompleted[] = "MountCompleted";
const char kRenameCompleted[] = "RenameCompleted";

// Properties.
// TODO(benchan): Rename 'DeviceIs*' property to 'DiskIs*' as the latter is more
// accurate.
const char kDeviceFile[] = "DeviceFile";
const char kDeviceIsDrive[] = "DeviceIsDrive";
const char kDeviceIsMediaAvailable[] = "DeviceIsMediaAvailable";
const char kDeviceIsMounted[] = "DeviceIsMounted";
const char kDeviceIsOnBootDevice[] = "DeviceIsOnBootDevice";
const char kDeviceIsOnRemovableDevice[] = "DeviceIsOnRemovableDevice";
const char kDeviceIsReadOnly[] = "DeviceIsReadOnly";
const char kDeviceIsVirtual[] = "DeviceIsVirtual";
const char kDeviceMediaType[] = "DeviceMediaType";
const char kDeviceMountPaths[] = "DeviceMountPaths";
const char kDevicePresentationHide[] = "DevicePresentationHide";
const char kDeviceSize[] = "DeviceSize";
const char kDriveIsRotational[] = "DriveIsRotational";
const char kDriveModel[] = "DriveModel";
const char kIdLabel[] = "IdLabel";
const char kIdUuid[] = "IdUuid";
const char kVendorId[] = "VendorId";
const char kVendorName[] = "VendorName";
const char kProductId[] = "ProductId";
const char kProductName[] = "ProductName";
const char kNativePath[] = "NativePath";
const char kFileSystemType[] = "FileSystemType";

// Enum values.
// DeviceMediaType enum values are reported through UMA.
// All values but DEVICE_MEDIA_NUM_VALUES should not be changed or removed.
// Additional values can be added but DEVICE_MEDIA_NUM_VALUES should always
// be the last value in the enum.
enum DeviceMediaType {
  DEVICE_MEDIA_UNKNOWN = 0,
  DEVICE_MEDIA_USB = 1,
  DEVICE_MEDIA_SD = 2,
  DEVICE_MEDIA_OPTICAL_DISC = 3,
  DEVICE_MEDIA_MOBILE = 4,
  DEVICE_MEDIA_DVD = 5,
  DEVICE_MEDIA_NUM_VALUES,
};

enum FormatErrorType {
  FORMAT_ERROR_NONE = 0,
  FORMAT_ERROR_UNKNOWN = 1,
  FORMAT_ERROR_INTERNAL = 2,
  FORMAT_ERROR_INVALID_DEVICE_PATH = 3,
  FORMAT_ERROR_DEVICE_BEING_FORMATTED = 4,
  FORMAT_ERROR_UNSUPPORTED_FILESYSTEM = 5,
  FORMAT_ERROR_FORMAT_PROGRAM_NOT_FOUND = 6,
  FORMAT_ERROR_FORMAT_PROGRAM_FAILED = 7,
  FORMAT_ERROR_DEVICE_NOT_ALLOWED = 8,
};

// TODO(benchan): After both Chrome and cros-disks use these enum values,
// make these error values contiguous so that they can be directly reported
// via UMA.
enum MountErrorType {
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
  MOUNT_ERROR_UNSUPPORTED_ARCHIVE = 202,
};

// MountSourceType enum values are solely used by Chrome/CrosDisks in
// the MountCompleted signal, and currently not reported through UMA.
enum MountSourceType {
  MOUNT_SOURCE_INVALID = 0,
  MOUNT_SOURCE_REMOVABLE_DEVICE = 1,
  MOUNT_SOURCE_ARCHIVE = 2,
  MOUNT_SOURCE_NETWORK_STORAGE = 3,
};

enum RenameErrorType {
  RENAME_ERROR_NONE = 0,
  RENAME_ERROR_UNKNOWN = 1,
  RENAME_ERROR_INTERNAL = 2,
  RENAME_ERROR_INVALID_DEVICE_PATH = 3,
  RENAME_ERROR_DEVICE_BEING_RENAMED = 4,
  RENAME_ERROR_UNSUPPORTED_FILESYSTEM = 5,
  RENAME_ERROR_RENAME_PROGRAM_NOT_FOUND = 6,
  RENAME_ERROR_RENAME_PROGRAM_FAILED = 7,
  RENAME_ERROR_DEVICE_NOT_ALLOWED = 8,
  RENAME_ERROR_LONG_NAME = 9,
  RENAME_ERROR_INVALID_CHARACTER = 10,
};
}  // namespace cros_disks

#endif  // SYSTEM_API_DBUS_CROS_DISKS_DBUS_CONSTANTS_H_
