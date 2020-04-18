// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/usb/usb_device_handle_impl.h"

#include <algorithm>
#include <memory>
#include <numeric>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/memory/ref_counted_memory.h"
#include "base/sequence_checker.h"
#include "base/single_thread_task_runner.h"
#include "base/stl_util.h"
#include "base/strings/string16.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/device_event_log/device_event_log.h"
#include "device/usb/usb_context.h"
#include "device/usb/usb_descriptors.h"
#include "device/usb/usb_device_impl.h"
#include "device/usb/usb_error.h"
#include "device/usb/usb_service.h"
#include "third_party/libusb/src/libusb/libusb.h"

namespace device {

void HandleTransferCompletion(PlatformUsbTransferHandle transfer);

namespace {

uint8_t ConvertTransferDirection(UsbTransferDirection direction) {
  switch (direction) {
    case UsbTransferDirection::INBOUND:
      return LIBUSB_ENDPOINT_IN;
    case UsbTransferDirection::OUTBOUND:
      return LIBUSB_ENDPOINT_OUT;
  }
  NOTREACHED();
  return 0;
}

uint8_t CreateRequestType(UsbTransferDirection direction,
                          UsbControlTransferType request_type,
                          UsbControlTransferRecipient recipient) {
  uint8_t result = ConvertTransferDirection(direction);

  switch (request_type) {
    case UsbControlTransferType::STANDARD:
      result |= LIBUSB_REQUEST_TYPE_STANDARD;
      break;
    case UsbControlTransferType::CLASS:
      result |= LIBUSB_REQUEST_TYPE_CLASS;
      break;
    case UsbControlTransferType::VENDOR:
      result |= LIBUSB_REQUEST_TYPE_VENDOR;
      break;
    case UsbControlTransferType::RESERVED:
      result |= LIBUSB_REQUEST_TYPE_RESERVED;
      break;
  }

  switch (recipient) {
    case UsbControlTransferRecipient::DEVICE:
      result |= LIBUSB_RECIPIENT_DEVICE;
      break;
    case UsbControlTransferRecipient::INTERFACE:
      result |= LIBUSB_RECIPIENT_INTERFACE;
      break;
    case UsbControlTransferRecipient::ENDPOINT:
      result |= LIBUSB_RECIPIENT_ENDPOINT;
      break;
    case UsbControlTransferRecipient::OTHER:
      result |= LIBUSB_RECIPIENT_OTHER;
      break;
  }

  return result;
}

static UsbTransferStatus ConvertTransferStatus(
    const libusb_transfer_status status) {
  switch (status) {
    case LIBUSB_TRANSFER_COMPLETED:
      return UsbTransferStatus::COMPLETED;
    case LIBUSB_TRANSFER_ERROR:
      return UsbTransferStatus::TRANSFER_ERROR;
    case LIBUSB_TRANSFER_TIMED_OUT:
      return UsbTransferStatus::TIMEOUT;
    case LIBUSB_TRANSFER_STALL:
      return UsbTransferStatus::STALLED;
    case LIBUSB_TRANSFER_NO_DEVICE:
      return UsbTransferStatus::DISCONNECT;
    case LIBUSB_TRANSFER_OVERFLOW:
      return UsbTransferStatus::BABBLE;
    case LIBUSB_TRANSFER_CANCELLED:
      return UsbTransferStatus::CANCELLED;
  }
  NOTREACHED();
  return UsbTransferStatus::TRANSFER_ERROR;
}

static void RunTransferCallback(
    scoped_refptr<base::TaskRunner> callback_task_runner,
    UsbDeviceHandle::TransferCallback callback,
    UsbTransferStatus status,
    scoped_refptr<base::RefCountedBytes> buffer,
    size_t result) {
  if (callback_task_runner->RunsTasksInCurrentSequence()) {
    std::move(callback).Run(status, buffer, result);
  } else {
    callback_task_runner->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), status, buffer, result));
  }
}

void ReportIsochronousTransferError(
    scoped_refptr<base::TaskRunner> callback_task_runner,
    UsbDeviceHandle::IsochronousTransferCallback callback,
    const std::vector<uint32_t> packet_lengths,
    UsbTransferStatus status) {
  std::vector<UsbDeviceHandle::IsochronousPacket> packets(
      packet_lengths.size());
  for (size_t i = 0; i < packet_lengths.size(); ++i) {
    packets[i].length = packet_lengths[i];
    packets[i].transferred_length = 0;
    packets[i].status = status;
  }
  if (callback_task_runner->RunsTasksInCurrentSequence()) {
    std::move(callback).Run(nullptr, packets);
  } else {
    callback_task_runner->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), nullptr, packets));
  }
}

}  // namespace

class UsbDeviceHandleImpl::InterfaceClaimer
    : public base::RefCountedThreadSafe<UsbDeviceHandleImpl::InterfaceClaimer> {
 public:
  InterfaceClaimer(scoped_refptr<UsbDeviceHandleImpl> handle,
                   int interface_number,
                   scoped_refptr<base::SingleThreadTaskRunner> task_runner);

  int interface_number() const { return interface_number_; }
  int alternate_setting() const { return alternate_setting_; }
  void set_alternate_setting(const int alternate_setting) {
    alternate_setting_ = alternate_setting;
  }

  void set_release_callback(ResultCallback callback) {
    release_callback_ = std::move(callback);
  }

 private:
  friend class base::RefCountedThreadSafe<InterfaceClaimer>;
  ~InterfaceClaimer();

  const scoped_refptr<UsbDeviceHandleImpl> handle_;
  const int interface_number_;
  int alternate_setting_;
  const scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  ResultCallback release_callback_;
  base::SequenceChecker sequence_checker_;

  DISALLOW_COPY_AND_ASSIGN(InterfaceClaimer);
};

UsbDeviceHandleImpl::InterfaceClaimer::InterfaceClaimer(
    scoped_refptr<UsbDeviceHandleImpl> handle,
    int interface_number,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : handle_(handle),
      interface_number_(interface_number),
      alternate_setting_(0),
      task_runner_(task_runner) {}

UsbDeviceHandleImpl::InterfaceClaimer::~InterfaceClaimer() {
  DCHECK(sequence_checker_.CalledOnValidSequence());
  int rc = libusb_release_interface(handle_->handle(), interface_number_);
  if (rc != LIBUSB_SUCCESS) {
    USB_LOG(DEBUG) << "Failed to release interface: "
                   << ConvertPlatformUsbErrorToString(rc);
  }
  if (!release_callback_.is_null()) {
    task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(std::move(release_callback_), rc == LIBUSB_SUCCESS));
  }
}

// This inner class owns the underlying libusb_transfer and may outlast
// the UsbDeviceHandle that created it.
class UsbDeviceHandleImpl::Transfer {
 public:
  // These functions takes |*callback| if they successfully create Transfer
  // instance, otherwise |*callback| left unchanged.
  static std::unique_ptr<Transfer> CreateControlTransfer(
      scoped_refptr<UsbDeviceHandleImpl> device_handle,
      uint8_t type,
      uint8_t request,
      uint16_t value,
      uint16_t index,
      uint16_t length,
      scoped_refptr<base::RefCountedBytes> buffer,
      unsigned int timeout,
      scoped_refptr<base::TaskRunner> callback_task_runner,
      TransferCallback* callback);
  static std::unique_ptr<Transfer> CreateBulkTransfer(
      scoped_refptr<UsbDeviceHandleImpl> device_handle,
      uint8_t endpoint,
      scoped_refptr<base::RefCountedBytes> buffer,
      int length,
      unsigned int timeout,
      scoped_refptr<base::TaskRunner> callback_task_runner,
      TransferCallback* callback);
  static std::unique_ptr<Transfer> CreateInterruptTransfer(
      scoped_refptr<UsbDeviceHandleImpl> device_handle,
      uint8_t endpoint,
      scoped_refptr<base::RefCountedBytes> buffer,
      int length,
      unsigned int timeout,
      scoped_refptr<base::TaskRunner> callback_task_runner,
      TransferCallback* callback);
  static std::unique_ptr<Transfer> CreateIsochronousTransfer(
      scoped_refptr<UsbDeviceHandleImpl> device_handle,
      uint8_t endpoint,
      scoped_refptr<base::RefCountedBytes> buffer,
      size_t length,
      const std::vector<uint32_t>& packet_lengths,
      unsigned int timeout,
      scoped_refptr<base::TaskRunner> callback_task_runner,
      IsochronousTransferCallback* callback);

  ~Transfer();

  void Submit();
  void Cancel();
  void ProcessCompletion();
  void TransferComplete(UsbTransferStatus status, size_t bytes_transferred);

  const UsbDeviceHandleImpl::InterfaceClaimer* claimed_interface() const {
    return claimed_interface_.get();
  }

  scoped_refptr<base::TaskRunner> callback_task_runner() const {
    return callback_task_runner_;
  }

 private:
  Transfer(scoped_refptr<UsbDeviceHandleImpl> device_handle,
           scoped_refptr<InterfaceClaimer> claimed_interface,
           UsbTransferType transfer_type,
           scoped_refptr<base::RefCountedBytes> buffer,
           size_t length,
           scoped_refptr<base::TaskRunner> callback_task_runner,
           TransferCallback callback);
  Transfer(scoped_refptr<UsbDeviceHandleImpl> device_handle,
           scoped_refptr<InterfaceClaimer> claimed_interface,
           scoped_refptr<base::RefCountedBytes> buffer,
           scoped_refptr<base::TaskRunner> callback_task_runner,
           IsochronousTransferCallback callback);

  static void LIBUSB_CALL PlatformCallback(PlatformUsbTransferHandle handle);

  void IsochronousTransferComplete();

  UsbTransferType transfer_type_;
  scoped_refptr<UsbDeviceHandleImpl> device_handle_;
  PlatformUsbTransferHandle platform_transfer_ = nullptr;
  scoped_refptr<base::RefCountedBytes> buffer_;
  scoped_refptr<UsbDeviceHandleImpl::InterfaceClaimer> claimed_interface_;
  size_t length_;
  bool cancelled_ = false;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  scoped_refptr<base::TaskRunner> callback_task_runner_;
  TransferCallback callback_;
  IsochronousTransferCallback iso_callback_;
};

// static
std::unique_ptr<UsbDeviceHandleImpl::Transfer>
UsbDeviceHandleImpl::Transfer::CreateControlTransfer(
    scoped_refptr<UsbDeviceHandleImpl> device_handle,
    uint8_t type,
    uint8_t request,
    uint16_t value,
    uint16_t index,
    uint16_t length,
    scoped_refptr<base::RefCountedBytes> buffer,
    unsigned int timeout,
    scoped_refptr<base::TaskRunner> callback_task_runner,
    TransferCallback* callback) {
  std::unique_ptr<Transfer> transfer(
      new Transfer(device_handle, nullptr, UsbTransferType::CONTROL, buffer,
                   length + LIBUSB_CONTROL_SETUP_SIZE, callback_task_runner,
                   std::move(*callback)));

  transfer->platform_transfer_ = libusb_alloc_transfer(0);
  if (!transfer->platform_transfer_) {
    USB_LOG(ERROR) << "Failed to allocate control transfer.";
    *callback = std::move(transfer->callback_);
    return nullptr;
  }

  libusb_fill_control_setup(buffer->front(), type, request, value, index,
                            length);
  libusb_fill_control_transfer(transfer->platform_transfer_,
                               device_handle->handle_, buffer->front(),
                               &UsbDeviceHandleImpl::Transfer::PlatformCallback,
                               transfer.get(), timeout);

  return transfer;
}

// static
std::unique_ptr<UsbDeviceHandleImpl::Transfer>
UsbDeviceHandleImpl::Transfer::CreateBulkTransfer(
    scoped_refptr<UsbDeviceHandleImpl> device_handle,
    uint8_t endpoint,
    scoped_refptr<base::RefCountedBytes> buffer,
    int length,
    unsigned int timeout,
    scoped_refptr<base::TaskRunner> callback_task_runner,
    TransferCallback* callback) {
  std::unique_ptr<Transfer> transfer(new Transfer(
      device_handle, device_handle->GetClaimedInterfaceForEndpoint(endpoint),
      UsbTransferType::BULK, buffer, length, callback_task_runner,
      std::move(*callback)));

  transfer->platform_transfer_ = libusb_alloc_transfer(0);
  if (!transfer->platform_transfer_) {
    USB_LOG(ERROR) << "Failed to allocate bulk transfer.";
    *callback = std::move(transfer->callback_);
    return nullptr;
  }

  libusb_fill_bulk_transfer(
      transfer->platform_transfer_, device_handle->handle_, endpoint,
      buffer->front(), length, &UsbDeviceHandleImpl::Transfer::PlatformCallback,
      transfer.get(), timeout);

  return transfer;
}

// static
std::unique_ptr<UsbDeviceHandleImpl::Transfer>
UsbDeviceHandleImpl::Transfer::CreateInterruptTransfer(
    scoped_refptr<UsbDeviceHandleImpl> device_handle,
    uint8_t endpoint,
    scoped_refptr<base::RefCountedBytes> buffer,
    int length,
    unsigned int timeout,
    scoped_refptr<base::TaskRunner> callback_task_runner,
    TransferCallback* callback) {
  std::unique_ptr<Transfer> transfer(new Transfer(
      device_handle, device_handle->GetClaimedInterfaceForEndpoint(endpoint),
      UsbTransferType::INTERRUPT, buffer, length, callback_task_runner,
      std::move(*callback)));

  transfer->platform_transfer_ = libusb_alloc_transfer(0);
  if (!transfer->platform_transfer_) {
    USB_LOG(ERROR) << "Failed to allocate interrupt transfer.";
    *callback = std::move(transfer->callback_);
    return nullptr;
  }

  libusb_fill_interrupt_transfer(
      transfer->platform_transfer_, device_handle->handle_, endpoint,
      buffer->front(), length, &UsbDeviceHandleImpl::Transfer::PlatformCallback,
      transfer.get(), timeout);

  return transfer;
}

// static
std::unique_ptr<UsbDeviceHandleImpl::Transfer>
UsbDeviceHandleImpl::Transfer::CreateIsochronousTransfer(
    scoped_refptr<UsbDeviceHandleImpl> device_handle,
    uint8_t endpoint,
    scoped_refptr<base::RefCountedBytes> buffer,
    size_t length,
    const std::vector<uint32_t>& packet_lengths,
    unsigned int timeout,
    scoped_refptr<base::TaskRunner> callback_task_runner,
    IsochronousTransferCallback* callback) {
  std::unique_ptr<Transfer> transfer(new Transfer(
      device_handle, device_handle->GetClaimedInterfaceForEndpoint(endpoint),
      buffer, callback_task_runner, std::move(*callback)));

  int num_packets = static_cast<int>(packet_lengths.size());
  transfer->platform_transfer_ = libusb_alloc_transfer(num_packets);
  if (!transfer->platform_transfer_) {
    USB_LOG(ERROR) << "Failed to allocate isochronous transfer.";
    *callback = std::move(transfer->iso_callback_);
    return nullptr;
  }

  libusb_fill_iso_transfer(transfer->platform_transfer_, device_handle->handle_,
                           endpoint, buffer->front(), static_cast<int>(length),
                           num_packets, &Transfer::PlatformCallback,
                           transfer.get(), timeout);

  for (size_t i = 0; i < packet_lengths.size(); ++i)
    transfer->platform_transfer_->iso_packet_desc[i].length = packet_lengths[i];

  return transfer;
}

UsbDeviceHandleImpl::Transfer::Transfer(
    scoped_refptr<UsbDeviceHandleImpl> device_handle,
    scoped_refptr<InterfaceClaimer> claimed_interface,
    UsbTransferType transfer_type,
    scoped_refptr<base::RefCountedBytes> buffer,
    size_t length,
    scoped_refptr<base::TaskRunner> callback_task_runner,
    TransferCallback callback)
    : transfer_type_(transfer_type),
      device_handle_(device_handle),
      buffer_(buffer),
      claimed_interface_(claimed_interface),
      length_(length),
      callback_task_runner_(callback_task_runner),
      callback_(std::move(callback)) {
  task_runner_ = base::ThreadTaskRunnerHandle::Get();
}

UsbDeviceHandleImpl::Transfer::Transfer(
    scoped_refptr<UsbDeviceHandleImpl> device_handle,
    scoped_refptr<InterfaceClaimer> claimed_interface,
    scoped_refptr<base::RefCountedBytes> buffer,
    scoped_refptr<base::TaskRunner> callback_task_runner,
    IsochronousTransferCallback callback)
    : transfer_type_(UsbTransferType::ISOCHRONOUS),
      device_handle_(device_handle),
      buffer_(buffer),
      claimed_interface_(claimed_interface),
      callback_task_runner_(callback_task_runner),
      iso_callback_(std::move(callback)) {
  task_runner_ = base::ThreadTaskRunnerHandle::Get();
}

UsbDeviceHandleImpl::Transfer::~Transfer() {
  if (platform_transfer_) {
    libusb_free_transfer(platform_transfer_);
  }
}

void UsbDeviceHandleImpl::Transfer::Submit() {
  const int rv = libusb_submit_transfer(platform_transfer_);
  if (rv != LIBUSB_SUCCESS) {
    USB_LOG(EVENT) << "Failed to submit transfer: "
                   << ConvertPlatformUsbErrorToString(rv);
    TransferComplete(UsbTransferStatus::TRANSFER_ERROR, 0);
  }
}

void UsbDeviceHandleImpl::Transfer::Cancel() {
  if (!cancelled_) {
    libusb_cancel_transfer(platform_transfer_);
    claimed_interface_ = nullptr;
  }
  cancelled_ = true;
}

void UsbDeviceHandleImpl::Transfer::ProcessCompletion() {
  DCHECK_GE(platform_transfer_->actual_length, 0)
      << "Negative actual length received";
  size_t actual_length =
      static_cast<size_t>(std::max(platform_transfer_->actual_length, 0));

  DCHECK(length_ >= actual_length)
      << "data too big for our buffer (libusb failure?)";

  switch (transfer_type_) {
    case UsbTransferType::CONTROL:
      // If the transfer is a control transfer we do not expose the control
      // setup header to the caller. This logic strips off the header if
      // present before invoking the callback provided with the transfer.
      if (actual_length > 0) {
        CHECK(length_ >= LIBUSB_CONTROL_SETUP_SIZE)
            << "buffer was not correctly set: too small for the control header";

        if (length_ >= (LIBUSB_CONTROL_SETUP_SIZE + actual_length)) {
          auto resized_buffer =
              base::MakeRefCounted<base::RefCountedBytes>(actual_length);
          memcpy(resized_buffer->front(),
                 buffer_->front() + LIBUSB_CONTROL_SETUP_SIZE, actual_length);
          buffer_ = resized_buffer;
        }
      }
      FALLTHROUGH;

    case UsbTransferType::BULK:
    case UsbTransferType::INTERRUPT:
      TransferComplete(ConvertTransferStatus(platform_transfer_->status),
                       actual_length);
      break;

    case UsbTransferType::ISOCHRONOUS:
      IsochronousTransferComplete();
      break;

    default:
      NOTREACHED() << "Invalid usb transfer type";
      break;
  }
}

/* static */
void LIBUSB_CALL UsbDeviceHandleImpl::Transfer::PlatformCallback(
    PlatformUsbTransferHandle platform_transfer) {
  Transfer* transfer =
      reinterpret_cast<Transfer*>(platform_transfer->user_data);
  DCHECK(transfer->platform_transfer_ == platform_transfer);
  transfer->ProcessCompletion();
}

void UsbDeviceHandleImpl::Transfer::TransferComplete(UsbTransferStatus status,
                                                     size_t bytes_transferred) {
  base::OnceClosure closure;
  if (transfer_type_ == UsbTransferType::ISOCHRONOUS) {
    DCHECK_NE(LIBUSB_TRANSFER_COMPLETED, platform_transfer_->status);
    std::vector<IsochronousPacket> packets(platform_transfer_->num_iso_packets);
    for (size_t i = 0; i < packets.size(); ++i) {
      packets[i].length = platform_transfer_->iso_packet_desc[i].length;
      packets[i].transferred_length = 0;
      packets[i].status = status;
    }
    closure = base::BindOnce(std::move(iso_callback_), buffer_, packets);
  } else {
    closure = base::BindOnce(std::move(callback_), status, buffer_,
                             bytes_transferred);
  }
  task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&UsbDeviceHandleImpl::TransferComplete, device_handle_,
                     base::Unretained(this), std::move(closure)));
}

void UsbDeviceHandleImpl::Transfer::IsochronousTransferComplete() {
  std::vector<IsochronousPacket> packets(platform_transfer_->num_iso_packets);
  for (size_t i = 0; i < packets.size(); ++i) {
    packets[i].length = platform_transfer_->iso_packet_desc[i].length;
    packets[i].transferred_length =
        platform_transfer_->iso_packet_desc[i].actual_length;
    packets[i].status =
        ConvertTransferStatus(platform_transfer_->iso_packet_desc[i].status);
  }
  task_runner_->PostTask(FROM_HERE,
                         base::BindOnce(&UsbDeviceHandleImpl::TransferComplete,
                                        device_handle_, base::Unretained(this),
                                        base::BindOnce(std::move(iso_callback_),
                                                       buffer_, packets)));
}

scoped_refptr<UsbDevice> UsbDeviceHandleImpl::GetDevice() const {
  return device_;
}

void UsbDeviceHandleImpl::Close() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!device_)
    return;

  // Cancel all the transfers, their callbacks will be called some time later.
  for (Transfer* transfer : transfers_)
    transfer->Cancel();

  // Release all remaining interfaces once their transfers have completed.
  // This loop must ensure that what may be the final reference is released on
  // the right thread.
  for (auto& map_entry : claimed_interfaces_) {
    InterfaceClaimer* interface_claimer = map_entry.second.get();
    interface_claimer->AddRef();
    map_entry.second = nullptr;
    blocking_task_runner_->ReleaseSoon(FROM_HERE, interface_claimer);
  }

  device_->HandleClosed(this);
  device_ = nullptr;

  // The device handle cannot be closed here. When libusb_cancel_transfer is
  // finished the last references to this device will be released and the
  // destructor will close the handle.
}

void UsbDeviceHandleImpl::SetConfiguration(int configuration_value,
                                           ResultCallback callback) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!device_) {
    std::move(callback).Run(false);
    return;
  }

  for (Transfer* transfer : transfers_) {
    transfer->Cancel();
  }
  claimed_interfaces_.clear();

  blocking_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&UsbDeviceHandleImpl::SetConfigurationOnBlockingThread,
                     this, configuration_value, std::move(callback)));
}

void UsbDeviceHandleImpl::ClaimInterface(int interface_number,
                                         ResultCallback callback) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!device_) {
    std::move(callback).Run(false);
    return;
  }
  if (base::ContainsKey(claimed_interfaces_, interface_number)) {
    std::move(callback).Run(true);
    return;
  }

  blocking_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&UsbDeviceHandleImpl::ClaimInterfaceOnBlockingThread, this,
                     interface_number, std::move(callback)));
}

void UsbDeviceHandleImpl::ReleaseInterface(int interface_number,
                                           ResultCallback callback) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!device_ || !base::ContainsKey(claimed_interfaces_, interface_number)) {
    task_runner_->PostTask(FROM_HERE,
                           base::BindOnce(std::move(callback), false));
    return;
  }

  // Cancel all the transfers on that interface.
  InterfaceClaimer* interface_claimer =
      claimed_interfaces_[interface_number].get();
  for (Transfer* transfer : transfers_) {
    if (transfer->claimed_interface() == interface_claimer) {
      transfer->Cancel();
    }
  }
  interface_claimer->AddRef();
  interface_claimer->set_release_callback(std::move(callback));
  blocking_task_runner_->ReleaseSoon(FROM_HERE, interface_claimer);
  claimed_interfaces_.erase(interface_number);

  RefreshEndpointMap();
}

void UsbDeviceHandleImpl::SetInterfaceAlternateSetting(
    int interface_number,
    int alternate_setting,
    ResultCallback callback) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!device_ || !base::ContainsKey(claimed_interfaces_, interface_number)) {
    std::move(callback).Run(false);
    return;
  }

  blocking_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(
          &UsbDeviceHandleImpl::SetInterfaceAlternateSettingOnBlockingThread,
          this, interface_number, alternate_setting, std::move(callback)));
}

void UsbDeviceHandleImpl::ResetDevice(ResultCallback callback) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!device_) {
    std::move(callback).Run(false);
    return;
  }

  blocking_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&UsbDeviceHandleImpl::ResetDeviceOnBlockingThread, this,
                     std::move(callback)));
}

void UsbDeviceHandleImpl::ClearHalt(uint8_t endpoint, ResultCallback callback) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!device_) {
    std::move(callback).Run(false);
    return;
  }

  InterfaceClaimer* interface_claimer =
      GetClaimedInterfaceForEndpoint(endpoint).get();
  for (Transfer* transfer : transfers_) {
    if (transfer->claimed_interface() == interface_claimer) {
      transfer->Cancel();
    }
  }

  blocking_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&UsbDeviceHandleImpl::ClearHaltOnBlockingThread,
                                this, endpoint, std::move(callback)));
}

void UsbDeviceHandleImpl::ControlTransfer(
    UsbTransferDirection direction,
    UsbControlTransferType request_type,
    UsbControlTransferRecipient recipient,
    uint8_t request,
    uint16_t value,
    uint16_t index,
    scoped_refptr<base::RefCountedBytes> buffer,
    unsigned int timeout,
    TransferCallback callback) {
  if (task_runner_->BelongsToCurrentThread()) {
    ControlTransferInternal(direction, request_type, recipient, request, value,
                            index, buffer, timeout, task_runner_,
                            std::move(callback));
  } else {
    task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&UsbDeviceHandleImpl::ControlTransferInternal,
                                  this, direction, request_type, recipient,
                                  request, value, index, buffer, timeout,
                                  base::ThreadTaskRunnerHandle::Get(),
                                  std::move(callback)));
  }
}

void UsbDeviceHandleImpl::IsochronousTransferIn(
    uint8_t endpoint_number,
    const std::vector<uint32_t>& packet_lengths,
    unsigned int timeout,
    IsochronousTransferCallback callback) {
  uint8_t endpoint_address =
      ConvertTransferDirection(UsbTransferDirection::INBOUND) | endpoint_number;
  if (task_runner_->BelongsToCurrentThread()) {
    IsochronousTransferInInternal(endpoint_address, packet_lengths, timeout,
                                  task_runner_, std::move(callback));
  } else {
    task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&UsbDeviceHandleImpl::IsochronousTransferInInternal,
                       this, endpoint_address, packet_lengths, timeout,
                       base::ThreadTaskRunnerHandle::Get(),
                       std::move(callback)));
  }
}

void UsbDeviceHandleImpl::IsochronousTransferOut(
    uint8_t endpoint_number,
    scoped_refptr<base::RefCountedBytes> buffer,
    const std::vector<uint32_t>& packet_lengths,
    unsigned int timeout,
    IsochronousTransferCallback callback) {
  uint8_t endpoint_address =
      ConvertTransferDirection(UsbTransferDirection::OUTBOUND) |
      endpoint_number;
  if (task_runner_->BelongsToCurrentThread()) {
    IsochronousTransferOutInternal(endpoint_address, buffer, packet_lengths,
                                   timeout, task_runner_, std::move(callback));
  } else {
    task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&UsbDeviceHandleImpl::IsochronousTransferOutInternal,
                       this, endpoint_address, buffer, packet_lengths, timeout,
                       base::ThreadTaskRunnerHandle::Get(),
                       std::move(callback)));
  }
}

void UsbDeviceHandleImpl::GenericTransfer(
    UsbTransferDirection direction,
    uint8_t endpoint_number,
    scoped_refptr<base::RefCountedBytes> buffer,
    unsigned int timeout,
    TransferCallback callback) {
  uint8_t endpoint_address =
      ConvertTransferDirection(direction) | endpoint_number;
  if (task_runner_->BelongsToCurrentThread()) {
    GenericTransferInternal(endpoint_address, buffer, timeout, task_runner_,
                            std::move(callback));
  } else {
    task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&UsbDeviceHandleImpl::GenericTransferInternal,
                                  this, endpoint_address, buffer, timeout,
                                  base::ThreadTaskRunnerHandle::Get(),
                                  std::move(callback)));
  }
}

const UsbInterfaceDescriptor* UsbDeviceHandleImpl::FindInterfaceByEndpoint(
    uint8_t endpoint_address) {
  DCHECK(thread_checker_.CalledOnValidThread());
  const auto endpoint_it = endpoint_map_.find(endpoint_address);
  if (endpoint_it != endpoint_map_.end())
    return endpoint_it->second.interface;
  return nullptr;
}

UsbDeviceHandleImpl::UsbDeviceHandleImpl(
    scoped_refptr<UsbContext> context,
    scoped_refptr<UsbDeviceImpl> device,
    PlatformUsbDeviceHandle handle,
    scoped_refptr<base::SequencedTaskRunner> blocking_task_runner)
    : device_(device),
      handle_(handle),
      context_(context),
      task_runner_(base::ThreadTaskRunnerHandle::Get()),
      blocking_task_runner_(blocking_task_runner) {
  DCHECK(handle) << "Cannot create device with NULL handle.";
}

UsbDeviceHandleImpl::~UsbDeviceHandleImpl() {
  DCHECK(!device_) << "UsbDeviceHandle must be closed before it is destroyed.";

  // This class is RefCountedThreadSafe and so the destructor may be called on
  // any thread. libusb is not safe to reentrancy so be sure not to try to close
  // the device from inside a transfer completion callback.
  if (blocking_task_runner_->RunsTasksInCurrentSequence()) {
    libusb_close(handle_);
  } else {
    blocking_task_runner_->PostTask(FROM_HERE,
                                    base::BindOnce(&libusb_close, handle_));
  }
}

void UsbDeviceHandleImpl::SetConfigurationOnBlockingThread(
    int configuration_value,
    ResultCallback callback) {
  int rv = libusb_set_configuration(handle_, configuration_value);
  if (rv != LIBUSB_SUCCESS) {
    USB_LOG(EVENT) << "Failed to set configuration " << configuration_value
                   << ": " << ConvertPlatformUsbErrorToString(rv);
  }
  task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&UsbDeviceHandleImpl::SetConfigurationComplete, this,
                     rv == LIBUSB_SUCCESS, std::move(callback)));
}

void UsbDeviceHandleImpl::SetConfigurationComplete(bool success,
                                                   ResultCallback callback) {
  if (!device_) {
    std::move(callback).Run(false);
    return;
  }

  if (success) {
    device_->RefreshActiveConfiguration();
    RefreshEndpointMap();
  }
  std::move(callback).Run(success);
}

void UsbDeviceHandleImpl::ClaimInterfaceOnBlockingThread(
    int interface_number,
    ResultCallback callback) {
  int rv = libusb_claim_interface(handle_, interface_number);
  scoped_refptr<InterfaceClaimer> interface_claimer;
  if (rv == LIBUSB_SUCCESS) {
    interface_claimer =
        new InterfaceClaimer(this, interface_number, task_runner_);
  } else {
    USB_LOG(EVENT) << "Failed to claim interface: "
                   << ConvertPlatformUsbErrorToString(rv);
  }
  task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&UsbDeviceHandleImpl::ClaimInterfaceComplete,
                                this, interface_claimer, std::move(callback)));
}

void UsbDeviceHandleImpl::ClaimInterfaceComplete(
    scoped_refptr<InterfaceClaimer> interface_claimer,
    ResultCallback callback) {
  if (!device_) {
    if (interface_claimer) {
      // Ensure that the InterfaceClaimer is released on the blocking thread.
      InterfaceClaimer* raw_interface_claimer = interface_claimer.get();
      interface_claimer->AddRef();
      interface_claimer = nullptr;
      blocking_task_runner_->ReleaseSoon(FROM_HERE, raw_interface_claimer);
    }

    std::move(callback).Run(false);
    return;
  }

  if (interface_claimer) {
    claimed_interfaces_[interface_claimer->interface_number()] =
        interface_claimer;
    RefreshEndpointMap();
  }
  std::move(callback).Run(interface_claimer != nullptr);
}

void UsbDeviceHandleImpl::SetInterfaceAlternateSettingOnBlockingThread(
    int interface_number,
    int alternate_setting,
    ResultCallback callback) {
  int rv = libusb_set_interface_alt_setting(handle_, interface_number,
                                            alternate_setting);
  if (rv != LIBUSB_SUCCESS) {
    USB_LOG(EVENT) << "Failed to set interface " << interface_number
                   << " to alternate setting " << alternate_setting << ": "
                   << ConvertPlatformUsbErrorToString(rv);
  }
  task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&UsbDeviceHandleImpl::SetInterfaceAlternateSettingComplete,
                     this, interface_number, alternate_setting,
                     rv == LIBUSB_SUCCESS, std::move(callback)));
}

void UsbDeviceHandleImpl::SetInterfaceAlternateSettingComplete(
    int interface_number,
    int alternate_setting,
    bool success,
    ResultCallback callback) {
  if (!device_) {
    std::move(callback).Run(false);
    return;
  }

  if (success) {
    claimed_interfaces_[interface_number]->set_alternate_setting(
        alternate_setting);
    RefreshEndpointMap();
  }
  std::move(callback).Run(success);
}

void UsbDeviceHandleImpl::ResetDeviceOnBlockingThread(ResultCallback callback) {
  int rv = libusb_reset_device(handle_);
  if (rv != LIBUSB_SUCCESS) {
    USB_LOG(EVENT) << "Failed to reset device: "
                   << ConvertPlatformUsbErrorToString(rv);
  }
  task_runner_->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), rv == LIBUSB_SUCCESS));
}

void UsbDeviceHandleImpl::ClearHaltOnBlockingThread(uint8_t endpoint,
                                                    ResultCallback callback) {
  int rv = libusb_clear_halt(handle_, endpoint);
  if (rv != LIBUSB_SUCCESS) {
    USB_LOG(EVENT) << "Failed to clear halt: "
                   << ConvertPlatformUsbErrorToString(rv);
  }
  task_runner_->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), rv == LIBUSB_SUCCESS));
}

void UsbDeviceHandleImpl::RefreshEndpointMap() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(device_);
  endpoint_map_.clear();
  const UsbConfigDescriptor* config = device_->active_configuration();
  if (config) {
    for (const auto& map_entry : claimed_interfaces_) {
      int interface_number = map_entry.first;
      const scoped_refptr<InterfaceClaimer>& claimed_iface = map_entry.second;

      for (const UsbInterfaceDescriptor& iface : config->interfaces) {
        if (iface.interface_number == interface_number &&
            iface.alternate_setting == claimed_iface->alternate_setting()) {
          for (const UsbEndpointDescriptor& endpoint : iface.endpoints) {
            endpoint_map_[endpoint.address] = {&iface, &endpoint};
          }
          break;
        }
      }
    }
  }
}

scoped_refptr<UsbDeviceHandleImpl::InterfaceClaimer>
UsbDeviceHandleImpl::GetClaimedInterfaceForEndpoint(uint8_t endpoint) {
  const auto endpoint_it = endpoint_map_.find(endpoint);
  if (endpoint_it != endpoint_map_.end())
    return claimed_interfaces_[endpoint_it->second.interface->interface_number];
  return nullptr;
}

void UsbDeviceHandleImpl::ControlTransferInternal(
    UsbTransferDirection direction,
    UsbControlTransferType request_type,
    UsbControlTransferRecipient recipient,
    uint8_t request,
    uint16_t value,
    uint16_t index,
    scoped_refptr<base::RefCountedBytes> buffer,
    unsigned int timeout,
    scoped_refptr<base::TaskRunner> callback_task_runner,
    TransferCallback callback) {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (!device_) {
    RunTransferCallback(callback_task_runner, std::move(callback),
                        UsbTransferStatus::DISCONNECT, buffer, 0);
    return;
  }

  if (!base::IsValueInRangeForNumericType<uint16_t>(buffer->size())) {
    USB_LOG(USER) << "Transfer too long.";
    RunTransferCallback(callback_task_runner, std::move(callback),
                        UsbTransferStatus::TRANSFER_ERROR, buffer, 0);
    return;
  }

  const size_t resized_length = LIBUSB_CONTROL_SETUP_SIZE + buffer->size();
  auto resized_buffer =
      base::MakeRefCounted<base::RefCountedBytes>(resized_length);
  memcpy(resized_buffer->front() + LIBUSB_CONTROL_SETUP_SIZE, buffer->front(),
         buffer->size());

  std::unique_ptr<Transfer> transfer = Transfer::CreateControlTransfer(
      this, CreateRequestType(direction, request_type, recipient), request,
      value, index, static_cast<uint16_t>(buffer->size()), resized_buffer,
      timeout, callback_task_runner, &callback);
  if (!transfer) {
    DCHECK(callback);
    RunTransferCallback(callback_task_runner, std::move(callback),
                        UsbTransferStatus::TRANSFER_ERROR, buffer, 0);
    return;
  }

  SubmitTransfer(std::move(transfer));
}

void UsbDeviceHandleImpl::IsochronousTransferInInternal(
    uint8_t endpoint_address,
    const std::vector<uint32_t>& packet_lengths,
    unsigned int timeout,
    scoped_refptr<base::TaskRunner> callback_task_runner,
    IsochronousTransferCallback callback) {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (!device_) {
    ReportIsochronousTransferError(callback_task_runner, std::move(callback),
                                   packet_lengths,
                                   UsbTransferStatus::DISCONNECT);
    return;
  }

  size_t length =
      std::accumulate(packet_lengths.begin(), packet_lengths.end(), 0u);
  auto buffer = base::MakeRefCounted<base::RefCountedBytes>(length);
  std::unique_ptr<Transfer> transfer = Transfer::CreateIsochronousTransfer(
      this, endpoint_address, buffer, length, packet_lengths, timeout,
      callback_task_runner, &callback);
  DCHECK(transfer);
  SubmitTransfer(std::move(transfer));
}

void UsbDeviceHandleImpl::IsochronousTransferOutInternal(
    uint8_t endpoint_address,
    scoped_refptr<base::RefCountedBytes> buffer,
    const std::vector<uint32_t>& packet_lengths,
    unsigned int timeout,
    scoped_refptr<base::TaskRunner> callback_task_runner,
    IsochronousTransferCallback callback) {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (!device_) {
    ReportIsochronousTransferError(callback_task_runner, std::move(callback),
                                   packet_lengths,
                                   UsbTransferStatus::DISCONNECT);
    return;
  }

  size_t length =
      std::accumulate(packet_lengths.begin(), packet_lengths.end(), 0u);
  std::unique_ptr<Transfer> transfer = Transfer::CreateIsochronousTransfer(
      this, endpoint_address, buffer, length, packet_lengths, timeout,
      callback_task_runner, &callback);
  DCHECK(transfer);
  SubmitTransfer(std::move(transfer));
}

void UsbDeviceHandleImpl::GenericTransferInternal(
    uint8_t endpoint_address,
    scoped_refptr<base::RefCountedBytes> buffer,
    unsigned int timeout,
    scoped_refptr<base::TaskRunner> callback_task_runner,
    TransferCallback callback) {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (!device_) {
    RunTransferCallback(callback_task_runner, std::move(callback),
                        UsbTransferStatus::DISCONNECT, buffer, 0);
    return;
  }

  const auto endpoint_it = endpoint_map_.find(endpoint_address);
  if (endpoint_it == endpoint_map_.end()) {
    USB_LOG(DEBUG) << "Failed to submit transfer because endpoint "
                   << static_cast<int>(endpoint_address)
                   << " not part of a claimed interface.";
    RunTransferCallback(callback_task_runner, std::move(callback),
                        UsbTransferStatus::TRANSFER_ERROR, buffer, 0);
    return;
  }

  if (!base::IsValueInRangeForNumericType<int>(buffer->size())) {
    USB_LOG(DEBUG) << "Transfer too long.";
    RunTransferCallback(callback_task_runner, std::move(callback),
                        UsbTransferStatus::TRANSFER_ERROR, buffer, 0);
    return;
  }

  std::unique_ptr<Transfer> transfer;
  UsbTransferType transfer_type = endpoint_it->second.endpoint->transfer_type;
  if (transfer_type == UsbTransferType::BULK) {
    transfer = Transfer::CreateBulkTransfer(
        this, endpoint_address, buffer, static_cast<int>(buffer->size()),
        timeout, callback_task_runner, &callback);
  } else if (transfer_type == UsbTransferType::INTERRUPT) {
    transfer = Transfer::CreateInterruptTransfer(
        this, endpoint_address, buffer, static_cast<int>(buffer->size()),
        timeout, callback_task_runner, &callback);
  } else {
    USB_LOG(DEBUG) << "Endpoint " << static_cast<int>(endpoint_address)
                   << " is not a bulk or interrupt endpoint.";
    RunTransferCallback(callback_task_runner, std::move(callback),
                        UsbTransferStatus::TRANSFER_ERROR, buffer, 0);
    return;
  }
  DCHECK(transfer);
  SubmitTransfer(std::move(transfer));
}

void UsbDeviceHandleImpl::SubmitTransfer(std::unique_ptr<Transfer> transfer) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(transfer);

  // Transfer is owned by libusb until its completion callback is run. This
  // object holds a weak reference.
  transfers_.insert(transfer.get());
  blocking_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&Transfer::Submit, base::Unretained(transfer.release())));
}

void UsbDeviceHandleImpl::TransferComplete(Transfer* transfer,
                                           base::OnceClosure callback) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(base::ContainsKey(transfers_, transfer))
      << "Missing transfer completed";
  transfers_.erase(transfer);

  if (transfer->callback_task_runner()->RunsTasksInCurrentSequence()) {
    std::move(callback).Run();
  } else {
    transfer->callback_task_runner()->PostTask(FROM_HERE, std::move(callback));
  }

  // libusb_free_transfer races with libusb_submit_transfer and only work-
  // around is to make sure to call them on the same thread.
  blocking_task_runner_->DeleteSoon(FROM_HERE, transfer);
}

}  // namespace device
