// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/dbus/fake_cros_disks_client.h"

#include <utility>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_task_runner_handle.h"

namespace chromeos {

namespace {

// Performs fake mounting by creating a directory with a dummy file.
MountError PerformFakeMount(const std::string& source_path,
                            const base::FilePath& mounted_path) {
  // Just create an empty directory and shows it as the mounted directory.
  if (!base::CreateDirectory(mounted_path)) {
    DLOG(ERROR) << "Failed to create directory at " << mounted_path.value();
    return MOUNT_ERROR_DIRECTORY_CREATION_FAILED;
  }

  // Put a dummy file.
  const base::FilePath dummy_file_path =
      mounted_path.Append("SUCCESSFULLY_PERFORMED_FAKE_MOUNT.txt");
  const std::string dummy_file_content = "This is a dummy file.";
  const int write_result = base::WriteFile(
      dummy_file_path, dummy_file_content.data(), dummy_file_content.size());
  if (write_result != static_cast<int>(dummy_file_content.size())) {
    DLOG(ERROR) << "Failed to put a dummy file at "
                << dummy_file_path.value();
    return MOUNT_ERROR_MOUNT_PROGRAM_FAILED;
  }

  return MOUNT_ERROR_NONE;
}

}  // namespace

FakeCrosDisksClient::FakeCrosDisksClient()
    : unmount_call_count_(0),
      unmount_success_(true),
      format_call_count_(0),
      format_success_(true),
      rename_call_count_(0),
      rename_success_(true),
      weak_ptr_factory_(this) {}

FakeCrosDisksClient::~FakeCrosDisksClient() = default;

void FakeCrosDisksClient::Init(dbus::Bus* bus) {
}

void FakeCrosDisksClient::AddObserver(Observer* observer) {
  observer_list_.AddObserver(observer);
}

void FakeCrosDisksClient::RemoveObserver(Observer* observer) {
  observer_list_.RemoveObserver(observer);
}

void FakeCrosDisksClient::Mount(const std::string& source_path,
                                const std::string& source_format,
                                const std::string& mount_label,
                                const std::vector<std::string>& mount_options,
                                MountAccessMode access_mode,
                                RemountOption remount,
                                VoidDBusMethodCallback callback) {
  // This fake implementation assumes mounted path is device when source_format
  // is empty, or an archive otherwise.
  MountType type =
      source_format.empty() ? MOUNT_TYPE_DEVICE : MOUNT_TYPE_ARCHIVE;

  base::FilePath mounted_path;
  switch (type) {
    case MOUNT_TYPE_ARCHIVE:
      mounted_path = GetArchiveMountPoint().Append(
          base::FilePath::FromUTF8Unsafe(mount_label));
      break;
    case MOUNT_TYPE_DEVICE:
      mounted_path = GetRemovableDiskMountPoint().Append(
          base::FilePath::FromUTF8Unsafe(mount_label));
      break;
    case MOUNT_TYPE_NETWORK_STORAGE:
      // TODO(sammc): Support mounting fake network storage.
      NOTREACHED();
      return;
    case MOUNT_TYPE_INVALID:
      NOTREACHED();
      return;
  }
  mounted_paths_.insert(mounted_path);

  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
      base::BindOnce(&PerformFakeMount, source_path, mounted_path),
      base::BindOnce(&FakeCrosDisksClient::DidMount,
                     weak_ptr_factory_.GetWeakPtr(), source_path, type,
                     mounted_path, std::move(callback)));
}

void FakeCrosDisksClient::DidMount(const std::string& source_path,
                                   MountType type,
                                   const base::FilePath& mounted_path,
                                   VoidDBusMethodCallback callback,
                                   MountError mount_error) {
  // Tell the caller of Mount() that the mount request was accepted.
  // Note that even if PerformFakeMount fails, this calls with |true| to
  // emulate the situation that 1) Mount operation is _successfully_ started,
  // 2) then failed for some reason.
  std::move(callback).Run(true);

  // Notify observers that the mount is completed.
  NotifyMountCompleted(mount_error, source_path, type,
                       mounted_path.AsUTF8Unsafe());
}

void FakeCrosDisksClient::Unmount(const std::string& device_path,
                                  UnmountOptions options,
                                  VoidDBusMethodCallback callback) {
  DCHECK(!callback.is_null());

  unmount_call_count_++;
  last_unmount_device_path_ = device_path;
  last_unmount_options_ = options;

  // Remove the dummy mounted directory if it exists.
  if (mounted_paths_.erase(base::FilePath::FromUTF8Unsafe(device_path))) {
    base::PostTaskWithTraitsAndReply(
        FROM_HERE,
        {base::MayBlock(), base::TaskPriority::BACKGROUND,
         base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
        base::BindOnce(base::IgnoreResult(&base::DeleteFile),
                       base::FilePath::FromUTF8Unsafe(device_path),
                       true /* recursive */),
        base::BindOnce(std::move(callback), unmount_success_));
  } else {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), unmount_success_));
  }
  if (!unmount_listener_.is_null())
    unmount_listener_.Run();
}

void FakeCrosDisksClient::EnumerateAutoMountableDevices(
    const EnumerateDevicesCallback& callback,
    const base::Closure& error_callback) {}

void FakeCrosDisksClient::EnumerateDevices(
    const EnumerateDevicesCallback& callback,
    const base::Closure& error_callback) {}

void FakeCrosDisksClient::EnumerateMountEntries(
    const EnumerateMountEntriesCallback& callback,
    const base::Closure& error_callback) {
}

void FakeCrosDisksClient::Format(const std::string& device_path,
                                 const std::string& filesystem,
                                 VoidDBusMethodCallback callback) {
  DCHECK(!callback.is_null());

  format_call_count_++;
  last_format_device_path_ = device_path;
  last_format_filesystem_ = filesystem;
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), format_success_));
}

void FakeCrosDisksClient::Rename(const std::string& device_path,
                                 const std::string& volume_name,
                                 VoidDBusMethodCallback callback) {
  DCHECK(!callback.is_null());

  rename_call_count_++;
  last_rename_device_path_ = device_path;
  last_rename_volume_name_ = volume_name;
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), rename_success_));
}

void FakeCrosDisksClient::GetDeviceProperties(
    const std::string& device_path,
    const GetDevicePropertiesCallback& callback,
    const base::Closure& error_callback) {
}

void FakeCrosDisksClient::NotifyMountCompleted(MountError error_code,
                                               const std::string& source_path,
                                               MountType mount_type,
                                               const std::string& mount_path) {
  for (auto& observer : observer_list_) {
    observer.OnMountCompleted(
        MountEntry(error_code, source_path, mount_type, mount_path));
  }
}

void FakeCrosDisksClient::NotifyFormatCompleted(
    FormatError error_code,
    const std::string& device_path) {
  for (auto& observer : observer_list_)
    observer.OnFormatCompleted(error_code, device_path);
}

void FakeCrosDisksClient::NotifyRenameCompleted(
    RenameError error_code,
    const std::string& device_path) {
  for (auto& observer : observer_list_)
    observer.OnRenameCompleted(error_code, device_path);
}

}  // namespace chromeos
