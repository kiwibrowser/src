// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include <map>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/macros.h"
#include "base/observer_list_threadsafe.h"
#include "base/scoped_observer.h"
#include "base/strings/utf_string_conversions.h"
#include "base/synchronization/lock.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/printing/ppd_provider_factory.h"
#include "chrome/browser/chromeos/printing/printer_configurer.h"
#include "chrome/browser/chromeos/printing/printer_event_tracker.h"
#include "chrome/browser/chromeos/printing/printer_event_tracker_factory.h"
#include "chrome/browser/chromeos/printing/synced_printers_manager_factory.h"
#include "chrome/browser/chromeos/printing/usb_printer_detector.h"
#include "chrome/browser/chromeos/printing/usb_printer_util.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/debug_daemon_client.h"
#include "chromeos/printing/ppd_provider.h"
#include "content/public/browser/browser_thread.h"
#include "device/base/device_client.h"
#include "device/usb/usb_device.h"
#include "device/usb/usb_service.h"

namespace chromeos {
namespace {

// Given a usb device, guesses the make and model for a driver lookup.
//
// TODO(justincarlson): Possibly go deeper and query the IEEE1284 fields
// for make and model if we determine those are more likely to contain
// what we want.  Strings currently come from udev.
std::string GuessEffectiveMakeAndModel(const device::UsbDevice& device) {
  return base::UTF16ToUTF8(device.manufacturer_string()) + " " +
         base::UTF16ToUTF8(device.product_string());
}

// The PrinterDetector that drives the flow for setting up a USB printer to use
// CUPS backend.
class UsbPrinterDetectorImpl : public UsbPrinterDetector,
                               public device::UsbService::Observer {
 public:
  explicit UsbPrinterDetectorImpl(device::UsbService* usb_service)
      : usb_observer_(this),
        observer_list_(
            new base::ObserverListThreadSafe<UsbPrinterDetector::Observer>),
        weak_ptr_factory_(this) {
    if (usb_service) {
      usb_observer_.Add(usb_service);
      usb_service->GetDevices(base::Bind(&UsbPrinterDetectorImpl::OnGetDevices,
                                         weak_ptr_factory_.GetWeakPtr()));
    }
  }
  ~UsbPrinterDetectorImpl() override = default;

  // PrinterDetector interface function.
  void AddObserver(UsbPrinterDetector::Observer* observer) override {
    observer_list_->AddObserver(observer);
  }

  // PrinterDetector interface function.
  void RemoveObserver(UsbPrinterDetector::Observer* observer) override {
    observer_list_->RemoveObserver(observer);
  }

  // PrinterDetector interface function.
  std::vector<DetectedPrinter> GetPrinters() override {
    base::AutoLock auto_lock(printers_lock_);
    return GetPrintersLocked();
  }

 private:
  std::vector<DetectedPrinter> GetPrintersLocked() {
    printers_lock_.AssertAcquired();
    std::vector<DetectedPrinter> printers_list;
    printers_list.reserve(printers_.size());
    for (const auto& entry : printers_) {
      printers_list.push_back(entry.second);
    }
    return printers_list;
  }

  // Callback for initial enumeration of usb devices.
  void OnGetDevices(
      const std::vector<scoped_refptr<device::UsbDevice>>& devices) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    for (const auto& device : devices) {
      OnDeviceAdded(device);
    }
  }

  // UsbService::observer override.
  void OnDeviceAdded(scoped_refptr<device::UsbDevice> device) override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    if (!UsbDeviceIsPrinter(*device)) {
      return;
    }
    std::unique_ptr<Printer> converted = UsbDeviceToPrinter(*device);
    if (!converted.get()) {
      // An error will already have been logged if we failed to convert.
      return;
    }
    DetectedPrinter entry;
    entry.printer = *converted;
    entry.ppd_search_data.usb_vendor_id = device->vendor_id();
    entry.ppd_search_data.usb_product_id = device->product_id();
    entry.ppd_search_data.make_and_model.push_back(
        GuessEffectiveMakeAndModel(*device));

    base::AutoLock auto_lock(printers_lock_);
    printers_[device->guid()] = entry;
    observer_list_->Notify(FROM_HERE,
                           &PrinterDetector::Observer::OnPrintersFound,
                           GetPrintersLocked());
  }

  // UsbService::observer override.
  void OnDeviceRemoved(scoped_refptr<device::UsbDevice> device) override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    if (!UsbDeviceIsPrinter(*device)) {
      return;
    }
    base::AutoLock auto_lock(printers_lock_);
    printers_.erase(device->guid());
    observer_list_->Notify(FROM_HERE,
                           &PrinterDetector::Observer::OnPrintersFound,
                           GetPrintersLocked());
  }

  // Map from USB GUID to DetectedPrinter for all detected printers, and
  // associated lock, since we don't require all access to be from the same
  // sequence.
  std::map<std::string, DetectedPrinter> printers_;
  base::Lock printers_lock_;

  ScopedObserver<device::UsbService, device::UsbService::Observer>
      usb_observer_;
  scoped_refptr<base::ObserverListThreadSafe<UsbPrinterDetector::Observer>>
      observer_list_;
  base::WeakPtrFactory<UsbPrinterDetectorImpl> weak_ptr_factory_;
};

}  // namespace

// static
std::unique_ptr<UsbPrinterDetector> UsbPrinterDetector::Create() {
  device::UsbService* usb_service =
      device::DeviceClient::Get()->GetUsbService();
  return std::make_unique<UsbPrinterDetectorImpl>(usb_service);
}

std::unique_ptr<UsbPrinterDetector> UsbPrinterDetector::CreateForTesting(
    device::UsbService* usb_service) {
  return std::make_unique<UsbPrinterDetectorImpl>(usb_service);
}

}  // namespace chromeos
