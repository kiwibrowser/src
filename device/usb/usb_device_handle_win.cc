// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/usb/usb_device_handle_win.h"

#include <windows.h>  // Must be in front of other Windows header files.

#include <usbioctl.h>
#include <usbspec.h>
#include <winioctl.h>
#include <winusb.h>

#include <algorithm>
#include <memory>
#include <numeric>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted_memory.h"
#include "base/single_thread_task_runner.h"
#include "base/stl_util.h"
#include "base/strings/string16.h"
#include "base/synchronization/lock.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/win/object_watcher.h"
#include "components/device_event_log/device_event_log.h"
#include "device/usb/usb_context.h"
#include "device/usb/usb_descriptors.h"
#include "device/usb/usb_device_win.h"
#include "device/usb/usb_service.h"

namespace device {

namespace {

uint8_t BuildRequestFlags(UsbTransferDirection direction,
                          UsbControlTransferType request_type,
                          UsbControlTransferRecipient recipient) {
  uint8_t flags = 0;

  switch (direction) {
    case UsbTransferDirection::OUTBOUND:
      flags |= BMREQUEST_HOST_TO_DEVICE << 7;
      break;
    case UsbTransferDirection::INBOUND:
      flags |= BMREQUEST_DEVICE_TO_HOST << 7;
      break;
  }

  switch (request_type) {
    case UsbControlTransferType::STANDARD:
      flags |= BMREQUEST_STANDARD << 5;
      break;
    case UsbControlTransferType::CLASS:
      flags |= BMREQUEST_CLASS << 5;
      break;
    case UsbControlTransferType::VENDOR:
      flags |= BMREQUEST_VENDOR << 5;
      break;
    case UsbControlTransferType::RESERVED:
      flags |= 4 << 5;  // Not defined by usbspec.h.
      break;
  }

  switch (recipient) {
    case UsbControlTransferRecipient::DEVICE:
      flags |= BMREQUEST_TO_DEVICE;
      break;
    case UsbControlTransferRecipient::INTERFACE:
      flags |= BMREQUEST_TO_INTERFACE;
      break;
    case UsbControlTransferRecipient::ENDPOINT:
      flags |= BMREQUEST_TO_ENDPOINT;
      break;
    case UsbControlTransferRecipient::OTHER:
      flags |= BMREQUEST_TO_OTHER;
      break;
  }

  return flags;
}

}  // namespace

// Encapsulates waiting for the completion of an overlapped event.
class UsbDeviceHandleWin::Request : public base::win::ObjectWatcher::Delegate {
 public:
  explicit Request(HANDLE handle)
      : handle_(handle), event_(CreateEvent(nullptr, false, false, nullptr)) {
    memset(&overlapped_, 0, sizeof(overlapped_));
    overlapped_.hEvent = event_.Get();
  }

  ~Request() override {
    if (callback_) {
      SetLastError(ERROR_REQUEST_ABORTED);
      std::move(callback_).Run(this, false, 0);
    }
  }

  // Starts watching for completion of the overlapped event.
  void MaybeStartWatching(
      BOOL success,
      base::OnceCallback<void(Request*, DWORD, size_t)> callback) {
    callback_ = std::move(callback);
    if (success) {
      OnObjectSignaled(event_.Get());
    } else {
      DWORD error = GetLastError();
      if (error == ERROR_IO_PENDING) {
        watcher_.StartWatchingOnce(event_.Get(), this);
      } else {
        std::move(callback_).Run(this, error, 0);
      }
    }
  }

  OVERLAPPED* overlapped() { return &overlapped_; }

  // base::win::ObjectWatcher::Delegate
  void OnObjectSignaled(HANDLE object) override {
    DCHECK_EQ(object, event_.Get());
    DWORD size;
    if (GetOverlappedResult(handle_, &overlapped_, &size, true))
      std::move(callback_).Run(this, ERROR_SUCCESS, size);
    else
      std::move(callback_).Run(this, GetLastError(), 0);
  }

 private:
  HANDLE handle_;
  OVERLAPPED overlapped_;
  base::win::ScopedHandle event_;
  base::win::ObjectWatcher watcher_;
  base::OnceCallback<void(Request*, DWORD, size_t)> callback_;

  DISALLOW_COPY_AND_ASSIGN(Request);
};

UsbDeviceHandleWin::Interface::Interface() = default;

UsbDeviceHandleWin::Interface::~Interface() = default;

scoped_refptr<UsbDevice> UsbDeviceHandleWin::GetDevice() const {
  return device_;
}

void UsbDeviceHandleWin::Close() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!device_)
    return;

  device_->HandleClosed(this);
  device_ = nullptr;

  if (hub_handle_.IsValid()) {
    CancelIo(hub_handle_.Get());
    hub_handle_.Close();
  }

  requests_.clear();
}

void UsbDeviceHandleWin::SetConfiguration(int configuration_value,
                                          ResultCallback callback) {
  DCHECK(thread_checker_.CalledOnValidThread());
  // Setting device configuration is not supported on Windows.
  task_runner_->PostTask(FROM_HERE, base::BindOnce(std::move(callback), false));
}

void UsbDeviceHandleWin::ClaimInterface(int interface_number,
                                        ResultCallback callback) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!device_) {
    task_runner_->PostTask(FROM_HERE,
                           base::BindOnce(std::move(callback), false));
    return;
  }

  auto interface_it = interfaces_.find(interface_number);
  if (interface_it == interfaces_.end()) {
    task_runner_->PostTask(FROM_HERE,
                           base::BindOnce(std::move(callback), false));
    return;
  }
  Interface* interface = &interface_it->second;

  if (!OpenInterfaceHandle(interface)) {
    task_runner_->PostTask(FROM_HERE,
                           base::BindOnce(std::move(callback), false));
    return;
  }

  interface->claimed = true;
  task_runner_->PostTask(FROM_HERE, base::BindOnce(std::move(callback), true));
}

void UsbDeviceHandleWin::ReleaseInterface(int interface_number,
                                          ResultCallback callback) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!device_) {
    task_runner_->PostTask(FROM_HERE,
                           base::BindOnce(std::move(callback), false));
    return;
  }

  auto interface_it = interfaces_.find(interface_number);
  if (interface_it == interfaces_.end()) {
    task_runner_->PostTask(FROM_HERE,
                           base::BindOnce(std::move(callback), false));
    return;
  }
  Interface* interface = &interface_it->second;

  if (interface->handle.IsValid()) {
    interface->handle.Close();
    interface->alternate_setting = 0;
  }

  task_runner_->PostTask(FROM_HERE, base::BindOnce(std::move(callback), true));
}

void UsbDeviceHandleWin::SetInterfaceAlternateSetting(int interface_number,
                                                      int alternate_setting,
                                                      ResultCallback callback) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!device_) {
    task_runner_->PostTask(FROM_HERE,
                           base::BindOnce(std::move(callback), false));
    return;
  }

  // TODO: Unimplemented.
  task_runner_->PostTask(FROM_HERE, base::BindOnce(std::move(callback), false));
}

void UsbDeviceHandleWin::ResetDevice(ResultCallback callback) {
  DCHECK(thread_checker_.CalledOnValidThread());
  // Resetting the device is not supported on Windows.
  task_runner_->PostTask(FROM_HERE, base::BindOnce(std::move(callback), false));
}

void UsbDeviceHandleWin::ClearHalt(uint8_t endpoint, ResultCallback callback) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!device_) {
    task_runner_->PostTask(FROM_HERE,
                           base::BindOnce(std::move(callback), false));
    return;
  }

  // TODO: Unimplemented.
  task_runner_->PostTask(FROM_HERE, base::BindOnce(std::move(callback), false));
}

void UsbDeviceHandleWin::ControlTransfer(
    UsbTransferDirection direction,
    UsbControlTransferType request_type,
    UsbControlTransferRecipient recipient,
    uint8_t request,
    uint16_t value,
    uint16_t index,
    scoped_refptr<base::RefCountedBytes> buffer,
    unsigned int timeout,
    TransferCallback callback) {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (!device_) {
    task_runner_->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback),
                                  UsbTransferStatus::DISCONNECT, nullptr, 0));
    return;
  }

  if (hub_handle_.IsValid()) {
    if (direction == UsbTransferDirection::INBOUND &&
        request_type == UsbControlTransferType::STANDARD &&
        recipient == UsbControlTransferRecipient::DEVICE &&
        request == USB_REQUEST_GET_DESCRIPTOR) {
      if ((value >> 8) == USB_DEVICE_DESCRIPTOR_TYPE) {
        auto* node_connection_info = new USB_NODE_CONNECTION_INFORMATION_EX;
        node_connection_info->ConnectionIndex = device_->port_number();

        Request* request = MakeRequest(hub_handle_.Get());
        request->MaybeStartWatching(
            DeviceIoControl(hub_handle_.Get(),
                            IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX,
                            node_connection_info, sizeof(*node_connection_info),
                            node_connection_info, sizeof(*node_connection_info),
                            nullptr, request->overlapped()),
            base::BindOnce(&UsbDeviceHandleWin::GotNodeConnectionInformation,
                           weak_factory_.GetWeakPtr(), std::move(callback),
                           base::Owned(node_connection_info), buffer));
        return;
      } else if (((value >> 8) == USB_CONFIGURATION_DESCRIPTOR_TYPE) ||
                 ((value >> 8) == USB_STRING_DESCRIPTOR_TYPE)) {
        size_t size = sizeof(USB_DESCRIPTOR_REQUEST) + buffer->size();
        auto request_buffer = base::MakeRefCounted<base::RefCountedBytes>(size);
        USB_DESCRIPTOR_REQUEST* descriptor_request =
            request_buffer->front_as<USB_DESCRIPTOR_REQUEST>();
        descriptor_request->ConnectionIndex = device_->port_number();
        descriptor_request->SetupPacket.bmRequest = BMREQUEST_DEVICE_TO_HOST;
        descriptor_request->SetupPacket.bRequest = USB_REQUEST_GET_DESCRIPTOR;
        descriptor_request->SetupPacket.wValue = value;
        descriptor_request->SetupPacket.wIndex = index;
        descriptor_request->SetupPacket.wLength = buffer->size();

        Request* request = MakeRequest(hub_handle_.Get());
        request->MaybeStartWatching(
            DeviceIoControl(hub_handle_.Get(),
                            IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION,
                            request_buffer->front(), size,
                            request_buffer->front(), size, nullptr,
                            request->overlapped()),
            base::BindOnce(&UsbDeviceHandleWin::GotDescriptorFromNodeConnection,
                           weak_factory_.GetWeakPtr(), std::move(callback),
                           request_buffer, buffer));
        return;
      }
    }

    // Unsupported transfer for hub.
    task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(std::move(callback), UsbTransferStatus::TRANSFER_ERROR,
                       nullptr, 0));
    return;
  }

  // Submit a normal control transfer.
  WINUSB_INTERFACE_HANDLE handle =
      GetInterfaceForControlTransfer(recipient, index);
  if (handle == INVALID_HANDLE_VALUE) {
    USB_LOG(ERROR) << "Interface handle not available for control transfer.";
    task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(std::move(callback), UsbTransferStatus::TRANSFER_ERROR,
                       nullptr, 0));
    return;
  }

  WINUSB_SETUP_PACKET setup = {0};
  setup.RequestType = BuildRequestFlags(direction, request_type, recipient);
  setup.Request = request;
  setup.Value = value;
  setup.Index = index;
  setup.Length = buffer->size();

  Request* control_request = MakeRequest(handle);
  control_request->MaybeStartWatching(
      WinUsb_ControlTransfer(handle, setup, buffer->front(), buffer->size(),
                             nullptr, control_request->overlapped()),
      base::BindOnce(&UsbDeviceHandleWin::TransferComplete,
                     weak_factory_.GetWeakPtr(), std::move(callback), buffer));
}

void UsbDeviceHandleWin::IsochronousTransferIn(
    uint8_t endpoint_number,
    const std::vector<uint32_t>& packet_lengths,
    unsigned int timeout,
    IsochronousTransferCallback callback) {
  DCHECK(thread_checker_.CalledOnValidThread());
  // Isochronous is not yet supported on Windows.
  ReportIsochronousError(packet_lengths, std::move(callback),
                         UsbTransferStatus::TRANSFER_ERROR);
}

void UsbDeviceHandleWin::IsochronousTransferOut(
    uint8_t endpoint_number,
    scoped_refptr<base::RefCountedBytes> buffer,
    const std::vector<uint32_t>& packet_lengths,
    unsigned int timeout,
    IsochronousTransferCallback callback) {
  DCHECK(thread_checker_.CalledOnValidThread());
  // Isochronous is not yet supported on Windows.
  ReportIsochronousError(packet_lengths, std::move(callback),
                         UsbTransferStatus::TRANSFER_ERROR);
}

void UsbDeviceHandleWin::GenericTransfer(
    UsbTransferDirection direction,
    uint8_t endpoint_number,
    scoped_refptr<base::RefCountedBytes> buffer,
    unsigned int timeout,
    TransferCallback callback) {
  if (task_runner_->BelongsToCurrentThread()) {
    GenericTransferInternal(direction, endpoint_number, std::move(buffer),
                            timeout, std::move(callback), task_runner_);
  } else {
    task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&UsbDeviceHandleWin::GenericTransferInternal, this,
                       direction, endpoint_number, std::move(buffer), timeout,
                       std::move(callback),
                       base::ThreadTaskRunnerHandle::Get()));
  }
}

const UsbInterfaceDescriptor* UsbDeviceHandleWin::FindInterfaceByEndpoint(
    uint8_t endpoint_address) {
  DCHECK(thread_checker_.CalledOnValidThread());
  auto it = endpoints_.find(endpoint_address);
  if (it != endpoints_.end())
    return it->second.interface;
  return nullptr;
}

UsbDeviceHandleWin::UsbDeviceHandleWin(
    scoped_refptr<UsbDeviceWin> device,
    bool composite,
    scoped_refptr<base::SequencedTaskRunner> blocking_task_runner)
    : device_(std::move(device)),
      task_runner_(base::ThreadTaskRunnerHandle::Get()),
      blocking_task_runner_(std::move(blocking_task_runner)),
      weak_factory_(this) {
  DCHECK(!composite);
  // Windows only supports configuration 1, which therefore must be active.
  DCHECK(device_->active_configuration());

  for (const auto& interface : device_->active_configuration()->interfaces) {
    if (interface.alternate_setting != 0)
      continue;

    Interface& interface_info = interfaces_[interface.interface_number];
    interface_info.interface_number = interface.interface_number;
    interface_info.first_interface = interface.first_interface;
    RegisterEndpoints(interface);
  }
}

UsbDeviceHandleWin::UsbDeviceHandleWin(
    scoped_refptr<UsbDeviceWin> device,
    base::win::ScopedHandle handle,
    scoped_refptr<base::SequencedTaskRunner> blocking_task_runner)
    : device_(std::move(device)),
      hub_handle_(std::move(handle)),
      task_runner_(base::ThreadTaskRunnerHandle::Get()),
      blocking_task_runner_(std::move(blocking_task_runner)),
      weak_factory_(this) {}

UsbDeviceHandleWin::~UsbDeviceHandleWin() {}

bool UsbDeviceHandleWin::OpenInterfaceHandle(Interface* interface) {
  if (interface->handle.IsValid())
    return true;

  WINUSB_INTERFACE_HANDLE handle;
  if (interface->first_interface == interface->interface_number) {
    if (!function_handle_.IsValid()) {
      function_handle_.Set(CreateFileA(
          device_->device_path().c_str(), GENERIC_READ | GENERIC_WRITE,
          FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
          FILE_FLAG_OVERLAPPED, nullptr));
      if (!function_handle_.IsValid()) {
        USB_PLOG(ERROR) << "Failed to open " << device_->device_path();
        return false;
      }
    }

    if (!WinUsb_Initialize(function_handle_.Get(), &handle)) {
      USB_PLOG(ERROR) << "Failed to initialize WinUSB handle";
      return false;
    }
  } else {
    auto first_interface_it = interfaces_.find(interface->first_interface);
    DCHECK(first_interface_it != interfaces_.end());
    Interface* first_interface = &first_interface_it->second;

    if (!OpenInterfaceHandle(first_interface))
      return false;

    int index = interface->interface_number - interface->first_interface - 1;
    if (!WinUsb_GetAssociatedInterface(first_interface->handle.Get(), index,
                                       &handle)) {
      USB_PLOG(ERROR) << "Failed to get associated interface " << index
                      << " from interface "
                      << static_cast<int>(interface->first_interface);
      return false;
    }
  }

  interface->handle.Set(handle);
  return interface->handle.IsValid();
}

void UsbDeviceHandleWin::RegisterEndpoints(
    const UsbInterfaceDescriptor& interface) {
  for (const auto& endpoint : interface.endpoints) {
    Endpoint& endpoint_info = endpoints_[endpoint.address];
    endpoint_info.interface = &interface;
    endpoint_info.type = endpoint.transfer_type;
  }
}

void UsbDeviceHandleWin::UnregisterEndpoints(
    const UsbInterfaceDescriptor& interface) {
  for (const auto& endpoint : interface.endpoints)
    endpoints_.erase(endpoint.address);
}

WINUSB_INTERFACE_HANDLE UsbDeviceHandleWin::GetInterfaceForControlTransfer(
    UsbControlTransferRecipient recipient,
    uint16_t index) {
  if (recipient == UsbControlTransferRecipient::ENDPOINT) {
    auto endpoint_it = endpoints_.find(index & 0xff);
    if (endpoint_it == endpoints_.end())
      return INVALID_HANDLE_VALUE;

    // "Fall through" to the interface case.
    recipient = UsbControlTransferRecipient::INTERFACE;
    index = endpoint_it->second.interface->interface_number;
  }

  Interface* interface;
  if (recipient == UsbControlTransferRecipient::INTERFACE) {
    auto interface_it = interfaces_.find(index & 0xff);
    if (interface_it == interfaces_.end())
      return INVALID_HANDLE_VALUE;

    interface = &interface_it->second;
  } else {
    // TODO: To support composite devices a particular function handle must be
    // chosen, probably arbitrarily.
    interface = &interfaces_[0];
  }

  OpenInterfaceHandle(interface);
  return interface->handle.Get();
}

UsbDeviceHandleWin::Request* UsbDeviceHandleWin::MakeRequest(HANDLE handle) {
  auto request = std::make_unique<Request>(hub_handle_.Get());
  Request* request_ptr = request.get();
  requests_[request_ptr] = std::move(request);
  return request_ptr;
}

std::unique_ptr<UsbDeviceHandleWin::Request> UsbDeviceHandleWin::UnlinkRequest(
    UsbDeviceHandleWin::Request* request_ptr) {
  auto it = requests_.find(request_ptr);
  DCHECK(it != requests_.end());
  std::unique_ptr<Request> request = std::move(it->second);
  requests_.erase(it);
  return request;
}

void UsbDeviceHandleWin::GotNodeConnectionInformation(
    TransferCallback callback,
    void* node_connection_info_ptr,
    scoped_refptr<base::RefCountedBytes> buffer,
    Request* request_ptr,
    DWORD win32_result,
    size_t bytes_transferred) {
  USB_NODE_CONNECTION_INFORMATION_EX* node_connection_info =
      static_cast<USB_NODE_CONNECTION_INFORMATION_EX*>(
          node_connection_info_ptr);
  std::unique_ptr<Request> request = UnlinkRequest(request_ptr);

  if (win32_result != ERROR_SUCCESS) {
    SetLastError(win32_result);
    USB_PLOG(ERROR) << "Failed to get node connection information";
    std::move(callback).Run(UsbTransferStatus::TRANSFER_ERROR, nullptr, 0);
    return;
  }

  DCHECK_EQ(bytes_transferred, sizeof(USB_NODE_CONNECTION_INFORMATION_EX));
  bytes_transferred = std::min(sizeof(USB_DEVICE_DESCRIPTOR), buffer->size());
  memcpy(buffer->front(), &node_connection_info->DeviceDescriptor,
         bytes_transferred);
  std::move(callback).Run(UsbTransferStatus::COMPLETED, buffer,
                          bytes_transferred);
}

void UsbDeviceHandleWin::GotDescriptorFromNodeConnection(
    TransferCallback callback,
    scoped_refptr<base::RefCountedBytes> request_buffer,
    scoped_refptr<base::RefCountedBytes> original_buffer,
    Request* request_ptr,
    DWORD win32_result,
    size_t bytes_transferred) {
  std::unique_ptr<Request> request = UnlinkRequest(request_ptr);

  if (win32_result != ERROR_SUCCESS) {
    SetLastError(win32_result);
    USB_PLOG(ERROR) << "Failed to read descriptor from node connection";
    std::move(callback).Run(UsbTransferStatus::TRANSFER_ERROR, nullptr, 0);
    return;
  }

  DCHECK_GE(bytes_transferred, sizeof(USB_DESCRIPTOR_REQUEST));
  bytes_transferred -= sizeof(USB_DESCRIPTOR_REQUEST);
  DCHECK_LE(bytes_transferred, original_buffer->size());
  memcpy(original_buffer->front(),
         request_buffer->front() + sizeof(USB_DESCRIPTOR_REQUEST),
         bytes_transferred);
  std::move(callback).Run(UsbTransferStatus::COMPLETED, original_buffer,
                          bytes_transferred);
}

void UsbDeviceHandleWin::TransferComplete(
    TransferCallback callback,
    scoped_refptr<base::RefCountedBytes> buffer,
    Request* request_ptr,
    DWORD win32_result,
    size_t bytes_transferred) {
  std::unique_ptr<Request> request = UnlinkRequest(request_ptr);

  if (win32_result != ERROR_SUCCESS) {
    SetLastError(win32_result);
    USB_PLOG(ERROR) << "Transfer failed";
    std::move(callback).Run(UsbTransferStatus::TRANSFER_ERROR, nullptr, 0);
    return;
  }

  std::move(callback).Run(UsbTransferStatus::COMPLETED, std::move(buffer),
                          bytes_transferred);
}

void UsbDeviceHandleWin::GenericTransferInternal(
    UsbTransferDirection direction,
    uint8_t endpoint_number,
    scoped_refptr<base::RefCountedBytes> buffer,
    unsigned int timeout,
    TransferCallback callback,
    scoped_refptr<base::SingleThreadTaskRunner> callback_task_runner) {
  DCHECK(thread_checker_.CalledOnValidThread());
  uint8_t endpoint_address = endpoint_number;
  if (direction == UsbTransferDirection::INBOUND)
    endpoint_address |= 0x80;

  auto endpoint_it = endpoints_.find(endpoint_address);
  if (endpoint_it == endpoints_.end()) {
    callback_task_runner->PostTask(
        FROM_HERE,
        base::BindOnce(std::move(callback), UsbTransferStatus::TRANSFER_ERROR,
                       nullptr, 0));
    return;
  }

  auto interface_it =
      interfaces_.find(endpoint_it->second.interface->interface_number);
  DCHECK(interface_it != interfaces_.end());
  Interface* interface = &interface_it->second;
  if (!interface->claimed) {
    callback_task_runner->PostTask(
        FROM_HERE,
        base::BindOnce(std::move(callback), UsbTransferStatus::TRANSFER_ERROR,
                       nullptr, 0));
    return;
  }

  DCHECK(interface->handle.IsValid());
  Request* request = MakeRequest(interface->handle.Get());
  BOOL result;
  if (direction == UsbTransferDirection::INBOUND) {
    result = WinUsb_ReadPipe(interface->handle.Get(), endpoint_address,
                             buffer->front(), buffer->size(), nullptr,
                             request->overlapped());
  } else {
    result = WinUsb_WritePipe(interface->handle.Get(), endpoint_address,
                              buffer->front(), buffer->size(), nullptr,
                              request->overlapped());
  }
  request->MaybeStartWatching(
      result, base::BindOnce(&UsbDeviceHandleWin::TransferComplete,
                             weak_factory_.GetWeakPtr(), std::move(callback),
                             std::move(buffer)));
}

void UsbDeviceHandleWin::ReportIsochronousError(
    const std::vector<uint32_t>& packet_lengths,
    IsochronousTransferCallback callback,
    UsbTransferStatus status) {
  std::vector<IsochronousPacket> packets(packet_lengths.size());
  for (size_t i = 0; i < packet_lengths.size(); ++i) {
    packets[i].length = packet_lengths[i];
    packets[i].transferred_length = 0;
    packets[i].status = status;
  }
  task_runner_->PostTask(FROM_HERE,
                         base::BindOnce(std::move(callback), nullptr, packets));
}

}  // namespace device
