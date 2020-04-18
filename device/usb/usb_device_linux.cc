// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/usb/usb_device_linux.h"

#include <fcntl.h>
#include <stddef.h>

#include <algorithm>

#include "base/bind.h"
#include "base/location.h"
#include "base/posix/eintr_wrapper.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/device_event_log/device_event_log.h"
#include "device/usb/usb_descriptors.h"
#include "device/usb/usb_device_handle_usbfs.h"
#include "device/usb/usb_service.h"

#if defined(OS_CHROMEOS)
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/permission_broker_client.h"
#endif                             // defined(OS_CHROMEOS)

namespace device {

UsbDeviceLinux::UsbDeviceLinux(const std::string& device_path,
                               const UsbDeviceDescriptor& descriptor,
                               const std::string& manufacturer_string,
                               const std::string& product_string,
                               const std::string& serial_number,
                               uint8_t active_configuration)
    : UsbDevice(descriptor,
                base::UTF8ToUTF16(manufacturer_string),
                base::UTF8ToUTF16(product_string),
                base::UTF8ToUTF16(serial_number)),
      device_path_(device_path) {
  ActiveConfigurationChanged(active_configuration);
}

UsbDeviceLinux::~UsbDeviceLinux() = default;

#if defined(OS_CHROMEOS)

void UsbDeviceLinux::CheckUsbAccess(ResultCallback callback) {
  DCHECK(sequence_checker_.CalledOnValidSequence());
  chromeos::PermissionBrokerClient* client =
      chromeos::DBusThreadManager::Get()->GetPermissionBrokerClient();
  DCHECK(client) << "Could not get permission broker client.";
  client->CheckPathAccess(device_path_,
                          base::AdaptCallbackForRepeating(std::move(callback)));
}

#endif  // defined(OS_CHROMEOS)

void UsbDeviceLinux::Open(OpenCallback callback) {
  DCHECK(sequence_checker_.CalledOnValidSequence());

#if defined(OS_CHROMEOS)
  chromeos::PermissionBrokerClient* client =
      chromeos::DBusThreadManager::Get()->GetPermissionBrokerClient();
  auto copyable_callback = base::AdaptCallbackForRepeating(std::move(callback));
  DCHECK(client) << "Could not get permission broker client.";
  client->OpenPath(
      device_path_,
      base::Bind(&UsbDeviceLinux::OnOpenRequestComplete, this,
                 copyable_callback),
      base::Bind(&UsbDeviceLinux::OnOpenRequestError, this, copyable_callback));
#else
  scoped_refptr<base::SequencedTaskRunner> blocking_task_runner =
      UsbService::CreateBlockingTaskRunner();
  blocking_task_runner->PostTask(
      FROM_HERE,
      base::BindOnce(&UsbDeviceLinux::OpenOnBlockingThread, this,
                     std::move(callback), base::ThreadTaskRunnerHandle::Get(),
                     blocking_task_runner));
#endif  // defined(OS_CHROMEOS)
}

#if defined(OS_CHROMEOS)

void UsbDeviceLinux::OnOpenRequestComplete(OpenCallback callback,
                                           base::ScopedFD fd) {
  if (!fd.is_valid()) {
    USB_LOG(EVENT) << "Did not get valid device handle from permission broker.";
    std::move(callback).Run(nullptr);
    return;
  }
  Opened(std::move(fd), std::move(callback),
         UsbService::CreateBlockingTaskRunner());
}

void UsbDeviceLinux::OnOpenRequestError(OpenCallback callback,
                                        const std::string& error_name,
                                        const std::string& error_message) {
  USB_LOG(EVENT) << "Permission broker failed to open the device: "
                 << error_name << ": " << error_message;
  std::move(callback).Run(nullptr);
}

#else

void UsbDeviceLinux::OpenOnBlockingThread(
    OpenCallback callback,
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    scoped_refptr<base::SequencedTaskRunner> blocking_task_runner) {
  base::ScopedFD fd(HANDLE_EINTR(open(device_path_.c_str(), O_RDWR)));
  if (fd.is_valid()) {
    task_runner->PostTask(
        FROM_HERE, base::BindOnce(&UsbDeviceLinux::Opened, this, std::move(fd),
                                  std::move(callback), blocking_task_runner));
  } else {
    USB_PLOG(EVENT) << "Failed to open " << device_path_;
    task_runner->PostTask(FROM_HERE,
                          base::BindOnce(std::move(callback), nullptr));
  }
}

#endif  // defined(OS_CHROMEOS)

void UsbDeviceLinux::Opened(
    base::ScopedFD fd,
    OpenCallback callback,
    scoped_refptr<base::SequencedTaskRunner> blocking_task_runner) {
  DCHECK(sequence_checker_.CalledOnValidSequence());
  scoped_refptr<UsbDeviceHandle> device_handle =
      new UsbDeviceHandleUsbfs(this, std::move(fd), blocking_task_runner);
  handles().push_back(device_handle.get());
  std::move(callback).Run(device_handle);
}

}  // namespace device
