// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_USB_USB_DEVICE_HANDLE_USBFS_H_
#define DEVICE_USB_USB_DEVICE_HANDLE_USBFS_H_

#include <list>
#include <map>
#include <memory>
#include <vector>

#include "base/files/scoped_file.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "device/usb/usb_device_handle.h"

struct usbdevfs_urb;

namespace base {
class SequencedTaskRunner;
class SingleThreadTaskRunner;
}

namespace device {

// Implementation of a USB device handle on top of the Linux USBFS ioctl
// interface available on Linux, Chrome OS and Android.
class UsbDeviceHandleUsbfs : public UsbDeviceHandle {
 public:
  // Constructs a new device handle from an existing |device| and open file
  // descriptor to that device. |blocking_task_runner| must run tasks on a
  // thread that supports FileDescriptorWatcher.
  UsbDeviceHandleUsbfs(
      scoped_refptr<UsbDevice> device,
      base::ScopedFD fd,
      scoped_refptr<base::SequencedTaskRunner> blocking_task_runner);

  // UsbDeviceHandle implementation.
  scoped_refptr<UsbDevice> GetDevice() const override;
  void Close() override;
  void SetConfiguration(int configuration_value,
                        ResultCallback callback) override;
  void ClaimInterface(int interface_number, ResultCallback callback) override;
  void ReleaseInterface(int interface_number, ResultCallback callback) override;
  void SetInterfaceAlternateSetting(int interface_number,
                                    int alternate_setting,
                                    ResultCallback callback) override;
  void ResetDevice(ResultCallback callback) override;
  void ClearHalt(uint8_t endpoint, ResultCallback callback) override;
  void ControlTransfer(UsbTransferDirection direction,
                       UsbControlTransferType request_type,
                       UsbControlTransferRecipient recipient,
                       uint8_t request,
                       uint16_t value,
                       uint16_t index,
                       scoped_refptr<base::RefCountedBytes> buffer,
                       unsigned int timeout,
                       TransferCallback callback) override;
  void IsochronousTransferIn(uint8_t endpoint_number,
                             const std::vector<uint32_t>& packet_lengths,
                             unsigned int timeout,
                             IsochronousTransferCallback callback) override;
  void IsochronousTransferOut(uint8_t endpoint_number,
                              scoped_refptr<base::RefCountedBytes> buffer,
                              const std::vector<uint32_t>& packet_lengths,
                              unsigned int timeout,
                              IsochronousTransferCallback callback) override;
  // To support DevTools this function may be called from any thread and on
  // completion |callback| will be run on that thread.
  void GenericTransfer(UsbTransferDirection direction,
                       uint8_t endpoint_number,
                       scoped_refptr<base::RefCountedBytes> buffer,
                       unsigned int timeout,
                       TransferCallback callback) override;
  const UsbInterfaceDescriptor* FindInterfaceByEndpoint(
      uint8_t endpoint_address) override;

 protected:
  ~UsbDeviceHandleUsbfs() override;

  scoped_refptr<base::SingleThreadTaskRunner> task_runner() const {
    return task_runner_;
  }

  // Destroys |helper_| and releases ownership of |fd_| without closing it.
  void ReleaseFileDescriptor();

  // Destroys |helper_| and closes |fd_|. Override to call
  // ReleaseFileDescriptor() if necessary.
  virtual void CloseBlocking();

 private:
  class FileThreadHelper;
  struct Transfer;
  struct InterfaceInfo {
    uint8_t alternate_setting;
  };
  struct EndpointInfo {
    UsbTransferType type;
    const UsbInterfaceDescriptor* interface;
  };

  void SetConfigurationComplete(int configuration_value,
                                bool success,
                                ResultCallback callback);
  void ReleaseInterfaceComplete(int interface_number, ResultCallback callback);
  void IsochronousTransferInternal(uint8_t endpoint_address,
                                   scoped_refptr<base::RefCountedBytes> buffer,
                                   size_t total_length,
                                   const std::vector<uint32_t>& packet_lengths,
                                   unsigned int timeout,
                                   IsochronousTransferCallback callback);
  void GenericTransferInternal(
      UsbTransferDirection direction,
      uint8_t endpoint_number,
      scoped_refptr<base::RefCountedBytes> buffer,
      unsigned int timeout,
      TransferCallback callback,
      scoped_refptr<base::SingleThreadTaskRunner> callback_runner);
  void ReapedUrbs(const std::vector<usbdevfs_urb*>& urbs);
  void TransferComplete(std::unique_ptr<Transfer> transfer);
  void RefreshEndpointInfo();
  void ReportIsochronousError(
      const std::vector<uint32_t>& packet_lengths,
      UsbDeviceHandle::IsochronousTransferCallback callback,
      UsbTransferStatus status);
  void SetUpTimeoutCallback(Transfer* transfer, unsigned int timeout);
  void OnTimeout(Transfer* transfer);
  std::unique_ptr<Transfer> RemoveFromTransferList(Transfer* transfer);
  void CancelTransfer(Transfer* transfer, UsbTransferStatus status);
  void DiscardUrbBlocking(Transfer* transfer);
  void UrbDiscarded(Transfer* transfer);

  scoped_refptr<UsbDevice> device_;
  int fd_;  // Copy of the base::ScopedFD held by |helper_| valid if |device_|.
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  scoped_refptr<base::SequencedTaskRunner> blocking_task_runner_;

  // Maps claimed interfaces by interface number to their current alternate
  // setting.
  std::map<uint8_t, InterfaceInfo> interfaces_;

  // Maps endpoints (by endpoint address) to the interface they are a part of.
  // Only endpoints of currently claimed and selected interface alternates are
  // included in the map.
  std::map<uint8_t, EndpointInfo> endpoints_;

  // Helper object exists on the blocking task thread and all calls to it and
  // its destruction must be posted there.
  std::unique_ptr<FileThreadHelper> helper_;

  std::list<std::unique_ptr<Transfer>> transfers_;
  base::SequenceChecker sequence_checker_;
};

}  // namespace device

#endif  // DEVICE_USB_USB_DEVICE_HANDLE_USBFS_H_
